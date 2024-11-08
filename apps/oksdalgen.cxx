#include "class_info.hpp"

#include "oks/kernel.hpp"
#include "oks/class.hpp"
#include "oks/attribute.hpp"
#include "oks/relationship.hpp"
#include "oks/method.hpp"

#include <stdlib.h>
#include <ctype.h>

#include <algorithm>
#include <string>
#include <list>
#include <set>
#include <iostream>

using namespace dunedaq;
using namespace dunedaq::oksdalgen;

  // declare external functions

extern std::string alnum_name(const std::string& in);
extern std::string capitalize_name(const std::string& in);
extern void print_description(std::ostream& s, const std::string& text, const char * dx);
extern void print_indented(std::ostream& s, const std::string& text, const char * dx);
extern std::string get_type(oks::OksData::Type oks_type, bool is_cpp);
extern void gen_dump_application(std::ostream& s, std::list<std::string>& class_names, const std::string& cpp_ns_name, const std::string& cpp_hdr_dir, const char * conf_header, const char * conf_name, const char * headres_prologue, const char * main_function_prologue);
extern void write_info_file(std::ostream& s, const std::string& cpp_namespace, const std::string& cpp_header_dir, const std::set<const oks::OksClass *, std::less<const oks::OksClass *> >& class_names);
extern std::string get_full_cpp_class_name(const oks::OksClass * c, const ClassInfo::Map& cl_info, const std::string & cpp_ns_name);
extern std::string get_include_dir(const oks::OksClass * c, const ClassInfo::Map& cl_info, const std::string& cpp_hdr_dir);
// extern const std::string& get_package_name(const oks::OksClass * c, const ClassInfo::Map& cl_info, const std::string& java_p_name);
extern void parse_arguments(int argc, char *argv[], std::list<std::string>& class_names, std::list<std::string>& file_names, std::list<std::string>& include_dirs, std::list<std::string>& user_classes, std::string& cpp_dir_name, std::string& cpp_ns_name, std::string& cpp_hdr_dir, std::string& info_file_name, bool& verbose);
extern bool process_external_class(ClassInfo::Map& cl_info, const oks::OksClass * c, const std::list<std::string>& include_dirs, const std::list<std::string>& user_classes, bool verbose);
extern std::string int2dx(int level);
extern int open_cpp_namespace(std::ostream& s, const std::string& value);
extern void close_cpp_namespace(std::ostream& s, int level);
extern std::string get_method_header_prologue(oks::OksMethodImplementation *);
extern std::string get_method_header_epilogue(oks::OksMethodImplementation *);
extern std::string get_public_section(oks::OksMethodImplementation * mi);
extern std::string get_private_section(oks::OksMethodImplementation * mi);
extern std::string get_member_initializer_list(oks::OksMethodImplementation * mi);
extern std::string get_method_implementation_body(oks::OksMethodImplementation * mi);
extern bool get_add_algo_1(oks::OksMethodImplementation * mi);
extern bool get_add_algo_n(oks::OksMethodImplementation * mi);
extern oks::OksMethodImplementation * find_cpp_method_implementation(const oks::OksMethod * method);


  /**
   *  The function has_superclass returns true if 'tested' class
   *  has class 'c' as a superclass
   */

static bool
has_superclass(const oks::OksClass * tested, const oks::OksClass * c)
{
  if (const oks::OksClass::FList * sclasses = tested->all_super_classes())
    {
      for (const auto& i : *sclasses)
        {
          if (i == c)
            {
              return true;
            }
        }
    }

  return false;
}

const std::string WHITESPACE = " \n\r\t\f\v";
 
std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}
 
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
 
std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

const std::vector<std::string> cpp_method_virtual_specifiers = {
    "virtual",
    "override",
    "final"
};

static void
gen_header(const oks::OksClass *cl,
           std::ostream& cpp_file,
           const std::string& cpp_ns_name,
           const std::string& cpp_hdr_dir,
           const ClassInfo::Map& cl_info)
{
  const std::string name(alnum_name(cl->get_name()));


    // get includes for super classes if necessary

  if (const std::list<std::string*> * super_list = cl->direct_super_classes())
    {
      cpp_file << "  // include files for classes used in inheritance hierarchy\n\n";

      for (const auto& i : *super_list)
        {
          oks::OksClass * c = cl->get_kernel()->find_class(*i);
          cpp_file << "#include \"" << get_include_dir(c, cl_info, cpp_hdr_dir) << ".hpp\"\n";
        }
    }

  cpp_file << std::endl;


    // generate forward declarations if necessary
    {
      // create set of classes to avoid multiple forward declarations of the same class
      std::set<oks::OksClass*> rclasses;

      // check direct relationships (c++)
      if (cl->direct_relationships() && !cl->direct_relationships()->empty())
        for (const auto &i : *cl->direct_relationships())
          rclasses.insert(i->get_class_type());

      // check methods
      if (const std::list<oks::OksMethod*> *mlist = cl->direct_methods())
        {
          for (const auto &i : *mlist)
            {
              if (oks::OksMethodImplementation *mi = find_cpp_method_implementation(i))
                {
                  const std::string mp(mi->get_prototype());
                  for (const auto &j : cl->get_kernel()->classes())
                    {
                      const std::string s(j.second->get_name());
                      std::string::size_type idx = mp.find(s);
                      if (idx != std::string::npos && (idx == 0 || !isalnum(mp[idx - 1])) && !isalnum(mp[idx + s.size()]))
                        rclasses.insert(j.second);
                    }
                }
            }
        }

      NameSpaceInfo ns_info;

      for (const auto &c : rclasses)
        {
          // check if the class' header is not already included
          if (has_superclass(cl, c) || cl == c)
            continue;

          ClassInfo::Map::const_iterator idx = cl_info.find(c);
          ns_info.add((idx != cl_info.end() ? (*idx).second.get_namespace() : cpp_ns_name), alnum_name(c->get_name()));
        }

      if (!ns_info.empty())
        {
          cpp_file << "  // forward declaration for classes used in relationships and algorithms\n\n";
          ns_info.print(cpp_file, 0);
          cpp_file << "\n\n";
        }
    }

    // generate methods prologues if necessary

  if (const std::list<oks::OksMethod*> *mlist = cl->direct_methods())
    for (const auto &i : *mlist)
      if (oks::OksMethodImplementation *mi = find_cpp_method_implementation(i))
        if (!get_method_header_prologue(mi).empty())
          {
            cpp_file << "  // prologue of method " << cl->get_name() << "::" << i->get_name() << "()\n";
            cpp_file << get_method_header_prologue(mi) << std::endl;
          }


    // open namespace

  int ns_level = open_cpp_namespace(cpp_file, cpp_ns_name);
  std::string ns_dx = int2dx(ns_level);
  std::string ns_dx2 = ns_dx + "    ";
  
  const char * dx  = ns_dx.c_str();    // are used for alignment
  const char * dx2 = ns_dx2.c_str();


    // generate description

  {
    std::string txt("Declares methods to get and put values of attributes / relationships, to print and to destroy object.\n");

    txt +=
      "<p>\n"
      "The methods can throw several exceptions:\n"
      "<ul>\n"
      " <li><code>conffwk.NotFoundException</code> - in case of wrong attribute or relationship name (e.g. in case of database schema modification)\n"
      " <li><code>conffwk.SystemException</code> - in case of system problems (communication or implementation database failure, schema modification, object destruction, etc.)\n"
      "</ul>\n"
      "<p>\n"
      "In addition the methods modifying database (set value, destroy object) can throw <code>conffwk.NotAllowedException</code> "
      "exception in case, if there are no write access rights or database is already locked by other process.\n";

    if (!cl->get_description().empty())
      {
        txt += "\n<p>\n";
        txt += cl->get_description();
        txt += "\n";
      }
 
    txt += "@author oksdalgen\n";

    cpp_file << std::endl;
    print_description(cpp_file, cl->get_description(), dx);
  }

    // generate class declaration itself

  cpp_file << dx << "class " << name << " : ";

    // generate inheritance list

  if (const std::list<std::string*> * super_list = cl->direct_super_classes())
    {
      for (std::list<std::string*>::const_iterator i = super_list->begin(); i != super_list->end();)
        {
          const oks::OksClass * c = cl->get_kernel()->find_class(**i);
          cpp_file << "public " << get_full_cpp_class_name(c, cl_info, cpp_ns_name);
          if (++i != super_list->end())
            cpp_file << ", ";
        }
    }
  else
    {
      cpp_file << "public virtual dunedaq::conffwk::DalObject";
    }

  cpp_file << " {\n\n";

    // generate standard methods

  cpp_file
    << dx << "  friend class conffwk::Configuration;\n"
    << dx << "  friend class conffwk::Configuration::Cache<" << name << ">;\n\n"
    << dx << "  friend class conffwk::DalObject;\n"
    << dx << "  friend class conffwk::DalFactory;\n"
    << dx << "  friend class conffwk::DalRegistry;\n\n"
    << dx << "  protected:\n\n"
    << dx << "    " << name << "(conffwk::DalRegistry& db, const conffwk::ConfigObject& obj) noexcept;\n"
    << dx << "    virtual ~" << name << "() noexcept;\n"
    << dx << "    virtual void init(bool init_children);\n\n"
    << dx << "  public:\n\n"
    << dx << "      /** The name of the configuration class. */\n\n"
    << dx << "    static const std::string& s_class_name;\n\n\n"
    << dx << "      /**\n"
    << dx << "       * \\brief Print details of the " << name << " object.\n"
    << dx << "       *\n"
    << dx << "       * Parameters are:\n"
    << dx << "       *   \\param offset        number of spaces to shift object right (useful to print nested objects)\n"
    << dx << "       *   \\param print_header  if false, do not print object header (to print attributes of base classes)\n"
    << dx << "       *   \\param s             output stream\n"
    << dx << "       */\n\n"
    << dx << "    virtual void print(unsigned int offset, bool print_header, std::ostream& s) const;\n\n\n"
    << dx << "      /**\n"
    << dx << "       * \\brief Get values of relationships and results of some algorithms as a vector of dunedaq::conffwk::DalObject pointers.\n"
    << dx << "       *\n"
    << dx << "       * Parameters are:\n"
    << dx << "       *   \\param name          name of the relationship or algorithm\n"
    << dx << "       *   \\return              value of relationship or result of algorithm\n"
    << dx << "       *   \\throw               std::exception if there is no relationship or algorithm with such name in this and base classes\n"
    << dx << "       */\n\n"
    << dx << "    virtual std::vector<const dunedaq::conffwk::DalObject *> get(const std::string& name, bool upcast_unregistered = true) const;\n\n\n"
    << dx << "  protected:\n\n"
    << dx << "    bool get(const std::string& name, std::vector<const dunedaq::conffwk::DalObject *>& vec, bool upcast_unregistered, bool first_call) const;\n\n\n";


    // generate class attributes and relationships in accordance with
    // database schema

  if (cl->direct_attributes() || cl->direct_relationships() || cl->direct_methods())
    {
      cpp_file << dx << "  private:\n\n";

      // generate attributes:
      //  - for single attributes this is a normal member variable.
      //  - for multiple values this is a std::vector<T>.

      if (const std::list<oks::OksAttribute*> * alist = cl->direct_attributes())
        {
          for (const auto& i : *alist)
            {
              const std::string aname(alnum_name(i->get_name()));
              std::string cpp_type = get_type(i->get_data_type(), true);

              if (i->get_is_multi_values())
                cpp_file << dx << "    std::vector<" << cpp_type << "> m_" << aname << ";\n";
              else
                cpp_file << dx << "    " << cpp_type << " m_" << aname << ";\n";
            }
        }


      // generate relationships:
      //  - for single values this is just a pointer.
      //  - for multiple values this is a vector of pointers.

      if (const std::list<oks::OksRelationship*> *rlist = cl->direct_relationships())
        {
          for (const auto& i : *rlist)
            {
              const std::string rname(alnum_name(i->get_name()));
              const std::string full_class_name(get_full_cpp_class_name(i->get_class_type(), cl_info, cpp_ns_name));
              if (i->get_high_cardinality_constraint() == oks::OksRelationship::Many)
                cpp_file << dx << "    std::vector<const " << full_class_name << "*> m_" << rname << ";\n";
              else
                cpp_file << dx << "    const " << full_class_name << "* m_" << rname << ";\n";
            }
        }


      // generate methods extension if any

      if (const std::list<oks::OksMethod*> * mlist = cl->direct_methods())
        {
          for (const auto& i : *mlist)
            {
              if (oks::OksMethodImplementation * mi = find_cpp_method_implementation(i))
                {
                  std::string method_extension = get_private_section(mi);
                  if (!method_extension.empty())
                    {
                      cpp_file << "\n" << dx << "      // extension of method " << cl->get_name() << "::" << i->get_name() << "()\n";
                      print_indented(cpp_file, method_extension, dx2);
                    }
                }
            }
        }


      cpp_file << std::endl << std::endl << dx << "  public:\n\n";


      // generate attribute accessors:
      //  1. for each attribute generate static std::string with it's name
      //  2a. for single values this is just:
      //     - attribute_type attribute_name() const { return m_attribute; }
      //  2b. for multiple values this is:
      //     - const std::vector<attribute_type>& attribute_name() const { return m_attribute; }

      if (const std::list<oks::OksAttribute*> *alist = cl->direct_attributes())
        {

          cpp_file << dx << "      // attribute names\n\n";

          for (const auto& i : *alist)
            {
              const std::string& aname(i->get_name());
              cpp_file << dx << "    inline static const std::string s_" << alnum_name(aname) << " = \"" << aname << "\";\n";
            }

          cpp_file << "\n";

          for (const auto& i : *alist)
            {
              const std::string& cpp_aname(alnum_name(i->get_name()));
              cpp_file << dx << "    static const std::string& __get_" << cpp_aname << "_str() noexcept { return s_" << cpp_aname << "; }\n";
            }

          cpp_file << std::endl << std::endl;

          for (const auto& i : *alist)
            {
              const std::string aname(alnum_name(i->get_name()));

              // generate enum values

              if (i->get_data_type() == oks::OksData::enum_type && !i->get_range().empty())
                {
                  std::string description("Valid enumeration values to compare with value returned by get_");
                  description += aname;
                  description += "() and to pass value to set_";
                  description += aname;
                  description += "() methods.";

                  print_description(cpp_file, description, dx2);

                  description += "\nUse toString() method to compare and to pass the values. Do not use name() method.";

                  cpp_file << dx << "    struct " << capitalize_name(aname) << " {\n";

                  oks::Oks::Tokenizer t(i->get_range(), ",");
                  std::string token;
                  while (!(token = t.next()).empty())
                    {
                      cpp_file << dx << "      inline static const std::string " << capitalize_name(alnum_name(token)) << " = \"" << token << "\";\n";
                    }

                  cpp_file << dx << "    };\n\n";
                }

              // generate get method description

                {

                  std::string description("Get \"");
                  description += i->get_name();
                  description += "\" attribute value.\n\n";
                  description += i->get_description();

                  std::string description2("\\brief ");
                  description2 += description;
                  description2 += "\n\\return the attribute value\n";
                  description2 += "\\throw dunedaq::conffwk::Generic, dunedaq::conffwk::DeletedObject\n";

                  print_description(cpp_file, description2, dx2);
                }

              // generate method body

              cpp_file << dx << "    ";

              std::string cpp_type = get_type(i->get_data_type(), true);

              if (i->get_is_multi_values())
                {
                  cpp_file << "const std::vector<" << cpp_type << ">&";
                }
              else
                {
                  if (cpp_type == "std::string")
                    cpp_file << "const std::string&";
                  else
                    cpp_file << cpp_type;
                }

              cpp_file << '\n'
                  << dx << "    get_" << aname << "() const\n"
                  << dx << "      {\n"
                  << dx << "        std::lock_guard scoped_lock(m_mutex);\n"
                  << dx << "        check();\n"
                  << dx << "        check_init();\n"
                  << dx << "        return m_" << aname << ";\n"
                  << dx << "      }\n\n";

              // generate set method description

                {
                  std::string description("Set \"");
                  description += i->get_name();
                  description += "\" attribute value.\n\n";
                  description += i->get_description();

                  std::string description2("\\brief ");
                  description2 += description;
                  description2 += "\n\\param value  new attribute value\n";
                  description2 += "\\throw dunedaq::conffwk::Generic, dunedaq::conffwk::DeletedObject\n";

                  print_description(cpp_file, description2, dx2);
                }

              // generate set method

              cpp_file << dx << "    void\n" << dx << "    set_" << aname << '(';

              if (i->get_is_multi_values())
                {
                  cpp_file << "const std::vector<" << cpp_type << ">&";
                }
              else
                {
                  if (cpp_type == "std::string")
                    {
                      cpp_file << "const std::string&";
                    }
                  else
                    {
                      cpp_file << cpp_type;
                    }
                }

              cpp_file << " value)\n"
                << dx << "      {\n"
                << dx << "        std::lock_guard scoped_lock(m_mutex);\n"
                << dx << "        check();\n"
                << dx << "        clear();\n"
                << dx << "        p_obj.";

              if (i->get_data_type() == oks::OksData::string_type && i->get_is_multi_values() == false)
                {
                  cpp_file << "set_by_ref";
                }
              else if (i->get_data_type() == oks::OksData::enum_type)
                {
                  cpp_file << "set_enum";
                }
              else if (i->get_data_type() == oks::OksData::class_type)
                {
                  cpp_file << "set_class";
                }
              else if (i->get_data_type() == oks::OksData::date_type)
                {
                  cpp_file << "set_date";
                }
              else if (i->get_data_type() == oks::OksData::time_type)
                {
                  cpp_file << "set_time";
                }
              else
                {
                  cpp_file << "set_by_val";
                }

              cpp_file << "(s_" << aname << ", value);\n"
                  << dx << "      }\n\n\n";
            }
        }


      // generate relationship accessors.
      //  1. for each relationship generate static std::string with it's name
      //  2a. for single values this is just:
      //      - const relation_type * relation() const { return m_relation; }
      //  2b. for multiple values this is:
      //      - const std::vector<const relation_type*>& relation() const { return m_relation; }

      if (const std::list<oks::OksRelationship*> *rlist = cl->direct_relationships())
        {

          cpp_file << dx << "      // relationship names\n\n";

          for (const auto& i : *rlist)
            {
              const std::string& rname(i->get_name());
              cpp_file << dx << "    inline static const std::string s_" << alnum_name(rname) << " = \"" << rname << "\";\n";
            }

          cpp_file << "\n";

          for (const auto& i : *rlist)
            {
              const std::string& cpp_rname(alnum_name(i->get_name()));
              cpp_file << dx << "    static const std::string& __get_" << cpp_rname << "_str() noexcept { return s_" << cpp_rname << "; }\n";
            }

          cpp_file << std::endl << std::endl;

          for (const auto& i : *rlist)
            {

              // generate description

                {
                  std::string description("Get \"");
                  description += i->get_name();
                  description += "\" relationship value.\n\n";
                  description += i->get_description();

                  std::string description2("\\brief ");
                  description2 += description;
                  description2 += "\n\\return the relationship value\n";
                  description2 += "\\throw dunedaq::conffwk::Generic, dunedaq::conffwk::DeletedObject\n";

                  print_description(cpp_file, description2, dx2);
                }

              // generate method body

              cpp_file << dx << "    const ";

              const std::string rname(alnum_name(i->get_name()));
              std::string full_cpp_class_name = get_full_cpp_class_name(i->get_class_type(), cl_info, cpp_ns_name);

              if (i->get_high_cardinality_constraint() == oks::OksRelationship::Many)
                {
                  cpp_file << "std::vector<const " << full_cpp_class_name << "*>&";
                }
              else
                {
                  cpp_file << full_cpp_class_name << " *";
                }

              cpp_file << "\n"
                  << dx << "    get_" << rname << "() const\n"
                  << dx << "    {\n"
                  << dx << "      std::lock_guard scoped_lock(m_mutex);\n"
                  << dx << "      check();\n"
                  << dx << "      check_init();\n";

              if (i->get_low_cardinality_constraint() == oks::OksRelationship::One)
                {
                  if (i->get_high_cardinality_constraint() == oks::OksRelationship::One)
                    {
                      cpp_file
                          << dx << "      if (!m_" << rname << ")\n"
                          << dx << "        {\n"
                          << dx << "          std::ostringstream text;\n"
                          << dx << "          text << \"relationship \\\"\" << s_" << rname << " << \"\\\" of object \" << this << \" is not set\";\n"
                          << dx << "          throw dunedaq::conffwk::Generic(ERS_HERE, text.str().c_str());\n"
                          << dx << "        }\n";
                    }
                  else
                    {
                      cpp_file
                          << dx << "      if (m_" << rname << ".empty())\n"
                          << dx << "        {\n"
                          << dx << "          std::ostringstream text;\n"
                          << dx << "          text << \"relationship \\\"\" << s_" << rname << " << \"\\\" of object \" << this << \" is empty\";\n"
                          << dx << "          throw dunedaq::conffwk::Generic(ERS_HERE, text.str().c_str());\n"
                          << dx << "        }\n";
                    }
                }

              cpp_file
                  << dx << "      return m_" << rname << ";\n"
                  << dx << "    }\n\n\n";

              // generate set method

                {
                  std::string description("Set \"");
                  description += i->get_name();
                  description += "\" relationship value.\n\n";
                  description += i->get_description();

                  std::string description2("\\brief ");
                  description2 += description;
                  description2 += "\n\\param value  new relationship value\n";
                  description2 += "\\throw dunedaq::conffwk::Generic, dunedaq::conffwk::DeletedObject\n";

                  print_description(cpp_file, description2, dx2);
                }

              cpp_file << dx << "    void\n" << dx << "    set_" << rname << "(const ";

              if (i->get_high_cardinality_constraint() == oks::OksRelationship::Many)
                {
                  cpp_file << "std::vector<const " << full_cpp_class_name << "*>&";
                }
              else
                {
                  cpp_file << full_cpp_class_name << " *";
                }

              cpp_file << " value);\n\n";
            }
        }
    }


    // generate methods

  if (const std::list<oks::OksMethod*> *mlist = cl->direct_methods())
    {
      bool cpp_comment_is_printed = false;

      for (const auto& i : *mlist)
        {

          // C++ algorithms

          if (oks::OksMethodImplementation * mi = find_cpp_method_implementation(i))
            {
              if (cpp_comment_is_printed == false)
                {
                  cpp_file << std::endl << dx << "  public:\n\n" << dx << "      // user-defined algorithms\n\n";
                  cpp_comment_is_printed = true;
                }
              else
                {
                  cpp_file << "\n\n";
                }

              // generate description

              print_description(cpp_file, i->get_description(), dx2);

              // generate prototype

              cpp_file << dx << "    " << mi->get_prototype() << ";\n";


              // generate public section extension

              std::string public_method_extension = get_public_section(mi);
              if (!public_method_extension.empty())
                {
                  cpp_file << "\n" << "      // extension of method " << cl->get_name() << "::" << i->get_name() << "()\n";
                  print_indented(cpp_file, public_method_extension, "    ");
                }
            }

        }
    }


    // class finished

  cpp_file << dx << "};\n\n";


    // generate ostream operators and typedef for iterator

  cpp_file
    << dx << "  // out stream operator\n\n"
    << dx << "inline std::ostream& operator<<(std::ostream& s, const " << name << "& obj)\n"
    << dx << "  {\n"
    << dx << "    return obj.print_object(s);\n"
    << dx << "  }\n\n"
    << dx << "typedef std::vector<const " << name << "*>::const_iterator " << name << "Iterator;\n\n";


    // close namespace

  close_cpp_namespace(cpp_file, ns_level);


    // generate methods epilogues if necessary

  if (const std::list<oks::OksMethod*> * mlist = cl->direct_methods())
    {
      for (const auto & i : *mlist)
        {
          oks::OksMethodImplementation * mi = find_cpp_method_implementation(i);
          if (mi && !get_method_header_epilogue(mi).empty())
            {
              cpp_file << "  // epilogue of method " << cl->get_name() << "::" << i->get_name() << "()\n";
              cpp_file << get_method_header_epilogue(mi) << std::endl;
            }
        }
    }

}


static void
set2out(std::ostream& out, const std::set<std::string>& data, bool& is_first)
{
  for (const auto& x : data)
    {
      if (is_first)
        is_first = false;
      else
        out << ',';
      out << '\"' << x << "()\"";
    }
}


static void
gen_cpp_body(const oks::OksClass *cl, std::ostream& cpp_s, const std::string& cpp_ns_name, const std::string& cpp_hdr_dir, const ClassInfo::Map& cl_info)
{
  cpp_s << "#include \"logging/Logging.hpp\"\n\n";

  const std::string name(alnum_name(cl->get_name()));

  std::set<oks::OksClass *> rclasses;

    // for each relationship, include the header file

  if (const std::list<oks::OksRelationship*> * rlist = cl->direct_relationships())
    {
      for (const auto& i : *rlist)
        {
          oks::OksClass * c = i->get_class_type();
          if (has_superclass(cl, c) == false && cl != c)
            {
              rclasses.insert(c);
            }
        }
    }

  if (const std::list<oks::OksMethod*> * mlist = cl->direct_methods())
    {
      for (const auto& i : *mlist)
        {
          if (oks::OksMethodImplementation * mi = find_cpp_method_implementation(i))
            {
              std::string prototype(mi->get_prototype());
              std::string::size_type idx = prototype.find('(');

              if (get_add_algo_n(mi) || get_add_algo_1(mi))
                {

                  if (idx != std::string::npos)
                    {
                      idx--;

                      // skip spaces between method name and ()
                      while (isspace(prototype[idx]) && idx > 0)
                        idx--;

                      // find beginning of the method name
                      while (!isspace(prototype[idx]) && idx > 0)
                        idx--;

                      // remove method name and arguments
                      prototype.erase(idx + 1);

                      // remove spaces
                      prototype.erase(std::remove_if(prototype.begin(), prototype.end(), [](unsigned char x)
                        { return std::isspace(x);}), prototype.end());

                      if (prototype.empty() == false)
                        {
                          idx = prototype.find('*');
                          prototype.erase(idx--);

                          if (idx != std::string::npos)
                            {
                              while (isalnum(prototype[idx]) && idx > 0)
                                idx--;

                              if(idx == 0 && get_add_algo_1(mi) && prototype.find("const") == 0)
                                prototype.erase(0, 5);
                              else
                                prototype.erase(0, idx+1);

                              if (oks::OksClass * c = cl->get_kernel()->find_class(prototype))
                                rclasses.insert(c);
                            }
                        }
                    }
                }
            }
        }
    }

  if (!rclasses.empty())
    {
      cpp_s << "  // include files for classes used in relationships and algorithms\n\n";

      for (const auto& j : rclasses)
        {
          cpp_s << "#include \"" << get_include_dir(j, cl_info, cpp_hdr_dir) << ".hpp\"\n";
        }

      cpp_s << std::endl << std::endl;
    }


    // open namespace

  int ns_level = open_cpp_namespace(cpp_s, cpp_ns_name);
  std::string ns_dx = int2dx(ns_level);
  std::string ns_dx2 = ns_dx + "    ";

  const char * dx  = ns_dx.c_str();    // are used for alignment
  const char * dx2 = ns_dx2.c_str();


    // static objects

  cpp_s << dx << "const std::string& " << name << "::s_class_name(dunedaq::conffwk::DalFactory::instance().get_known_class_name_ref(\"" << name << "\"));\n\n";

  std::set<std::string> algo_n_set, algo_1_set;

  if (const std::list<oks::OksMethod*> * mlist = cl->direct_methods())
    {
      for (const auto& i : *mlist)
        {
          if(oks::OksMethodImplementation * mi = find_cpp_method_implementation(i))
            {
              std::string prototype(mi->get_prototype());
              std::string::size_type idx = prototype.find('(');

              if (idx != std::string::npos)
                {
                  idx--;

                  // skip spaces between method name and ()
                  while (isspace(prototype[idx]) && idx > 0)
                    idx--;

                  // remove method arguments
                  prototype.erase(idx+1);

                  // find beginning of the method name
                  while (!isspace(prototype[idx]) && idx > 0)
                    idx--;

                  if (idx > 0)
                    {
                      prototype.erase(0, idx+1);

                      if (get_add_algo_n(mi))
                        {
                          algo_n_set.insert(prototype);
                        }
                      else if (get_add_algo_1(mi))
                        {
                          algo_1_set.insert(prototype);
                        }
                    }
                }
            }
        }
    }


  cpp_s
    << dx << "  // the factory initializer\n\n"
    << dx << "static struct __" << name << "_Registrator\n"
    << dx << "  {\n"
    << dx << "    __" << name << "_Registrator()\n"
  //   << dx << "      {\n"
  //   << dx << "        dunedaq::conffwk::DalFactory::instance().register_dal_class<" << name << ">(\"" << cl->get_name() << "\", {";

  //   {
  //     bool is_first = true;
  //     set2out(cpp_s, algo_1_set, is_first);
  //     set2out(cpp_s, algo_n_set, is_first);
  //   }

  // cpp_s
  //   << "});\n"
  //   << dx << "      }\n"
  //   << dx << "  } registrator;\n\n\n";

    << dx << "      {\n"
    << dx << "        dunedaq::conffwk::DalFactory::instance().register_dal_class_2g<" << name << ">(\"" << cl->get_name() << "\");\n"
    << dx << "      }\n"
    << dx << "  } registrator;\n\n\n";


    // the constructor

  cpp_s
    << dx << "  // the constructor\n\n"
    << dx << name << "::" << name << "(conffwk::DalRegistry& db, const conffwk::ConfigObject& o) noexcept :\n"
    << dx << "  " << "dunedaq::conffwk::DalObject(db, o)";


    // fill member initializer list, if any

  std::list<std::string> initializer_list;

  if(const std::list<std::string*> * slist = cl->direct_super_classes())
    {
      for(auto & i : *slist)
        {
          initializer_list.push_back(get_full_cpp_class_name(cl->get_kernel()->find_class(*i), cl_info, "") + "(db, o)");
        }
    }

  if (const std::list<oks::OksRelationship *> *rlist = cl->direct_relationships())
    {
      for (std::list<oks::OksRelationship*>::const_iterator i = rlist->begin(); i != rlist->end(); ++i)
        {
          if ((*i)->get_high_cardinality_constraint() != oks::OksRelationship::Many)
            {
              initializer_list.push_back(std::string("m_") + alnum_name((*i)->get_name()) + " (nullptr)");
            }
        }
    }


  if(const std::list<oks::OksMethod*> * mlist = cl->direct_methods())
    {
      for (std::list<oks::OksMethod*>::const_iterator i = mlist->begin(); i != mlist->end(); ++i)
        {
          if (oks::OksMethodImplementation * mi = find_cpp_method_implementation(*i))
            {
              std::string member_initializer = get_member_initializer_list(mi);
              member_initializer.erase(remove(member_initializer.begin(), member_initializer.end(), '\n'), member_initializer.end());

              if (!member_initializer.empty())
                {
                  initializer_list.push_back(member_initializer);
                }
            }
        }
    }

  if (initializer_list.empty() == false)
    {
      for (auto & i : initializer_list)
        {
          cpp_s << ",\n" << dx << "  " << i;
        }

      cpp_s << std::endl;
    }

  cpp_s << "\n"
    << dx << "{\n"
    << dx << "  ;\n"
    << dx << "}\n\n\n";


    // print method
 
  cpp_s
    << dx << "void " << name << "::print(unsigned int indent, bool print_header, std::ostream& s) const\n"
    << dx << "{\n"
    << dx << "  check_init();\n\n"
    << dx << "  try {\n";

  if (cl->direct_attributes() || cl->direct_relationships())
    cpp_s << dx << "    const std::string str(indent+2, ' ');\n";

  cpp_s
    << '\n'
    << dx << "    if (print_header)\n"
    << dx << "      p_hdr(s, indent, s_class_name";

  if(!cpp_ns_name.empty()) {
    cpp_s << ", \"" << cpp_ns_name << '\"';
  }

  cpp_s << ");\n";


  if (const std::list<std::string*> * slist = cl->direct_super_classes())
    {
      cpp_s << "\n\n" << dx << "      // print direct super-classes\n\n";

      for (const auto& i : *slist)
        cpp_s << dx << "    " << get_full_cpp_class_name(cl->get_kernel()->find_class(*i), cl_info, cpp_ns_name) << "::print(indent, false, s);\n";
    }

  if(const std::list<oks::OksAttribute*> * alist = cl->direct_attributes()) {
    cpp_s << "\n\n" << dx << "      // print direct attributes\n\n";

    for(const auto& i : *alist) {
      const std::string aname(alnum_name(i->get_name()));
      std::string abase = (i->get_format() == oks::OksAttribute::Hex) ? "<dunedaq::conffwk::hex>" : (i->get_format() == oks::OksAttribute::Oct) ? "<dunedaq::conffwk::oct>" : "";

      if (i->get_is_multi_values())
        cpp_s << dx << "    dunedaq::conffwk::p_mv_attr" << abase << "(s, str, s_" << aname << ", m_" << aname << ");\n";
      else
        cpp_s << dx << "    dunedaq::conffwk::p_sv_attr" << abase << "(s, str, s_" << aname << ", m_" << aname << ");\n";
    }
  }

  if (const std::list<oks::OksRelationship*> * rlist = cl->direct_relationships())
    {
      cpp_s << "\n\n" << dx << "      // print direct relationships\n\n";

      for (const auto& i : *rlist)
        {
          const std::string rname(alnum_name(i->get_name()));

          if (i->get_high_cardinality_constraint() == oks::OksRelationship::Many)
            {
              if (i->get_is_composite())
                cpp_s << dx << "    dunedaq::conffwk::p_mv_rel(s, str, indent, s_" << rname << ", m_" << rname << ");\n";
              else
                cpp_s << dx << "    dunedaq::conffwk::p_mv_rel(s, str, s_" << rname << ", m_" << rname << ");\n";
            }
          else
            {
              if (i->get_is_composite())
                cpp_s << dx <<"    dunedaq::conffwk::p_sv_rel(s, str, indent, s_" << rname << ", m_" << rname << ");\n";
              else
                cpp_s << dx <<"    dunedaq::conffwk::p_sv_rel(s, str, s_" << rname << ", m_" << rname << ");\n";
            }
        }
    }

  cpp_s << dx << "  }\n"
        << dx << "  catch (dunedaq::conffwk::Exception & ex) {\n"
        << dx << "    dunedaq::conffwk::DalObject::p_error(s, ex);\n"
	<< dx << "  }\n"
        << dx << "}\n\n\n";


   // init method

  {
    const char * ic_value = (
      ( cl->direct_relationships() && cl->direct_relationships()->size() ) ||
      ( cl->direct_super_classes() && cl->direct_super_classes()->size() )
        ? "init_children"
        : "/* init_children */"
    );

    cpp_s
      << dx << "void " << name << "::init(bool " << ic_value << ")\n"
      << dx << "{\n";
  }

  if (cl->direct_super_classes() == nullptr)
    {
      cpp_s
        << dx << "  p_was_read = true;\n"
        << dx << "  increment_read();\n";
    }

    // generate initialization for super classes    

  if (const std::list<std::string*> * slist = cl->direct_super_classes())
    {
      for (const auto& i : *slist)
        {
          cpp_s << dx << "  " << alnum_name(*i) << "::init(init_children);\n";
        }
      if (!slist->empty())
        cpp_s << std::endl;
    }

  cpp_s << dx << "  TLOG_DEBUG(5) << \"read object \" << this << \" (class \" << s_class_name << \')\';\n";

    // put try / catch only if there are attributes or relationships to be initialized
  const std::list<oks::OksAttribute*> *alist = cl->direct_attributes();
  const std::list<oks::OksRelationship*> *rlist = cl->direct_relationships();

  if ((alist && !alist->empty()) || (rlist && !rlist->empty()))
    {
      cpp_s << std::endl << dx << "  try {\n";


      // generate initialization for attributes
      if (alist)
        {
          for (const auto& i : *alist)
            {
              const std::string cpp_name = alnum_name(i->get_name());
              cpp_s << dx << "    p_obj.get(s_" << cpp_name << ", m_" << cpp_name << ");\n";
            }
        }


      // generate initialization for relationships
      if (rlist)
        {
          for (const auto& i : *rlist)
            {
              const std::string& rname = i->get_name();
              std::string cpp_name = alnum_name(rname);
              std::string rcname = get_full_cpp_class_name(i->get_class_type(), cl_info, cpp_ns_name);
              if (i->get_high_cardinality_constraint() == oks::OksRelationship::Many)
                {
                  cpp_s << dx << "    p_registry._ref<" << rcname << ">(p_obj, s_" << cpp_name << ", " << "m_" << cpp_name << ", init_children);\n";
                }
              else
                {
                  cpp_s << dx << "    m_" << cpp_name << " = p_registry._ref<" << rcname << ">(p_obj, s_" << cpp_name << ", init_children);\n";
                }
            }
        }

      cpp_s
        << dx << "  }\n"
        << dx << "  catch (dunedaq::conffwk::Exception & ex) {\n"
        << dx << "    throw_init_ex(ex);\n"
        << dx << "  }\n";

    }

  cpp_s << dx << "}\n\n";


    // destructor

  cpp_s
    << dx << name << "::~" << name << "() noexcept\n"
    << dx << "{\n"
    << dx << "}\n\n";

  cpp_s
    << dx << "std::vector<const dunedaq::conffwk::DalObject *> " << name << "::get(const std::string& name, bool upcast_unregistered) const\n"
    << dx << "{\n"
    << dx << "  std::vector<const dunedaq::conffwk::DalObject *> vec;\n\n"
    << dx << "  if (!get(name, vec, upcast_unregistered, true))\n"
    << dx << "    throw_get_ex(name, s_class_name, this);\n\n"
    << dx << "  return vec;\n"
    << dx << "}\n\n";

  cpp_s
    << dx << "bool " << name << "::get(const std::string& name, std::vector<const dunedaq::conffwk::DalObject *>& vec, bool upcast_unregistered, bool first_call) const\n"
    << dx << "{\n"
    << dx << "  if (first_call)\n"
    << dx << "    {\n"
    << dx << "      std::lock_guard scoped_lock(m_mutex);\n\n"
    << dx << "      check();\n"
    << dx << "      check_init();\n\n"
    << dx << "      if (get_rel_objects(name, upcast_unregistered, vec))\n"
    << dx << "        return true;\n"
    << dx << "    }\n\n";


  for (const auto &prototype : algo_n_set)
    cpp_s
      << dx << "  if (name == \"" << prototype << "()\")\n"
      << dx << "    {\n"
      << dx << "      p_registry.downcast_dal_objects(" << prototype << "(), upcast_unregistered, vec);\n"
      << dx << "      return true;\n"
      << dx << "    }\n\n";

  for (const auto &prototype : algo_1_set)
    cpp_s
      << dx << "  if (name == \"" << prototype << "()\")\n"
      << dx << "    {\n"
      << dx << "      p_registry.downcast_dal_object(" << prototype << "(), upcast_unregistered, vec);\n"
      << dx << "      return true;\n"
      << dx << "    }\n\n";


  if (const std::list<std::string*> * slist = cl->direct_super_classes())
    {
      for (const auto& i : *slist)
        {
          cpp_s << dx << "  if (" << alnum_name(*i) << "::get(name, vec, upcast_unregistered, false)) return true;\n";
        }

      cpp_s << "\n";
    }

  cpp_s
    << dx << "  if (first_call)\n"
    << dx << "    return get_algo_objects(name, vec);\n\n"
    << dx << "  return false;\n"
    << dx << "}\n\n";


      // generate relationship set methods

  if (const std::list<oks::OksRelationship*> *rlist = cl->direct_relationships())
    {
      for (const auto& i : *rlist)
        {
          const std::string rname(alnum_name(i->get_name()));
          std::string full_cpp_class_name = get_full_cpp_class_name(i->get_class_type(), cl_info, cpp_ns_name);

          cpp_s << dx << "void " << name << "::set_" << rname << "(const ";

          if (i->get_high_cardinality_constraint() == oks::OksRelationship::Many)
            {
              cpp_s
                << "std::vector<const " << full_cpp_class_name << "*>& value)\n"
                << dx << "{\n"
                << dx << "  _set_objects(s_" << rname << ", value);\n"
                << dx << "}\n\n";
            }
          else
            {
              cpp_s
                << full_cpp_class_name << " * value)\n"
                << dx << "{\n"
                << dx << "  _set_object(s_" << rname << ", value);\n"
                << dx << "}\n\n";
            }
        }
    }


    // generate methods for c++

  if (const std::list<oks::OksMethod*> * mlist = cl->direct_methods())
    {
      bool comment_is_printed = false;
      for (const auto& i : *mlist)
        {
          oks::OksMethodImplementation * mi = find_cpp_method_implementation(i);

          if (mi && !get_method_implementation_body(mi).empty())
            {
              if (comment_is_printed == false)
                {
                  cpp_s << dx << "    // user-defined algorithms\n\n";
                  comment_is_printed = true;
                }

              // generate description

              print_description(cpp_s, i->get_description(), dx2);

              // generate prototype

              std::string prototype(mi->get_prototype());
              std::string::size_type idx = prototype.find('(');

              if (idx != std::string::npos)
                {
                  idx--;

                  // skip spaces between method name and ()

                  while (isspace(prototype[idx]) && idx > 0)
                    idx--;

                  // find beginning of the method name

                  while ((isalnum(prototype[idx]) || prototype[idx] == '_') && idx > 0)
                    idx--;

                  prototype[idx] = '\n';
                  std::string s(dx);
                  s += alnum_name(name);
                  s += "::";
                  prototype.insert(idx + 1, s);
                }

              // Second pass
              std::string::size_type idx_open_bracket = prototype.find('(');
              std::string::size_type idx_space = prototype.rfind(' ', idx_open_bracket);
              std::string::size_type idx_close_bracket = prototype.rfind(')');
            
              auto prototype_attrs = prototype.substr(0,idx_space+1);
              auto prototype_head = prototype.substr(idx_space+1,idx_close_bracket-idx_space);
              auto prototype_specs = prototype.substr(idx_close_bracket+1);

              for ( const auto& spec : cpp_method_virtual_specifiers ) {
                  idx = prototype_attrs.find(spec);
                  if ( idx != std::string::npos) {
                      prototype_attrs.erase(idx,spec.size());
                  }
                  prototype_attrs = trim(prototype_attrs);
                                    
                  idx = prototype_specs.find(spec);
                  if ( idx != std::string::npos) {
                      prototype_specs.erase(idx,spec.size());
                  }
                  prototype_specs = trim(prototype_specs);
              }

              prototype = prototype_attrs + ' ' + prototype_head + ' ' + prototype_specs;

              cpp_s
                << dx << prototype << std::endl
                << dx << "{\n"
                << get_method_implementation_body(mi) << std::endl
                << dx << "}\n\n";
            }
        }
    }


    // close namespace

  close_cpp_namespace(cpp_s, ns_level);
}

static void
load_schemas(oks::OksKernel& kernel, const std::list<std::string>& file_names, std::set<oks::OksFile *, std::less<oks::OksFile *> >& file_hs)
{
  for (const auto& i : file_names)
    {
      if (oks::OksFile * fh = kernel.load_schema(i))
        {
          file_hs.insert(fh);
        }
      else
        {
          std::cerr << "ERROR: can not load schema file \"" << i << "\"\n";
          exit(EXIT_FAILURE);
        }
    }
}

static void
gen_cpp_header_prologue(const std::string& file_name,
                        std::ostream& s,
			const std::string& cpp_ns_name,
			const std::string& cpp_hdr_dir)
{
  s <<
    "// *** this file is generated by oksdalgen, do not modify it ***\n\n"

    "#ifndef _" << alnum_name(file_name) << "_0_" << alnum_name(cpp_ns_name) << "_0_" << alnum_name(cpp_hdr_dir) << "_H_\n"
    "#define _" << alnum_name(file_name) << "_0_" << alnum_name(cpp_ns_name) << "_0_" << alnum_name(cpp_hdr_dir) << "_H_\n\n"

    "#include <stdint.h>   // to define 64 bits types\n"
    "#include <iostream>\n"
    "#include <sstream>\n"
    "#include <string>\n"
    "#include <map>\n"
    "#include <vector>\n\n"

    "#include \"conffwk/Configuration.hpp\"\n"
    "#include \"conffwk/DalObject.hpp\"\n\n";
}


static void
gen_cpp_header_epilogue(std::ostream& s)
{
  s << "\n#endif\n";
}


static void
gen_cpp_body_prologue(const std::string& file_name,
                      std::ostream& src,
                      const std::string& cpp_hdr_dir)
{
  src <<
    "#include \"conffwk/ConfigObject.hpp\"\n"
    "#include \"conffwk/DalFactory.hpp\"\n"
    "#include \"conffwk/DalObjectPrint.hpp\"\n"
    "#include \"conffwk/Errors.hpp\"\n"
    "#include \"";
    if(cpp_hdr_dir != "") {
      src <<  cpp_hdr_dir << "/";
    }
    src << file_name << ".hpp\"\n\n";
}


int
main(int argc, char *argv[])
{
  std::list<std::string> class_names;
  std::list<std::string> file_names;
  std::list<std::string> include_dirs;
  std::list<std::string> user_classes;

  std::string cpp_dir_name = ".";                // directory for c++ implementation files
  std::string cpp_hdr_dir = "";                  // directory for c++ header files
  std::string cpp_ns_name = "";                  // c++ namespace
  std::string info_file_name = "oksdalgen.info"; // name of info file
  bool verbose = false;

  parse_arguments(argc, argv, class_names, file_names, include_dirs, user_classes, cpp_dir_name, cpp_ns_name, cpp_hdr_dir, info_file_name, verbose);

  // init OKS

  std::set<oks::OksFile *, std::less<oks::OksFile *> > file_hs;

  oks::OksKernel kernel(false, false, false, false);

  try
    {
      load_schemas(kernel, file_names, file_hs);

      // if no user defined classes, generate all

      if (class_names.size() == 0)
        {
          if (verbose)
            {
              std::cout << "No explicit classes were provided,\n"
                  "search for classes which belong to the given schema files:\n";
            }

          const oks::OksClass::Map& class_list = kernel.classes();

          if (!class_list.empty())
            {
              for (const auto& i : class_list)
                {
                  if (verbose)
                    {
                      std::cout << " * check for class \"" << i.first << "\" ";
                    }

                  if (file_hs.find(i.second->get_file()) != file_hs.end())
                    {
                      class_names.push_back(i.second->get_name());
                      if (verbose)
                        {
                          std::cout << "OK\n";
                        }
                    }
                  else
                    {
                      if (verbose)
                        {
                          std::cout << "skip\n";
                        }
                    }
                }
            }
          else
            {
              std::cerr << "No classes in schema file and no user classes specified\n";
              exit(EXIT_FAILURE);
            }
        }

      // calculate set of classes which should be mentioned by the generator;
      // note, some of classes can come from other DALs

      typedef std::set<const oks::OksClass *, std::less<const oks::OksClass *> > Set;
      Set generated_classes;
      ClassInfo::Map cl_info;

      unsigned int error_num = 0;

      // build set of classes which are generated

      for (const auto& i : class_names)
        {
          if (oks::OksClass *cl = kernel.find_class(i))
            {
              generated_classes.insert(cl);
            }
          else
            {
              std::cerr << "ERROR: can not find class " << i << std::endl;
              error_num++;
            }
        }

      // build set of classes which are external to generated

      for (const auto& i : generated_classes)
        {
          if (const std::list<oks::OksRelationship *> * rels = i->direct_relationships())
            {
              for (const auto& j : *rels)
                {
                  const oks::OksClass * rc = j->get_class_type();

                  if (rc == 0)
                    {
                      std::cerr << "\nERROR: the class \"" << j->get_type() << "\" (it is used by the relationship \"" << j->get_name() << "\" of class \"" << i->get_name() << "\") is not defined by the schema.\n";
                      error_num++;
                    }
                  else if (generated_classes.find(rc) == generated_classes.end())
                    {
                      if (process_external_class(cl_info, rc, include_dirs, user_classes, verbose) == false)
                        {
                          std::cerr << "\nERROR: the class \"" << j->get_type() << "\" is used by the relationship \"" << j->get_name() << "\" of class \"" << i->get_name() << "\".\n"
                              "       The class is not in the list of generated classes, "
                              "it was not found among already generated headers "
                              "(search list is defined by the -I | --include-dirs parameter) and "
                              "it is not defined by user via -D | --user-defined-classes.\n";
                          error_num++;
                        }
                    }
                }
            }

          if (const std::list<std::string *> * sclasses = i->direct_super_classes())
            {
              for (const auto& j : *sclasses)
                {
                  const oks::OksClass * rc = kernel.find_class(*j);

                  if (rc == 0)
                    {
                      std::cerr << "\nERROR: the class \"" << *j << "\" (it is direct superclass of class \"" << i->get_name() << "\") is not defined by the schema.\n";
                      error_num++;
                    }
                  else if (generated_classes.find(rc) == generated_classes.end())
                    {
                      if (process_external_class(cl_info, rc, include_dirs, user_classes, verbose) == false)
                        {
                          std::cerr << "\nERROR: the class \"" << rc->get_name() << "\" is direct superclass of class \"" << i->get_name() << "\".\n"
                              "       The class is not in the list of generated classes, "
                              "it was not found among already generated headers "
                              "(search list is defined by the -I | --include-dirs parameter) and "
                              "it is not defined by user via -D | --user-defined-classes.\n";
                          error_num++;
                        }
                    }
                }
            }
        }

      if (error_num != 0)
        {
          std::cerr << "\n*** " << error_num << (error_num == 1 ? " error was" : " errors were") << " found.\n\n";
          return (EXIT_FAILURE);
        }

      for (const auto& cl : generated_classes)
        {
          std::string name(alnum_name(cl->get_name()));

          std::string cpp_hdr_name = cpp_dir_name + "/" + name + ".hpp";
          std::string cpp_src_name = cpp_dir_name + "/" + name + ".cpp";

          std::ofstream cpp_hdr_file(cpp_hdr_name.c_str());
          std::ofstream cpp_src_file(cpp_src_name.c_str());

          if (!cpp_hdr_file)
            {
              std::cerr << "ERROR: can not create file \"" << cpp_hdr_name << "\"\n";
              return (EXIT_FAILURE);
            }

          if (!cpp_src_file)
            {
              std::cerr << "ERROR: can not create file \"" << cpp_src_name << "\"\n";
              return (EXIT_FAILURE);
            }

          gen_cpp_header_prologue(name, cpp_hdr_file, cpp_ns_name, cpp_hdr_dir);
          gen_cpp_body_prologue(name, cpp_src_file, cpp_hdr_dir);

          gen_header(cl, cpp_hdr_file, cpp_ns_name, cpp_hdr_dir, cl_info);
          gen_cpp_body(cl, cpp_src_file, cpp_ns_name, cpp_hdr_dir, cl_info);

          gen_cpp_header_epilogue(cpp_hdr_file);
        }

      // generate dump applications

        {
          struct ConfigurationImplementations
          {
            const char * name;
            const char * header;
            const char * class_name;
            const char * header_prologue;
            const char * main_function_prologue;
          } confs[] =
            {
              { 0, "conffwk/Configuration.hpp", 0, "", "" } };

          for (unsigned int i = 0; i < sizeof(confs) / sizeof(ConfigurationImplementations); ++i)
            {
              std::string dump_name = cpp_dir_name + "/dump";
              if (!cpp_ns_name.empty())
                {
                  dump_name += '_';
                  dump_name += alnum_name(cpp_ns_name);
                }
              if (confs[i].name)
                {
                  dump_name += '_';
                  dump_name += confs[i].name;
                }
              dump_name += ".cpp";

              std::ofstream dmp(dump_name.c_str());

              dmp.exceptions(std::ostream::failbit | std::ostream::badbit);

              if (dmp)
                {
                  try
                    {
                      gen_dump_application(dmp, class_names, cpp_ns_name, cpp_hdr_dir, confs[i].header, confs[i].class_name, confs[i].header_prologue, confs[i].main_function_prologue);
                    }
                  catch (std::exception& ex)
                    {
                      std::cerr << "ERROR: can not create file \"" << dump_name << "\": " << ex.what() << std::endl;
                    }
                }
              else
                {
                  std::cerr << "ERROR: can not create file \"" << dump_name << "\"\n";
                  return (EXIT_FAILURE);
                }
            }
        }

      // generate info file

        {
          std::ofstream info(info_file_name.c_str());

          if (info)
            {
              write_info_file(info, cpp_ns_name, cpp_hdr_dir, generated_classes);
            }
          else
            {
              std::cerr << "ERROR: can not create file \"" << info_file_name << "\"\n";
              return (EXIT_FAILURE);
            }
        }

    }
  catch (oks::exception & ex)
    {
      std::cerr << "Caught oks exception:\n" << ex << std::endl;
      return (EXIT_FAILURE);
    }
  catch (std::exception & e)
    {
      std::cerr << "Caught standard C++ exception: " << e.what() << std::endl;
      return (EXIT_FAILURE);
    }
  catch (...)
    {
      std::cerr << "Caught unknown exception" << std::endl;
      return (EXIT_FAILURE);
    }

  return (EXIT_SUCCESS);
}


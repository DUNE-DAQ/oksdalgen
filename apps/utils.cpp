
#include "class_info.hpp"

#include "oks/kernel.hpp"
#include "oks/method.hpp"

#include <ctype.h>

#include <algorithm>
#include <ctime>
#include <initializer_list>
#include <iomanip>
#include <iostream>


using namespace dunedaq::oks;
using namespace dunedaq::oksdalgen;

  /**
   *  The functions cvt_symbol() and alnum_name() are used
   *  to replace all symbols allowed in names of classes, attributes
   *  and relationships, but misinterpreted by c++.
   */

char
cvt_symbol(char c)
{
  return (isalnum(c) ? c : '_');
}

std::string
alnum_name(const std::string& in)
{
  std::string s(in);
  std::transform(s.begin(), s.end(), s.begin(), cvt_symbol);
  return s;
}

std::string
capitalize_name(const std::string& in)
{
  std::string s(in);
  if (!in.empty())
    {
      if (isalpha(in[0]))
        {
          if (islower(in[0]))
            s[0] = toupper(in[0]);
        }
      else
        {
          s.insert(s.begin(), '_');
        }
    }
  return s;
}


  /**
   *  The function print_description() is used for printing of the
   *  multi-line text with right alignment.
   */

void
print_description(std::ostream& s, const std::string& text, const char * dx)
{
  if (!text.empty())
    {
      s << dx << " /**\n";

      Oks::Tokenizer t(text, "\n");
      std::string token;

      while (!(token = t.next()).empty())
        {
          s << dx << "  *  " << token << std::endl;
        }

      s << dx << "  */\n\n";
    }
}

void
print_indented(std::ostream& s, const std::string& text, const char * dx)
{
  if (!text.empty())
    {
      Oks::Tokenizer t(text, "\n");
      std::string token;

      while (!(token = t.next()).empty())
        {
          s << dx << token << std::endl;
        }
    }
}

std::string
get_type(OksData::Type oks_type, bool is_cpp)
{
  switch (oks_type)
    {
      case OksData::unknown_type:
        return "UNKNOWN";
      case OksData::s8_int_type:
        return (is_cpp ? "int8_t" : "char");
      case OksData::u8_int_type:
        return (is_cpp ? "uint8_t" : "byte");
      case OksData::s16_int_type:
        return (is_cpp ? "int16_t" : "short");
      case OksData::u16_int_type:
        return (is_cpp ? "uint16_t" : "short");
      case OksData::s32_int_type:
        return (is_cpp ? "int32_t" : "int");
      case OksData::u32_int_type:
        return (is_cpp ? "uint32_t" : "int");
      case OksData::s64_int_type:
        return (is_cpp ? "int64_t" : "long");
      case OksData::u64_int_type:
        return (is_cpp ? "uint64_t" : "long");
      case OksData::float_type:
        return "float";
      case OksData::double_type:
        return "double";
      case OksData::bool_type:
        return (is_cpp ? "bool" : "boolean");

      case OksData::string_type:
      case OksData::enum_type:
      case OksData::date_type:
      case OksData::time_type:
      case OksData::class_type:
        return (is_cpp ? "std::string" : "String");

      case OksData::object_type:
      case OksData::list_type:
      case OksData::uid_type:
      case OksData::uid2_type:
        return "funnytype";
    }

  return "invalid";
}

// std::string
// get_java_impl_name(const std::string& s)
// {
//   std::string str(s);
//   str += "_Impl";
//   return str;
// }

// std::string
// get_java_helper_name(const std::string& s)
// {
//   std::string str(s);
//   str += "_Helper";
//   return str;
// }

void
gen_dump_application(std::ostream& s,
                     std::list<std::string>& class_names,
		     const std::string& cpp_ns_name,
		     const std::string& cpp_hdr_dir,
		     const char * conf_header,
		     const char * conf_name,
		     const char * headres_prologue,
		     const char * main_function_prologue
		     )
{
  s <<
    "  // *** this file is generated by oksdalgen ***\n"
    "\n"
    "#include \"conffwk/ConfigObject.hpp\"\n"
    "#include \"" << conf_header << "\"\n\n";

  // generate list of includes
  for (const auto& i : class_names)
    {
      std::string hname(cpp_hdr_dir);
      if (!hname.empty())
        hname += '/';
      hname += i;
      s << "#include \"" << hname << ".hpp\"\n";
    }

  if (headres_prologue && *headres_prologue)
    {
      s << "\n  // db implementation-specific headrers prologue\n\n" << headres_prologue << "\n";
    }

  s <<
      "\n"
      "\n"
      "static void usage(const char * s)\n"
      "{\n"
      "  std::cout << s << \" -d db-name -c class-name [-q query | -i object-id] [-t]\\n\"\n"
      "    \"\\n\"\n"
      "    \"Options/Arguments:\\n\"\n"
      "    \"  -d | --data db-name            mandatory name of the database\\n\"\n"
      "    \"  -c | --class-name class-name   mandatory name of class\\n\"\n"
      "    \"  -q | --query query             optional query to select class objects\\n\"\n"
      "    \"  -i | --object-id object-id     optional identity to select one object\\n\"\n"
      "    \"  -t | --init-children           all referenced objects are initialized (is used\\n\"\n"
      "    \"                                 for debug purposes and performance measurements)\\n\"\n"
      "    \"  -h | --help                    print this message\\n\"\n"
      "    \"\\n\"\n"
      "    \"Description:\\n\"\n"
      "    \"  The program prints out object(s) of given class.\\n\"\n"
      "    \"  If no query or object id is provided, all objects of the class are printed.\\n\"\n"
      "    \"  It is automatically generated by oksdalgen utility.\\n\"\n"
      "    \"\\n\";\n"
      "}\n"
      "\n"
      "static void no_param(const char * s)\n"
      "{\n"
      "  std::cerr << \"ERROR: the required argument for option \\\'\" << s << \"\\\' is missing\\n\\n\";\n"
      "  exit (EXIT_FAILURE);\n"
      "}\n"
      "\n"
      "int main(int argc, char *argv[])\n"
      "{\n";

  if (main_function_prologue && *main_function_prologue)
    {
      s << "    // db implementation-specific main function prologue\n\n  " << main_function_prologue << "\n\n";
    }

  s <<
      "    // parse parameters\n"
      "\n"
      "  const char * db_name = nullptr;\n"
      "  const char * object_id = nullptr;\n"
      "  const char * query = \"\";\n"
      "  std::string class_name;\n"
      "  bool init_children = false;\n"
      "\n"
      "  for(int i = 1; i < argc; i++) {\n"
      "    const char * cp = argv[i];\n"
      "    if(!strcmp(cp, \"-h\") || !strcmp(cp, \"--help\")) {\n"
      "      usage(argv[0]);\n"
      "      return 0;\n"
      "    }\n"
      "    if(!strcmp(cp, \"-t\") || !strcmp(cp, \"--init-children\")) {\n"
      "      init_children = true;\n"
      "    }\n"
      "    else if(!strcmp(cp, \"-d\") || !strcmp(cp, \"--data\")) {\n"
      "      if(++i == argc || argv[i][0] == '-') { no_param(cp); } else { db_name = argv[i]; }\n"
      "    }\n"
      "    else if(!strcmp(cp, \"-c\") || !strcmp(cp, \"--class-name\")) {\n"
      "      if(++i == argc || argv[i][0] == '-') { no_param(cp); } else { class_name = argv[i]; }\n"
      "    }\n"
      "    else if(!strcmp(cp, \"-i\") || !strcmp(cp, \"--object-id\")) {\n"
      "      if(++i == argc || argv[i][0] == '-') { no_param(cp); } else { object_id = argv[i]; }\n"
      "    }\n"
      "    else if(!strcmp(cp, \"-q\") || !strcmp(cp, \"--query\")) {\n"
      "      if(++i == argc || argv[i][0] == '-') { no_param(cp); } else { query = argv[i]; }\n"
      "    }\n"
      "    else {\n"
      "      std::cerr << \"ERROR: bad parameter \" << cp << std::endl;\n"
      "      usage(argv[0]);\n"
      "      return (EXIT_FAILURE);\n"
      "    }\n"
      "  }\n"
      "\n"
      "  if(db_name == nullptr) {\n"
      "    std::cerr << \"ERROR: no database name provided\\n\";\n"
      "    return (EXIT_FAILURE);\n"
      "  }\n"
      "\n"
      "  if(class_name.empty()) {\n"
      "    std::cerr << \"ERROR: no class name provided\\n\";\n"
      "    return (EXIT_FAILURE);\n"
      "  }\n"
      "\n"
      "  if(*query != 0 && object_id != nullptr) {\n"
      "    std::cerr << \"ERROR: only one parameter -i or -q can be provided\\n\";\n"
      "    return (EXIT_FAILURE);\n"
      "  }\n"
      "\n"
      "\n"
      "std::cout << std::boolalpha;\n"
      "\n";

  if (conf_name)
    {
      s <<
          "  " << conf_name << " impl_conf;\n"
          "\n"
          "  {\n"
          "    dunedaq::conffwk::Configuration conf(db_name, &impl_conf);\n";
    }
  else
    {
      s <<
          "  try {\n"
          "    dunedaq::conffwk::Configuration conf(db_name);\n";
    }

  s <<
      "\n"
      "    if(!conf.loaded()) {\n"
      "      std::cerr << \"Can not load database: \" << db_name << std::endl;\n"
      "      return (EXIT_FAILURE);\n"
      "    }\n"
      "    \n"
      "    std::vector< dunedaq::conffwk::ConfigObject > objects;\n"
      "    \n"
      "    if(object_id) {\n"
      "      dunedaq::conffwk::ConfigObject obj;\n"
      "      try {\n"
      "        conf.get(class_name, object_id, obj, 1);\n"
      "      }\n"
      "      catch (dunedaq::conffwk::NotFound & ex) {\n"
      "        std::cerr << \"Can not get object \\'\" << object_id << \"\\' of class \\'\" << class_name << \"\\':\\n\" << ex << std::endl;\n"
      "        return (EXIT_FAILURE);\n"
      "      }\n"
      "      objects.push_back(obj);\n"
      "    }\n"
      "    else {\n"
      "      try {\n"
      "        conf.get(class_name, objects, query, 1);\n"
      "      }\n"
      "      catch (dunedaq::conffwk::NotFound & ex) {\n"
      "        std::cerr << \"Can not get objects of class \\'\" << class_name << \"\\':\\n\" << ex << std::endl;\n"
      "        return (EXIT_FAILURE);\n"
      "      }\n"
      "    }\n"
      "    \n"
      "    struct SortByUId {\n"
      "      bool operator() (const dunedaq::conffwk::ConfigObject * o1, const dunedaq::conffwk::ConfigObject * o2) const {\n"
      "        return (o1->UID() < o2->UID());\n"
      "      };\n"
      "    };\n"
      "    \n"
      "    std::set< dunedaq::conffwk::ConfigObject *, SortByUId > sorted_objects;\n"
      "    \n"
      "    for(auto& i : objects)\n"
      "      sorted_objects.insert(&i);\n"
      "    \n"
      "    for(auto& i : sorted_objects) {\n";

  for (std::list<std::string>::iterator i = class_names.begin(); i != class_names.end(); ++i)
    {
      const char * op = (i == class_names.begin() ? "if" : "else if");
      std::string cname(cpp_ns_name);

      if (!cname.empty())
        cname += "::";

      cname += *i;

      s <<
          "      " << op << "(class_name == \"" << *i << "\") {\n"
          "        std::cout << *conf.get<" << cname << ">(*i, init_children) << std::endl;\n"
          "      }\n";
    }

  s <<
      "      else {\n"
      "        std::cerr << \"ERROR: do not know how to dump object of \" << class_name << \" class\\n\";\n"
      "        return (EXIT_FAILURE);\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "  catch (dunedaq::conffwk::Exception & ex) {\n"
      "    std::cerr << \"Caught \" << ex << std::endl;\n"
      "    return (EXIT_FAILURE);\n"
      "  }\n"
      "\n"
      "  return 0;\n"
      "}\n";
}


void
write_info_file(std::ostream& s,
                const std::string& cpp_namespace,
                const std::string& cpp_header_dir,
                // const std::string& java_pname,
                const std::set<const OksClass *, std::less<const OksClass *> >& classes)
{
  std::time_t now = std::time(nullptr);

  s << "// the file is generated " << std::put_time(std::localtime(&now), "%F %T %Z") << " by oksdalgen utility\n"
      "// *** do not modify the file ***\n"
      "c++-namespace=" << cpp_namespace << "\n"
      "c++-header-dir-prefix=" << cpp_header_dir << "\n"
      // "java-package-name=" << java_pname << "\n"
      "classes:\n";

  for (const auto& i : classes)
    s << "  " << i->get_name() << std::endl;
}


  /**
   *  The function get_full_cpp_class_name() returns name of class
   *  with it's namespace (e.g. "NAMESPACE_A::CLASS_X")
   */

std::string
get_full_cpp_class_name(const OksClass * c, const ClassInfo::Map& cl_info, const std::string & cpp_ns_name)
{
  std::string s;
  ClassInfo::Map::const_iterator idx = cl_info.find(c);

  if (idx != cl_info.end())
    s = (*idx).second.get_namespace();
  else
    s = cpp_ns_name;

  if (!s.empty())
    s += "::";

  s += alnum_name(c->get_name());

  return s;
}


std::string
get_include_dir(const OksClass * c, const ClassInfo::Map& cl_info, const std::string& cpp_hdr_dir)
{
  std::string s;

  ClassInfo::Map::const_iterator idx = cl_info.find(c);

  if (idx != cl_info.end())
    {
      const std::string include_prefix = (*idx).second.get_include_prefix();
      if (!include_prefix.empty())
        {
          s = include_prefix;
          s += '/';
        }
    }
  else if (!cpp_hdr_dir.empty())
    {
      s = cpp_hdr_dir;
      s += '/';
    }

  s += alnum_name(c->get_name());

  return s;
}


// const std::string&
// get_package_name(const OksClass * c, const ClassInfo::Map& cl_info, const std::string& java_p_name)
// {
//   std::string s;

//   ClassInfo::Map::const_iterator idx = cl_info.find(c);

//   if (idx != cl_info.end())
//     {
//       return (*idx).second.get_package_name();
//     }
//   else
//     {
//       return java_p_name;
//     }
// }

bool
process_external_class(
  ClassInfo::Map& cl_info,
  const OksClass * c,
  const std::list<std::string>& include_dirs,
  const std::list<std::string>& user_classes,
  bool verbose)
{
  if (cl_info.find(c) != cl_info.end())
    return true;

  // process list of user-defined classes first

  for (const auto &s : user_classes)
    {
      std::string::size_type idx = s.find("::");
      std::string::size_type idx2 = s.find("@", idx + 2);

      std::string cpp_ns_name;
      std::string class_name;
      std::string cpp_dir_name;

      if (idx == std::string::npos)
        {
          class_name = s.substr(0, idx2);
        }
      else
        {
          cpp_ns_name = s.substr(0, idx);
          class_name = s.substr(idx + 2, idx2 - idx - 2);
        }

      if (idx2 != std::string::npos)
        cpp_dir_name = s.substr(idx2 + 1);

      if (class_name == c->get_name())
        {
          cl_info[c] = ClassInfo(cpp_ns_name, cpp_dir_name);
          if (verbose)
            {
              std::cout << " * class " << c->get_name() << " is defined by user in ";

              if (cpp_ns_name.empty())
                std::cout << "global namespace";
              else
                std::cout << "namespace \"" << cpp_ns_name << '\"';

              if (!cpp_dir_name.empty())
                std::cout << " with \"" << cpp_dir_name << "\" include prefix";

              std::cout << std::endl;
            }

          return true;
        }
    }

  // process oksdalgen files

  if (verbose)
    std::cout << "Looking for oksdalgen info files ...\n";

  bool found_class_declaration = false;

  for (const auto& i : include_dirs)
    {
      std::string file_name(i);
      file_name += "/oksdalgen.info";

      std::ifstream f(file_name.c_str());

      if (f)
        {
          if (verbose)
            std::cout << " *** found file \"" << file_name << "\" ***\n";

          std::string cpp_ns_name;
          std::string cpp_dir_name;
          // std::string java_pname;
          bool is_class = false;

          std::string s;
          while (std::getline(f, s))
            {
              if (s.find("//") != std::string::npos)
                {
                  if (verbose)
                    std::cout << " - skip comment \"" << s << "\"\n";
                }
              else if (s.find("classes:") != std::string::npos)
                {
                  if (verbose)
                    std::cout << " - found classes keyword\n";
                  is_class = true;
                }
              else
                {
                  const char s1[] = "c++-namespace=";
                  std::string::size_type idx = s.find(s1);
                  if (idx != std::string::npos)
                    {
                      cpp_ns_name = s.substr(sizeof(s1) - 1);
                      if (verbose)
                        std::cout << " - c++ namespace = \"" << cpp_ns_name << "\"\n";
                    }
                  else
                    {
                      const char s2[] = "c++-header-dir-prefix=";
                      std::string::size_type idx = s.find(s2);
                      if (idx != std::string::npos)
                        {
                          cpp_dir_name = s.substr(sizeof(s2) - 1);
                          if (verbose)
                            std::cout << " - c++ header dir prefix name = \"" << cpp_dir_name << "\"\n";
                        }
                      // else
                        // {
                        //   const char s3[] = "java-package-name=";
                        //   std::string::size_type idx = s.find(s3);
                        //   if (idx != std::string::npos)
                        //     {
                        //       java_pname = s.substr(sizeof(s3) - 1);
                        //       if (verbose)
                        //         std::cout << " - java package name = \"" << java_pname << "\"\n";
                        //     }
                          else if (is_class)
                            {
                              std::string cname = s.substr(2);
                              OksClass * cl = c->get_kernel()->find_class(cname);
                              if (cl && cl_info.find(cl) == cl_info.end())
                                {
                                  cl_info[cl] = ClassInfo(cpp_ns_name, cpp_dir_name);
                                  if (verbose)
                                    std::cout << " * class " << cl->get_name() << " is defined by " << file_name << " in namespace \"" << cpp_ns_name << "\" with include prefix \"" << cpp_dir_name << "\"\n";
                                  if (cl == c)
                                    found_class_declaration = true;
                                }
                              else if (verbose)
                                {
                                  std::cout << " - skip class \"" << cname << "\" declaration\n";
                                }
                            }
                          else
                            {
                              std::cerr << "Failed to parse line \"" << s << "\"\n";
                            }
                        // }
                    }
                }
            }
        }
      else if (verbose)
        {
          std::cout << " *** file \"" << file_name << "\" does not exist ***\n";
        }
    }

  return found_class_declaration;
}

std::string
int2dx(int level)
{
  return std::string(level * 2, ' ');
}

int
open_cpp_namespace(std::ostream& s, const std::string& value)
{
  int level = 0;
  std::string dx;

  if (!value.empty())
    {
      Oks::Tokenizer t(value, ":");
      std::string token;

      while (!(token = t.next()).empty())
        {
          s << dx << "namespace " << token << " {\n";
          level++;
          dx += "  ";
        }
    }

  return level;
}

void
close_cpp_namespace(std::ostream& s, int level)
{
  while (level > 0)
    {
      std::string dx = int2dx(--level);
      s << dx << "}\n";
    }
}



const std::string begin_header_prologue("BEGIN_HEADER_PROLOGUE");
const std::string end_header_prologue("END_HEADER_PROLOGUE");
const std::string begin_header_epilogue("BEGIN_HEADER_EPILOGUE");
const std::string end_header_epilogue("END_HEADER_EPILOGUE");
const std::string begin_public_section("BEGIN_PUBLIC_SECTION"); // => java interface
const std::string end_public_section("END_PUBLIC_SECTION");
const std::string begin_private_section("BEGIN_PRIVATE_SECTION"); // => c++ and java
const std::string end_private_section("END_PRIVATE_SECTION");
const std::string begin_member_initializer_list("BEGIN_MEMBER_INITIALIZER_LIST");
const std::string end_member_initializer_list("END_MEMBER_INITIALIZER_LIST");
const std::string has_add_algo_1("ADD_ALGO_1");
const std::string has_add_algo_n("ADD_ALGO_N");

static std::string
get_method_header_x_logue(OksMethodImplementation * mi, const std::string& begin, const std::string& end)
{
  std::string::size_type begix_idx = mi->get_body().find(begin, 0);
  if (begix_idx == std::string::npos)
    return "";
  std::string::size_type end_idx = mi->get_body().find(end, begix_idx + begin.size());
  if (end_idx == std::string::npos)
    return "";
  return mi->get_body().substr(begix_idx + begin.size(), end_idx - begix_idx - begin.size());
}

std::string
get_method_header_prologue(OksMethodImplementation * mi)
{
  return get_method_header_x_logue(mi, begin_header_prologue, end_header_prologue);
}

std::string
get_method_header_epilogue(OksMethodImplementation * mi)
{
  return get_method_header_x_logue(mi, begin_header_epilogue, end_header_epilogue);
}

std::string
get_public_section(OksMethodImplementation * mi)
{
  return get_method_header_x_logue(mi, begin_public_section, end_public_section);
}

std::string
get_private_section(OksMethodImplementation * mi)
{
  return get_method_header_x_logue(mi, begin_private_section, end_private_section);
}

std::string
get_member_initializer_list(OksMethodImplementation * mi)
{
  return get_method_header_x_logue(mi, begin_member_initializer_list, end_member_initializer_list);
}

bool
get_add_algo_1(OksMethodImplementation * mi)
{
  return (mi->get_body().find(has_add_algo_1, 0) != std::string::npos);
}

bool
get_add_algo_n(OksMethodImplementation * mi)
{
  return (mi->get_body().find(has_add_algo_n, 0) != std::string::npos);
}

static void
remove_string_section(std::string& s, const std::string& begin, const std::string& end)
{
  std::string::size_type begix_idx = s.find(begin, 0);
  if (begix_idx == std::string::npos)
    return;

  std::string::size_type end_idx = (end.empty() ? (begix_idx + begin.size()) : s.find(end, begix_idx + begin.size()));
  if (end_idx == std::string::npos)
    return;

  end_idx += end.size();
  while (s[end_idx] == '\n')
    end_idx++;

  s.erase(begix_idx, end_idx - begix_idx);
}

std::string
get_method_implementation_body(OksMethodImplementation * mi)
{
  std::string s = mi->get_body();
  remove_string_section(s, begin_header_prologue, end_header_prologue);
  remove_string_section(s, begin_header_epilogue, end_header_epilogue);
  remove_string_section(s, begin_public_section, end_public_section);
  remove_string_section(s, begin_private_section, end_private_section);
  remove_string_section(s, begin_member_initializer_list, end_member_initializer_list);
  remove_string_section(s, has_add_algo_1, "");
  remove_string_section(s, has_add_algo_n, "");

  if(std::all_of(s.begin(),s.end(),isspace))
    return "";
  else
    return s;
}

static OksMethodImplementation *
find_method_implementation(const OksMethod * method, std::initializer_list<std::string> names)
{
  for (const auto& name : names)
    {
      if (OksMethodImplementation * mi = method->find_implementation(name))
        if (mi->get_prototype().empty() == false)
          return mi;
    }

  return nullptr;
}

OksMethodImplementation *
find_cpp_method_implementation(const OksMethod * method)
{
  return find_method_implementation(method, { "c++", "C++" });
}


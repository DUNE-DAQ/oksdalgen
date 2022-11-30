#ifndef __GENCONFIG_CLASS_INFO__
#define __GENCONFIG_CLASS_INFO__

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <oks/class.h>

class ClassInfo {

  public:

    struct SortByName {
      bool operator() (const OksClass * c1, const OksClass * c2) const {
        return c1->get_name() < c2->get_name();
      }
    };

    typedef std::map<const OksClass *, ClassInfo, SortByName> Map;

    ClassInfo() {};

    ClassInfo(const std::string& cpp_ns_name, const std::string& dir_prefix, const std::string& package_name) :
      p_namespace (cpp_ns_name), p_include_prefix (dir_prefix), p_package_name(package_name) {};

    const std::string& get_namespace() const {return p_namespace;}
    const std::string& get_include_prefix() const {return p_include_prefix;}
    const std::string& get_package_name() const {return p_package_name;}


  private:

    std::string p_namespace;
    std::string p_include_prefix;
    std::string p_package_name;

};

struct NameSpaceInfo
{
  std::set<std::string> m_classes;
  std::map<std::string, NameSpaceInfo> m_nested;

  bool
  empty() const
  {
    return (m_classes.empty() && m_nested.empty());
  }

  void
  add(const std::string &ns_name, const std::string &class_name)
  {
    if (!ns_name.empty())
      {
        std::string::size_type idx = ns_name.find_first_of(':');
        NameSpaceInfo &ns = m_nested[ns_name.substr(0, idx)];
        if (idx != std::string::npos)
          ns.add(ns_name.substr(ns_name.find_first_not_of(':', idx)), class_name);
        else
          ns.add("", class_name);
      }
    else
      m_classes.insert(class_name);
  }

  void
  print(std::ostream &s, int level) const
  {
    std::string dx(level * 2, ' ');

    for (const auto &x : m_nested)
      {
        s << dx << "namespace " << x.first << " {\n";
        x.second.print(s, level + 1);
        s << dx << "}\n";
      }

    for (const auto &x : m_classes)
      s << dx << "class " << x << ";\n";
  }
};


#endif

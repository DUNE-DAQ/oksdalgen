#include <cstdlib>
#include <cstring>

#include <iostream>
#include <string>
#include <list>


static void
usage()
{
  std::cout <<
    "usage: genconfig [-d | --c++-dir-name directory-name]\n"
    "                 [-n | --c++-namespace namespace\n"
    "                 [-i | --c++-headers-dir directory-prefix]\n"
    "                 [-j | --java-dir-name directory-name]\n"
    "                 [-p | --java-package-name package-name]\n"
    "                 [-I | --include-dirs dirs*]\n"
    "                 [-c | --classes class*]\n"
    "                 [-D | --user-defined-classes [namespace::]user-class[@dir-prefix]*]\n"
    "                 [-f | --info-file-name file-name]\n"
    "                 [-v | --verbose]\n"
    "                 [-h | --help]\n"
    "                 -s | --schema-files file.schema.xml+\n"
    "\n"
    "Options/Arguments:\n"
    "       -d directory-name    name of directory for c++ header and implementation files\n"
    "       -n namespace         namespace for c++ classes\n"
    "       -i directory-prefix  name of directory prefix for c++ header files\n"
    "       -j directory-name    name of directory for java files\n"
    "       -p package-name      package name for java files\n"
    "       -I dirs*             directories where to search for already generated files\n"
    "       -c class*            explicit list of classes to be generated\n"
    "       -D [x::]c[@d]*       user-defined classes\n"
    "       -f filename          name of output file describing generated files\n"
    "       -v                   switch on verbose output\n"
    "       -h                   this message\n"
    "       -s files+            the schema files (at least one is mandatory)\n"
    "\n"
    "Description:\n"
    "       The utility generates c++ and java code for OKS schema files.\n\n";
}

static void
no_param(const char * s)
{
  std::cerr << "ERROR: the required argument for option \'" << s << "\' is missing\n\n";
  exit(EXIT_FAILURE);
}

void
parse_arguments(int argc, char *argv[], 
                std::list<std::string>& class_names, 
                std::list<std::string>& file_names,
                std::list<std::string>& include_dirs,
                std::list<std::string>& user_classes,
                std::string& cpp_dir_name,
                std::string& cpp_ns_name,
                std::string& cpp_hdr_dir,
		std::string& java_dir_name,
		std::string& java_pack_name,
		std::string& info_file_name,
		bool& verbose)
{
  for (int i = 1; i < argc; i++)
    {
      const char * cp = argv[i];

      if (!strcmp(cp, "-h") || !strcmp(cp, "--help"))
        {
          usage();
          exit(EXIT_SUCCESS);
        }
      else if (!strcmp(cp, "-v") || !strcmp(cp, "--verbose"))
        {
          verbose = true;
        }
      else if (!strcmp(cp, "-d") || !strcmp(cp, "--c++-dir-name"))
        {
          if (++i == argc || argv[i][0] == '-')
            no_param(cp);
          else
            cpp_dir_name = argv[i];
        }
      else if (!strcmp(cp, "-n") || !strcmp(cp, "--c++-namespace"))
        {
          if (++i == argc || argv[i][0] == '-')
            no_param(cp);
          else
            cpp_ns_name = argv[i];
        }
      else if (!strcmp(cp, "-i") || !strcmp(cp, "--c++-headers-dir"))
        {
          if (++i == argc || argv[i][0] == '-')
            no_param(cp);
          else
            cpp_hdr_dir = argv[i];
        }
      else if (!strcmp(cp, "-j") || !strcmp(cp, "--java-dir-name"))
        {
          if (++i == argc || argv[i][0] == '-')
            no_param(cp);
          else
            java_dir_name = argv[i];
        }
      else if (!strcmp(cp, "-p") || !strcmp(cp, "--java-package-name"))
        {
          if (++i == argc || argv[i][0] == '-')
            no_param(cp);
          else
            java_pack_name = argv[i];
        }
      else if (!strcmp(cp, "-f") || !strcmp(cp, "--info-file-name"))
        {
          if (++i == argc || argv[i][0] == '-')
            no_param(cp);
          else
            info_file_name = argv[i];
        }
      else
        {
          std::list<std::string> * slist = (
            (!strcmp(cp, "-c") || !strcmp(cp, "--classes"))              ? &class_names  :
            (!strcmp(cp, "-s") || !strcmp(cp, "--schema-files"))         ? &file_names   :
            (!strcmp(cp, "-I") || !strcmp(cp, "--include-dirs"))         ? &include_dirs :
            (!strcmp(cp, "-D") || !strcmp(cp, "--user-defined-classes")) ? &user_classes :
            nullptr
          );

          if (slist)
            {
              int j = 0;
              for (; j < argc - i - 1; ++j)
                {
                  if (argv[i + 1 + j][0] != '-')
                    slist->push_back(argv[i + 1 + j]);
                  else
                    break;
                }
              i += j;
            }
          else
            {
              std::cerr << "ERROR: Unexpected parameter: \"" << cp << "\"\n\n";
              usage();
              exit(EXIT_FAILURE);
            }
        }
    }

  if (file_names.size() == 0)
    {
      std::cerr << "At least one schema file is required\n";
      exit(EXIT_FAILURE);
    }

  if (verbose)
    {
      std::cout <<
          "VERBOSE:\n"
          "  Command line parameters:\n"
          "    c++ directory name:    \"" << cpp_dir_name << "\"\n"
          "    c++ namespace name:    \"" << cpp_ns_name << "\"\n"
          "    c++ headers directory: \"" << cpp_hdr_dir << "\"\n"
          "    java directory name:   \"" << java_dir_name << "\"\n"
          "    java package name:     \"" << java_pack_name << "\"\n"
          "    classes:";

      if (!class_names.empty())
        {
          std::cout << std::endl;

          for (const auto& i : class_names)
            std::cout << "      - " << i << std::endl;
        }
      else
        {
          std::cout << " no\n";
        }

      std::cout << "    file names:";

      if (!file_names.empty())
        {
          std::cout << std::endl;

          for (const auto& i : file_names)
            std::cout << "      * " << i << std::endl;
        }
      else
        {
          std::cout << " no\n";
        }

      std::cout << "    include directories for search:";

      if (!include_dirs.empty())
        {
          std::cout << std::endl;

          for (const auto& i : include_dirs)
            std::cout << "      * " << i << std::endl;
        }
      else
        {
          std::cout << " no\n";
        }

      std::cout << "    user-defined classes:";

      if (!user_classes.empty())
        {
          std::cout << std::endl;

          for (const auto& i : user_classes)
            std::cout << "      * " << i << std::endl;
        }
      else
        {
          std::cout << " no\n";
        }
    }
}

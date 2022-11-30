
tdaq_package()

tdaq_add_executable(genconfig 
  src/genconfig.cpp 
  src/parse_cmdl.cpp 
  src/utils.cpp 
  LINK_LIBRARIES oks)


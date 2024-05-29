cmake_minimum_required(VERSION 3.12)
project(oksdalgen VERSION 1.1.1)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(oks REQUIRED)
find_package(conffwk REQUIRED)

daq_add_application(oksdalgen oksdalgen.cxx *.cpp LINK_LIBRARIES oks::oks )

daq_install()

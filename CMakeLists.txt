cmake_minimum_required(VERSION 3.12)
project(genconfig VERSION 1.1.1)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(oks REQUIRED)
find_package(oksdbinterfaces REQUIRED)

daq_add_application(genconfig genconfig.cxx *.cpp LINK_LIBRARIES oks::oks )

daq_install()

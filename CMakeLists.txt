cmake_minimum_required(VERSION 3.12)
project(genconfig VERSION 0.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(oks REQUIRED)

daq_add_application(genconfig genconfig.cxx *.cpp LINK_LIBRARIES oks::oks )

daq_install()

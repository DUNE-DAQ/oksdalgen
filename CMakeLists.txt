# DUNE DAQ modification notice:
# This file has been modified from the original ATLAS oks source for the DUNE DAQ project.
# Fork baseline commit: genconfig-03-10-00 (2021-04-26).
# Renamed since fork: no.

cmake_minimum_required(VERSION 3.12)
project(oksdalgen VERSION 1.3.1)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(oks REQUIRED)
find_package(conffwk REQUIRED)

daq_add_application(oksdalgen oksdalgen.cxx *.cpp LINK_LIBRARIES oks::oks )

daq_install()

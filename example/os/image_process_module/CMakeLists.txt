# Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.

if(NOT DEFINED CMAKE_MINIMUM_REQUIRED_VERSION)
  cmake_minimum_required(VERSION 3.12)
  project(image_process_module)
  find_package(YARP REQUIRED COMPONENTS os sig)
endif()

add_executable(image_process_module)
target_sources(image_process_module PRIVATE image_process_module.cpp)
target_link_libraries(image_process_module PRIVATE YARP::YARP_os
                                                   YARP::YARP_init
                                                   YARP::YARP_sig)
set_property(TARGET image_process_module PROPERTY FOLDER "Examples/os")

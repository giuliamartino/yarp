# Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
# Copyright (C) 2006-2010 RobotCub Consortium
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.

# Make sure that YARP_DIR is set in your environment to wherever you
# compiled YARP.

cmake_minimum_required(VERSION 3.12)
project(example)

find_package(YARP COMPONENTS os REQUIRED)

add_executable(example)
target_sources(example PRIVATE main.cpp)
target_link_libraries(example PRIVATE ${YARP_LIBRARIES})

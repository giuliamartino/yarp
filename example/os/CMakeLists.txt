# Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.

foreach(example_name bottle_add
                     browse_bottle
                     buffered_port
                     database
                     image_process
                     image_process_module
                     image_source
                     internal_monologue
                     listports
                     make_count
                     playback
                     portable_pair
                     queue_manager
                     periodicthread
                     periodicThreadTiming
                     rpc_client
                     rpc_server
                     simple_receiver
                     simple_sender
                     summer
                     threads
                     view_count)

  if(YARP_COMPILE_EXAMPLES)
    add_subdirectory(${example_name})
  endif()

  if(YARP_ENABLE_EXAMPLES_AS_TESTS)
    add_test(NAME examples::os::${example_name}
             COMMAND ${CMAKE_CTEST_COMMAND} -VV
                                            --build-and-test "${CMAKE_CURRENT_SOURCE_DIR}/${example_name}"
                                                             "${CMAKE_CURRENT_BINARY_DIR}/tests/${example_name}"
                                            ${build_generator}
                                            --build-project  ${example_name}
                                            --build-options ${build_options}
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    set_tests_properties(examples::os::${example_name} PROPERTIES LABELS "examples::os")
    list(APPEND EXAMPLES_BUILD_DIRS "${CMAKE_CURRENT_BINARY_DIR}/tests/${example_name}")
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_CURRENT_BINARY_DIR}/tests/${example_name}")

  endif()

endforeach()

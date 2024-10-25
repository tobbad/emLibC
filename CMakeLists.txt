# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# project name and language
project(unit_test
    LANGUAGES C
    VERSION 1.0.0
    DESCRIPTION "Produce all tests"
    )

set(CMAKE_CXX_STANDARD 11 CACHE STRING "The C++ standard to use")
# example library
set(cut "common/src/common.c;common/src/device.c;common/src/memmgr_flsf.c;common/src/memory_device.c;common/src/packet.c;common/src/ringbuffer.c;common/src/simple_test.c;common/src/slip.c;common/src/stream.c")
add_library(cut STATIC "${cut}")
include_directories("common/inc/")
target_include_directories(cut PRIVATE "common/inc")
message(STATUS "cut  = ${cut}")

set(base "test/AllTests.cpp")
message(STATUS  "base = ${base}")

set(tests "common/test/device_test.cpp;common/test/memmgr_test.cpp;common/test/packet_test.cpp;common/test/ringbuffer_test.cpp;common/test/simple_test.cpp;common/test/slip_test.cpp")
message(STATUS  "tests = ${tests}")

foreach(test  ${tests})
  get_filename_component(name ${test} NAME_WLE)
  message(STATUS "ITEM is ${test}")
  string(REPLACE "_" ";" my_list ${name}) 
  list(LENGTH my_list len)
  list(GET 0 my_list basic)
  message(STATUS "my_list is ${my_list} has length ${len}")
  message(STATUS "Build test ${test} as ${name} or ${basic}")
  unset(name)
endforeach()
#target_link_libraries(ringbuffer cut)
# we will use the network to fetch Google Test sources
# make it possible to disable unit tests when not on network
option(ENABLE_UNIT_TESTS "Enable unit tests" ON)
message(STATUS "Enable testing: ${ENABLE_UNIT_TESTS}")
message(STATUS "Build shared lib: ${BUILD_SHARED_LIBS}")
if(ENABLE_UNIT_TESTS)
  # the following code to fetch googletest
  # is inspired by and adapted after:
  #   - https://cmake.org/cmake/help/v3.11/module/FetchContent.html
  include(FetchContent)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.8.0
  )

  FetchContent_GetProperties(googletest)

  if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)

    # Prevent GoogleTest from overriding our compiler/linker options
    # when building with Visual Studio
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # Prevent GoogleTest from using PThreads
    set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)

    # adds the targers: gtest, gtest_main, gmock, gmock_main
    add_subdirectory(
      ${googletest_SOURCE_DIR}
      ${googletest_BINARY_DIR}
      )

    # Silence std::tr1 warning on MSVC
    if(MSVC)
      foreach(_tgt gtest gtest_main gmock gmock_main)
        target_compile_definitions(${_tgt}
          PRIVATE
            "_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING"
          )
      endforeach()
    endif()
  endif()

 foreach(test ${tests})
    get_filename_component(my_name ${test} NAME_WLE)
    message(STATUS "Test is ${test}")
    string(REPLACE "_" ";" split ${my_name}) 
    message(STATUS "Name is ${my_name}")
    set(GET split 0 ${name} )
    message(STATUS "Program name ${name}")
    add_executable(${my_name} ${test})

 
    target_link_libraries(${my_name}
      PRIVATE
        ${cut}
        gtest_main
      )
    add_test(
      NAME ${my_name}
      COMMAND "${name}"
    )
      
    unset(my_name)
  ENDFOREACH()
  enable_testing()

 endif()

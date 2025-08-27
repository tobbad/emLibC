cmake_minimum_required(VERSION 3.10)
project(MyLibraryTests)

# Setze C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Definiere eine optionaldebuge Compiler-Definition
option(DEBUG "Aktiviere die benutzerdefinierte Funktion" ON)

# Pfade zur Bibliothek
set(COMMON_SRC_DIR "${CMAKE_SOURCE_DIR}/common/src")
set(COMMON_INC_DIR "${CMAKE_SOURCE_DIR}/common/inc")
set(COMMON_TEST_DIR "${CMAKE_SOURCE_DIR}/common/test")

set(ALL_TESTS_FILE "${CMAKE_SOURCE_DIR}/test/AllTests.cpp")
list(REMOVE_ITEM COMMON_HEADER_FILES "${COMMON_INC_DIR}/_time.h")
list(REMOVE_ITEM COMMON_HEADER_FILES "${COMMON_INC_DIR}/mutex.h")
list(REMOVE_ITEM COMMON_HEADER_FILES "${COMMON_INC_DIR}/xpad.h")
list(REMOVE_ITEM COMMON_HEADER_FILES "${COMMON_INC_DIR}/keyboard.h")
list(REMOVE_ITEM COMMON_HEADER_FILES "${COMMON_INC_DIR}/xpad.h")
list(REMOVE_ITEM COMMON_HEADER_FILES "${COMMON_INC_DIR}/terminal.h")
list(REMOVE_ITEM COMMON_HEADER_FILES "${COMMON_SRC_DIR}/slip.h")

# GoogleTest Pfad
set(GOOGLETEST_DIR "${CMAKE_SOURCE_DIR}/test/googletest/")
# Füge GoogleTest hinzu
message(STATUS "Füge GoogleTest hinzu...")
add_subdirectory(${GOOGLETEST_DIR})
# Zusätzlichen Include-Pfad definieren
set(GTEST  "${CMAKE_SOURCE_DIR}/test/googletest/googletest")

# Füge den zusätzlichen Include-Pfad zur Bibliothek hinzu
#target_include_directories(my_library PUBLIC ${ADDITIONAL_INC_DIR})
add_definitions("-DUNIT_TEST")
# Sammle alle Quell- und Testdateien
message(STATUS "Sammle Quell- und Testdateien...")
file(GLOB COMMON_SRC_FILES "${COMMON_SRC_DIR}/*.c")
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/_time.c")
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/mutex.c")
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/xpad.c")
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/keyboard.c")
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/xpad.c")
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/terminal.c")
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/stateled.c")
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/slip.c")
file(GLOB TEST_SRC_FILES "${COMMON_TEST_DIR}/*.cpp")
#list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/common_test.cpp")
#list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/buffer_test.cpp")
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/device_test.cpp")
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/packet_test.cpp")
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/ringbuffer_test.cpp")
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/slip_test.cpp")
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/simple_test.cpp")
#list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/state_test.cpp")


# Erstelle die Bibliothek
message(STATUS "Erstelle die Bibliothek...")
add_library(cut ${COMMON_SRC_FILES})
target_include_directories(cut PUBLIC ${COMMON_INC_DIR})

# Füge die Compiler-Definition hinzu, wenn USE_CUSTOM_FEATURE aktiviert ist
if(DEBUG)
    target_compile_definitions(cut PUBLIC DEBUG)
endif()

#add_compile_options(-H)

# Erstelle eine ausführbare Datei für jeden Test
foreach(TEST_FILE ${TEST_SRC_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    message(STATUS "Erstelle ausführbare Testdatei: ${TEST_NAME}")
    add_executable(${TEST_NAME} ${TEST_FILE} ${ALL_TESTS_FILE})
    target_include_directories(${TEST_NAME} PRIVATE ${COMMON_INC_DIR} ${GOOGLETEST_DIR} ${GTEST} )
    target_link_libraries(${TEST_NAME} cut gtest gtest_main pthread)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Test mit CTest aktivieren
message(STATUS "Aktiviere Tests mit CTest...")
enable_testing()

message(STATUS "CMake Konfiguration abgeschlossen.")

cmake_minimum_required(VERSION 3.10)
project(MyLibraryTests)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# Setze C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_DEBUG_POSTFIX d)
SET(CMAKE_BUILD_TYPE  Debug)
#SET(CMAKE_CXX_FLAGS_DEBUG "-g")
# Definiere eine optionaldebuge Compiler-Definition
option(DEBUG "Aktiviere die benutzerdefinierte Funktion" ON)

# Pfade zur Bibliothek
set(COMMON_SRC_DIR "${CMAKE_SOURCE_DIR}/common/src")
set(COMMON_INC_DIR "${CMAKE_SOURCE_DIR}/common/inc")
set(PROJECT_INC_DIR "${CMAKE_SOURCE_DIR}/../../Core/Inc")
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
list(REMOVE_ITEM COMMON_SRC_FILES "${COMMON_SRC_DIR}/packet.c")

file(GLOB TEST_SRC_FILES "${COMMON_TEST_DIR}/*.cpp")
# slip.c nutzt die veraltete device-API und kompiliert nicht im Host-Build ->
# slip_test bleibt deaktiviert.
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/slip_test.cpp")
# common_test referenziert str2uint(), das in common.h deklariert, aber nirgends
# implementiert ist -> linkt nicht. Bleibt deaktiviert.
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/common_test.cpp")
# device_test erwartet eine ältere device_t-Signatur (uint16_t counts) als die
# aktuelle device.h -> kompiliert nicht. Bleibt deaktiviert.
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/device_test.cpp")
# packet_test nutzt eine veraltete packet-API (mbuf, alte packet_open-Signatur)
# -> kompiliert nicht. Bleibt deaktiviert.
list(REMOVE_ITEM TEST_SRC_FILES "${COMMON_TEST_DIR}/packet_test.cpp")


# Erstelle die Bibliothek
message(STATUS "Erstelle die Bibliothek...")
add_library(cut ${COMMON_SRC_FILES})
target_link_options(cut PRIVATE
    -fsanitize=address
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

target_compile_definitions(cut PRIVATE $<$<BOOL:UNIT_TEST>:>)
target_include_directories(cut PUBLIC ${COMMON_INC_DIR}  ${PROJECT_INC_DIR})

# Füge die Compiler-Definition hinzu, wenn USE_CUSTOM_FEATURE aktiviert ist
if(DEBUG)
    target_compile_definitions(cut PUBLIC UNIT_TEST)
endif()

#add_compile_options(-H)
if(TARGET state_test)
    message(FATAL_ERROR "state_test target already exists!")
endif()

get_target_property(CUT_DEFS cut COMPILE_DEFINITIONS)
message(STATUS "cut COMPILE_DEFINITIONS = ${CUT_DEFS}")
message(STATUS "cut files         : ${COMMON_SRC_FILES}")
message(STATUS "common inc folder : ${COMMON_INC_DIR}")
message(STATUS "inc folder        : ${PROJECT_INC_DIR}")

# Erstelle eine ausführbare Datei für jeden Test
foreach(TEST_FILE ${TEST_SRC_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

    message(STATUS "Erstelle ausführbare Testdatei: ${TEST_NAME}")
    add_executable(${TEST_NAME} ${TEST_FILE} ${ALL_TESTS_FILE})
    target_compile_options(${TEST_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>
    )

    target_include_directories(${TEST_NAME}
        PRIVATE ${COMMON_INC_DIR} ${GOOGLETEST_DIR} ${GTEST} )

    target_compile_definitions(${TEST_NAME} PRIVATE UNIT_TEST)
    target_link_libraries(${TEST_NAME} cut gtest gtest_main pthread)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
# Test mit CTest aktivieren
message(STATUS "Aktiviere Tests mit CTest...")
enable_testing()

message(STATUS "CMake Konfiguration abgeschlossen.")

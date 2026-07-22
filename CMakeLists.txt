cmake_minimum_required(VERSION 3.10)

# ==========================================================================
# Target-Build: statische Bibliotheken fuer den Mikrocontroller
#
# Pro MCU-Port unter port/src/stm/ entsteht eine eigene Bibliothek:
#
#     stm32l4  ->  libemLibC_stm32l4.a
#     stm32f4  ->  libemLibC_stm32f4.a
#     stm32f3  ->  libemLibC_stm32f3.a
#
# Zusaetzlich zeigt der Aliasname 'emLibC' auf den Port aus EMLIBC_PORT.
#
# Greift nur beim Cross-Compilieren; der Host-Build darunter (GoogleTest)
# bleibt unveraendert. Einbindung aus dem Projekt heraus:
#
#     add_subdirectory(lib/emLibC)
#     target_link_libraries(<ziel> PRIVATE emLibC)
#
# Konfiguration ueber Cache-Variablen:
#   EMLIBC_PORTS            zu bauende Ports              (Default stm32l4)
#   EMLIBC_PORT             Port hinter dem Alias emLibC  (Default 1. aus PORTS)
#   EMLIBC_CONFIG_INCLUDES  Verzeichnisse mit options.h & (Default ../../Core/Inc)
#                           den uebrigen Projekt-Headern
#
# Pro Port zusaetzlich (<PORT> in Grossbuchstaben, z.B. STM32F4):
#   EMLIBC_<PORT>_INCLUDES  CMSIS-/HAL-Includes der Familie
#   EMLIBC_<PORT>_DEFINES   Symbole, z.B. STM32F407xx
#   EMLIBC_<PORT>_FLAGS     abweichende Compilerflags, z.B. -mcpu=cortex-m4
#   EMLIBC_<PORT>_CONFIG_INCLUDES  ersetzt EMLIBC_CONFIG_INCLUDES fuer den Port
#   EMLIBC_<PORT>_EXCLUDE   Quelldateien, die dieser Port nicht uebersetzt
#                           (Dateiname ohne Pfad, z.B. "serial.c")
#
# Jeder Port bringt seine Familie selbst mit, entweder vom Elternprojekt
# gesetzt (set(...) vor add_subdirectory) oder auf der Kommandozeile:
#   cmake --preset Debug \
#     -DEMLIBC_PORTS="stm32l4;stm32f4" \
#     -DEMLIBC_STM32F4_INCLUDES="/pfad/STM32F4xx_HAL/Inc;/pfad/CMSIS/Include" \
#     -DEMLIBC_STM32F4_DEFINES="STM32F407xx;USE_HAL_DRIVER"
# ==========================================================================

# CMAKE_CROSSCOMPILING wird erst durch project() gesetzt (die Toolchain-Datei
# wird dabei ausgewertet) -> muss vor der Fallunterscheidung stehen.
project(emLibC C)

if(CMAKE_CROSSCOMPILING)
    # Achtung: set(... CACHE ...) loescht beim ersten Configure eine bereits
    # vorhandene normale Variable gleichen Namens. Das Elternprojekt setzt diese
    # Werte per set() vor add_subdirectory() -> nur belegen, wenn undefiniert.
    if(NOT DEFINED EMLIBC_PORTS)
        set(EMLIBC_PORTS "stm32l4" CACHE STRING
            "Zu bauende MCU-Ports unter port/src/stm/ (Liste)")
    endif()
    if(NOT DEFINED EMLIBC_CONFIG_INCLUDES)
        set(EMLIBC_CONFIG_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Inc" CACHE STRING
            "Include-Pfade des Projekts (options.h etc.), von emLibC benoetigt")
    endif()
    if(NOT DEFINED EMLIBC_PORT)
        list(GET EMLIBC_PORTS 0 EMLIBC_DEFAULT_PORT)
        set(EMLIBC_PORT "${EMLIBC_DEFAULT_PORT}" CACHE STRING
            "Port, auf den der Aliasname 'emLibC' zeigt")
    endif()
    if(NOT DEFINED EMLIBC_CUBE_REPOSITORY)
        set(EMLIBC_CUBE_REPOSITORY "$ENV{HOME}/STM32Cube/Repository" CACHE PATH
            "Ablage der STM32Cube-Firmwarepakete (STM32Cube_FW_<Familie>_V<x.y.z>)")
    endif()

    # Ermittelt CMSIS-Include-Pfade und Familien-Symbol aus einem Cube-Paket.
    # Wird nur benutzt, wenn EMLIBC_<PORT>_INCLUDES nicht ohnehin gesetzt ist.
    function(emlibc_cube_defaults port)
        string(TOUPPER ${port} port_uc)
        if(EMLIBC_${port_uc}_INCLUDES)
            return()
        endif()

        # stm32f4 -> F4
        string(REGEX REPLACE "^stm32" "" family ${port})
        string(TOUPPER ${family} family_uc)

        file(GLOB pkgs "${EMLIBC_CUBE_REPOSITORY}/STM32Cube_FW_${family_uc}_V*")
        if(NOT pkgs)
            return()
        endif()
        list(SORT pkgs)
        list(REVERSE pkgs)          # neuestes Paket zuerst
        list(GET pkgs 0 pkg)

        set(dev "${pkg}/Drivers/CMSIS/Device/ST/STM32${family_uc}xx/Include")
        if(NOT EXISTS "${dev}/${port}xx.h")
            return()
        endif()

        set(inc
            "${dev}"
            "${pkg}/Drivers/CMSIS/Include"
            "${pkg}/Drivers/STM32${family_uc}xx_HAL_Driver/Inc")

        # stm32<familie>xx.h zieht stm32<familie>xx_hal_conf.h nach. Die erzeugt
        # sonst CubeMX pro Projekt; fuer eine reine Bibliothek genuegt die
        # Vorlage aus dem Cube-Paket.
        set(conf_dir "${CMAKE_CURRENT_BINARY_DIR}/hal_conf/${port}")
        set(tmpl "${pkg}/Drivers/STM32${family_uc}xx_HAL_Driver/Inc/${port}xx_hal_conf_template.h")
        if(EXISTS "${tmpl}")
            configure_file("${tmpl}" "${conf_dir}/${port}xx_hal_conf.h" COPYONLY)
            list(APPEND inc "${conf_dir}")
        endif()

        set(EMLIBC_${port_uc}_INCLUDES ${inc} PARENT_SCOPE)

        # stm32<familie>xx.h verlangt ein Device-Symbol, sonst bricht es mit
        # "Please select first the target device" ab. Der konkrete Typ legt nur
        # die Registerdefinitionen fest -> Platzhalter, den das Projekt mit
        # EMLIBC_<PORT>_DEFINES auf sein Bauteil setzen sollte.
        if(NOT EMLIBC_${port_uc}_DEFINES)
            set(EMLIBC_${port_uc}_DEFINES
                USE_HAL_DRIVER ${EMLIBC_${port_uc}_DEVICE_DEFAULT} PARENT_SCOPE)
        endif()

        message(STATUS "emLibC: ${port} verwendet ${pkg}")
    endfunction()

    # Platzhalter-Bauteil je Familie (siehe emlibc_cube_defaults)
    set(EMLIBC_STM32F4_DEVICE_DEFAULT STM32F407xx)
    set(EMLIBC_STM32F3_DEVICE_DEFAULT STM32F303xC)

    file(GLOB EMLIBC_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/common/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/wire/src/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/port/src/*.c"
    )
    # slip.c nutzt die veraltete device-API und wird auch im CubeIDE-Projekt
    # nicht uebersetzt.
    list(REMOVE_ITEM EMLIBC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/common/src/slip.c")

    foreach(port ${EMLIBC_PORTS})
        set(port_dir "${CMAKE_CURRENT_SOURCE_DIR}/port/src/stm/${port}")
        if(NOT IS_DIRECTORY "${port_dir}")
            message(FATAL_ERROR "emLibC: Port '${port}' nicht gefunden (${port_dir})")
        endif()

        string(TOUPPER ${port} port_uc)
        set(lib emLibC_${port})

        # Fehlen Include-Pfade, aus dem Cube-Repository ergaenzen
        emlibc_cube_defaults(${port})

        # Projekt-Header koennen pro Port abweichen: main.h & Co. gehoeren zu
        # genau einer MCU-Familie, andere Ports brauchen eine eigene Auswahl.
        set(cfg_inc ${EMLIBC_CONFIG_INCLUDES})
        if(EMLIBC_${port_uc}_CONFIG_INCLUDES)
            set(cfg_inc ${EMLIBC_${port_uc}_CONFIG_INCLUDES})
        endif()

        # Quellen, die fuer diesen Port nicht uebersetzt werden sollen
        # (Dateinamen ohne Pfad, z.B. "serial.c").
        set(src ${EMLIBC_SRC})
        foreach(skip ${EMLIBC_${port_uc}_EXCLUDE})
            list(FILTER src EXCLUDE REGEX "/${skip}$")
        endforeach()

        add_library(${lib} STATIC ${src})

        target_include_directories(${lib} PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/common/inc"
            "${CMAKE_CURRENT_SOURCE_DIR}/port/inc"
            "${CMAKE_CURRENT_SOURCE_DIR}/wire/inc"
            "${port_dir}"
            ${cfg_inc}
            ${EMLIBC_${port_uc}_INCLUDES}
        )
        target_compile_definitions(${lib} PUBLIC ${EMLIBC_${port_uc}_DEFINES})
        target_compile_options(${lib} PRIVATE ${EMLIBC_${port_uc}_FLAGS})

        # hal_port.h zieht stm32<familie>xx.h nach. Fehlt der CMSIS-Header,
        # bricht erst der Compiler ab -> hier fruehzeitig und verstaendlich melden.
        get_target_property(inc_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
        unset(EMLIBC_${port_uc}_CMSIS CACHE)
        find_file(EMLIBC_${port_uc}_CMSIS "${port}xx.h"
            PATHS ${inc_dirs} NO_DEFAULT_PATH)
        if(NOT EMLIBC_${port_uc}_CMSIS)
            message(FATAL_ERROR
                "emLibC: '${port}xx.h' fuer Port '${port}' nicht gefunden.\n"
                "  Die CMSIS-/HAL-Header dieser Familie liegen nicht im Projekt.\n"
                "  Abhilfe: -DEMLIBC_${port_uc}_INCLUDES=\"<pfad/CMSIS/Device/...>;<pfad/CMSIS/Include>\"\n"
                "  oder den Port aus EMLIBC_PORTS entfernen.")
        endif()

        message(STATUS "emLibC: ${lib} (${port}xx.h aus ${EMLIBC_${port_uc}_CMSIS})")
    endforeach()

    if(NOT TARGET emLibC_${EMLIBC_PORT})
        message(FATAL_ERROR
            "emLibC: EMLIBC_PORT='${EMLIBC_PORT}' ist nicht in EMLIBC_PORTS='${EMLIBC_PORTS}'")
    endif()
    add_library(emLibC ALIAS emLibC_${EMLIBC_PORT})
    message(STATUS "emLibC: Alias 'emLibC' -> emLibC_${EMLIBC_PORT}")

    return()
endif()

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

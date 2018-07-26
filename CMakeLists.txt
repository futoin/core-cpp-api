
cmake_minimum_required(VERSION 3.2)

#--------------------------------------
# Hunter init
#--------------------------------------

include("cmake/HunterGate.cmake")

HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.1.tar.gz"
    SHA1 "51d2d6be411251c8de18c4ca20ef778880cf4cce"
)

#--------------------------------------
# Project definition
#--------------------------------------

project(futoin_api
        VERSION "0.0.1"
        LANGUAGES CXX)

option(FUTOIN_API_WITH_TESTS "Build with tests" ON)
option(FUTOIN_API_WITH_DOCS "Build documentation" ON)

if (FUTOIN_API_WITH_TESTS)
    hunter_add_package(Boost COMPONENTS test)
    find_package(Boost CONFIG REQUIRED unit_test_framework)
endif()

file(GLOB_RECURSE FUTOIN_API_SRC
    ${CMAKE_SOURCE_DIR}/include/*.?pp
    ${CMAKE_SOURCE_DIR}/src/*.?pp
)

add_compile_options(
    -std=c++11
    -Wall
    -Wextra
    -Werror
)
include_directories( AFTER include )

#add_library(futoin_api STATIC ${FUTOIN_API_SRC})

#--------------------------------------
# Project testing
#--------------------------------------
if (FUTOIN_API_WITH_TESTS)
    include(CTest)
    
    enable_testing()

    add_executable(CompileTest tests/compile.test.cpp)
    add_test(Compile CompileTest)

    add_executable(AsyncStepsTest tests/asyncsteps.test.cpp)
    target_link_libraries(AsyncStepsTest PRIVATE Boost::unit_test_framework)
    add_test(AsyncSteps AsyncStepsTest)
endif()

#--------------------------------------
# Static analysis & formatting
#--------------------------------------
if (FUTOIN_API_WITH_TESTS)
    file(GLOB_RECURSE ALL_SRC_FILES
        ${CMAKE_SOURCE_DIR}/include/*.?pp
        ${CMAKE_SOURCE_DIR}/src/*.?pp
    )
    
    get_target_property(BOOST_INC_DIRS "Boost::boost" INTERFACE_INCLUDE_DIRECTORIES)

    #---
    add_custom_target(
        cppcheck ALL
        COMMAND /usr/bin/cppcheck
        --enable=warning,style,performance,portability,information,missingInclude
        --std=c++11
        --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
        --verbose
        --quiet
        --inline-suppr
        # --check-config
        -I ${CMAKE_CURRENT_LIST_DIR}/include
        ${ALL_SRC_FILES}
    )
    
    add_custom_target(
        cppcheck-tests
        COMMAND /usr/bin/cppcheck
        --enable=warning,performance,portability,information
        --std=c++11
        --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
        --verbose
        --quiet
        --inline-suppr
        # --check-config
        -I ${BOOST_INC_DIRS}
        -I ${CMAKE_CURRENT_LIST_DIR}/include
        -I /usr/include/
        -I /usr/include/x86_64-linux-gnu/
        -I /usr/include/x86_64-linux-gnu/c++/6/
        -I /usr/include/c++/6/
        -I /usr/include/c++/6/tr1/
        -I /usr/include/linux/
        ${CMAKE_CURRENT_LIST_DIR}/tests/
    )
    
    #---
    add_custom_target(
        clangtidy
        COMMAND /usr/bin/clang-tidy-6.0
        -fix
        -format-style=file
        -std=c++11
        ${ALL_SRC_FILES}
    )
    
    add_custom_target(
        clangformat ALL
        COMMAND /usr/bin/clang-format-6.0
        -style=file
        -i
        ${ALL_SRC_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
    )
endif()

#--------------------------------------
# Build documentation
#--------------------------------------
if (FUTOIN_API_WITH_DOCS)
    find_package(Doxygen REQUIRED dot)
    
    add_custom_target(
        docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
    # prevent Doxygen segfault on invalid code
    add_dependencies(docs CompileTest)
endif()

#--------------------------------------

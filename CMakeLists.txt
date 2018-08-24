
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

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

# Options
#-----
option(FUTOIN_WITH_TESTS "Build with tests" ON)
option(FUTOIN_WITH_DOCS "Build documentation" ON)

# Deps
#-----
hunter_add_package(Boost)
find_package(Boost CONFIG REQUIRED)

if (FUTOIN_WITH_TESTS)
    hunter_add_package(Boost COMPONENTS test)
    find_package(Boost CONFIG REQUIRED unit_test_framework)
endif()

# Sources
#-----
file(GLOB_RECURSE FUTOIN_API_SRC
    ${CMAKE_CURRENT_LIST_DIR}/include/*.?pp
    ${CMAKE_CURRENT_LIST_DIR}/src/*.?pp
)

# Result objects
#-----
add_library(${PROJECT_NAME} ${FUTOIN_API_SRC})
add_library(futoin::api ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    Boost::boost
)
# since CMake 3.8
#target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11 )
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
    target_compile_options(${PROJECT_NAME} PRIVATE
        # see target_compile_features
        -std=c++11
        -Wall
        -Wextra
        -Werror
    )
endif()


#--------------------------------------
# Project testing
#--------------------------------------
if (FUTOIN_WITH_TESTS)
    include(CTest)
    
    enable_testing()

    #---
    add_executable(IAsyncStepsTest tests/iasyncsteps.test.cpp)
    target_link_libraries(IAsyncStepsTest
        PRIVATE ${PROJECT_NAME} Boost::unit_test_framework)
    add_test(AsyncSteps IAsyncStepsTest)
endif()

#--------------------------------------
# Static analysis & formatting
#--------------------------------------
if (FUTOIN_WITH_TESTS)
    file(GLOB_RECURSE ALL_SRC_FILES
        ${CMAKE_CURRENT_LIST_DIR}/include/*.?pp
        ${CMAKE_CURRENT_LIST_DIR}/src/*.?pp
        ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp
    )
    set(LINT_SRC_FILES ${ALL_SRC_FILES})
    list(FILTER LINT_SRC_FILES EXCLUDE REGEX reqcpp)
    
    get_target_property(BOOST_INC_DIRS "Boost::boost" INTERFACE_INCLUDE_DIRECTORIES)
    
    #---
    add_custom_target(
        clangtidy-api ALL
        COMMAND /usr/bin/clang-tidy-6.0
        -checks=bugprone-*,cert-*,clang-analyzer-*,clang-analyzer-*,llvm-*,misc-*,modernize-*,performance-*,readability-*
        -warnings-as-errors=*
        -quiet
        -format-style=file
        ${LINT_SRC_FILES}
        --
        -I${CMAKE_CURRENT_LIST_DIR}/include/
    )
    
    add_custom_target(
        clangformat-api ALL
        COMMAND /usr/bin/clang-format-6.0
        -style=file
        -i
        ${ALL_SRC_FILES}
    )
endif()

#--------------------------------------
# Build documentation
#--------------------------------------
if (FUTOIN_API_WITH_DOCS)
    find_package(Doxygen REQUIRED dot)
    
    add_custom_target(
        docs-api ALL
        COMMAND ${DOXYGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
    # prevent Doxygen segfault on invalid code
    add_dependencies(docs ${PROJECT_NAME})
endif()

#--------------------------------------

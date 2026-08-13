
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

#--------------------------------------
# Hunter init
#--------------------------------------

include("cmake/HunterGate.cmake")

HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.26.10.tar.gz"
    SHA1 "3d904b93ba684f0e671a2d44969a1a127abe3e66"
    LOCAL
)

#--------------------------------------
# Project definition
#--------------------------------------

project(futoin_api
        VERSION "0.3.3" # AUTO-REPLACE
        LANGUAGES CXX)

# Options
#-----
option(FUTOIN_WITH_TESTS "Build with tests" OFF)
option(FUTOIN_WITH_DOCS "Build documentation" OFF)
option(FUTOIN_WITH_EXC "Build with exceptions" ON)

# Deps
#-----
if (FUTOIN_WITH_TESTS)
    hunter_add_package(Boost COMPONENTS test)
    find_package(Boost CONFIG REQUIRED unit_test_framework)
endif()

# Sources
#-----
file(GLOB_RECURSE PROJECT_HDR
    ${CMAKE_CURRENT_LIST_DIR}/include/*.?pp
    ${CMAKE_CURRENT_LIST_DIR}/include/*.?
)
file(GLOB_RECURSE PROJECT_SRC
    ${CMAKE_CURRENT_LIST_DIR}/src/*.?pp
    ${CMAKE_CURRENT_LIST_DIR}/src/*.?
)

# Result objects
#-----
add_library(${PROJECT_NAME} ${PROJECT_SRC})
add_library(futoin::api ALIAS ${PROJECT_NAME})
target_sources(${PROJECT_NAME} PUBLIC ${PROJECT_HDR})

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11 )
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
    target_compile_options(${PROJECT_NAME} PRIVATE
        # see target_compile_features
        -std=c++11
        -Wall
        -Wextra
        -Werror
    )
    if (NOT FUTOIN_WITH_EXC)
        target_compile_options(${PROJECT_NAME} PRIVATE
            -fno-exceptions
            -DFUTOIN_NO_EXC
        )
    endif()
endif()


#--------------------------------------
# Project testing
#--------------------------------------
if (FUTOIN_WITH_TESTS)
    include(CTest)

    enable_testing()

    #---
    set(PROJECT_TEST_NAME FutoInAPITest)
    file(GLOB_RECURSE PROJECT_TEST_SRC
        ${CMAKE_CURRENT_LIST_DIR}/tests/*.test.?pp
    )
    add_executable(${PROJECT_TEST_NAME} ${PROJECT_TEST_SRC})
    
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
        target_compile_options(${PROJECT_TEST_NAME} PRIVATE
            # see target_compile_features
            -std=c++11
            -Wall
            -Wextra
            -Werror
        )
        if (NOT FUTOIN_WITH_EXC)
            target_compile_options(${PROJECT_TEST_NAME} PRIVATE
                -fno-exceptions
                -DFUTOIN_NO_EXC
            )
        endif()
    endif()

    target_link_libraries(${PROJECT_TEST_NAME}
        PRIVATE ${PROJECT_NAME} Boost::unit_test_framework)
    add_test(${PROJECT_NAME} ${PROJECT_TEST_NAME})
endif()

#--------------------------------------
# Static analysis & formatting
#--------------------------------------
if (FUTOIN_WITH_TESTS)
    #---
    add_custom_target(
        clangtidy-${PROJECT_NAME} ALL
        COMMAND /usr/bin/clang-tidy
        -quiet
        ${PROJECT_SRC}
        --
        `echo
        $<JOIN:$<TARGET_PROPERTY:${PROJECT_NAME},COMPILE_OPTIONS>,\ >
        -I$<JOIN:$<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>,\ -I>`
        -DFUTOIN_IN_CLANG_TIDY
    )

    add_custom_target(
        clangformat-${PROJECT_NAME} ALL
        COMMAND /usr/bin/clang-format
        -style=file
        -i
        `echo
        $<JOIN:$<TARGET_PROPERTY:${PROJECT_NAME},SOURCES>,\ >
        $<JOIN:$<TARGET_PROPERTY:${PROJECT_TEST_NAME},SOURCES>,\ >`
    )
endif()

#--------------------------------------
# Build documentation
#--------------------------------------
if (FUTOIN_WITH_DOCS)
    find_package(Doxygen REQUIRED dot)

    doxygen_add_docs(docs-${PROJECT_NAME})
endif()

#--------------------------------------

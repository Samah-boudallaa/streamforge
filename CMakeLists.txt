cmake_minimum_required(VERSION 3.20)

project(StreamForge
    VERSION 0.1.0
    DESCRIPTION "A high-performance analytical data processing engine in C17"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SF_BUILD_TESTS "Build StreamForge tests" ON)
option(SF_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

function(sf_enable_warnings target)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )

        if(SF_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

add_library(streamforge_core STATIC
    src/core/types.c
    src/core/error.c
    src/core/log.c
    src/core/version.c
    src/core/config.c
    src/ingestion/csv.c
    src/storage/column.c
)

add_library(StreamForge::Core ALIAS streamforge_core)

target_include_directories(streamforge_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(streamforge_core PUBLIC c_std_17)

sf_enable_warnings(streamforge_core)

if(SF_BUILD_TESTS)
    enable_testing()

    function(sf_add_test name source)
        add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE StreamForge::Core)
        sf_enable_warnings(${name})
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    sf_add_test(test_core   tests/unit/test_core.c)
    sf_add_test(test_csv    tests/unit/test_csv.c)
    sf_add_test(test_column tests/unit/test_column.c)
endif()

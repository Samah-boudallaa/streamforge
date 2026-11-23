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
    src/ingestion/infer.c
    src/ingestion/csv_loader.c
    src/storage/column.c
    src/storage/table.c
    src/execution/aggregate.c
)

add_library(StreamForge::Core ALIAS streamforge_core)

target_include_directories(streamforge_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(streamforge_core PUBLIC c_std_17)

sf_enable_warnings(streamforge_core)

add_executable(streamforge src/cli/main.c)
target_link_libraries(streamforge PRIVATE StreamForge::Core)
sf_enable_warnings(streamforge)

if(SF_BUILD_TESTS)
    enable_testing()

    function(sf_add_test name source)
        add_executable(${name} ${source})
        target_link_libraries(${name} PRIVATE StreamForge::Core)
        sf_enable_warnings(${name})
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    sf_add_test(test_core      tests/unit/test_core.c)
    sf_add_test(test_csv       tests/unit/test_csv.c)
    sf_add_test(test_column    tests/unit/test_column.c)
    sf_add_test(test_infer     tests/unit/test_infer.c)
    sf_add_test(test_loader    tests/unit/test_loader.c)
    sf_add_test(test_aggregate tests/unit/test_aggregate.c)

    # End-to-end tests of the command-line tool.
    set(SF_SAMPLE ${CMAKE_CURRENT_SOURCE_DIR}/examples/sales.csv)

    add_test(NAME cli_schema COMMAND streamforge schema ${SF_SAMPLE})
    set_tests_properties(cli_schema PROPERTIES
        PASS_REGULAR_EXPRESSION "6 rows, 4 columns")

    add_test(NAME cli_sum_amount COMMAND streamforge agg ${SF_SAMPLE} amount sum)
    set_tests_properties(cli_sum_amount PROPERTIES
        PASS_REGULAR_EXPRESSION "sum\\(amount\\) = 756\\.5")

    add_test(NAME cli_avg_quantity COMMAND streamforge agg ${SF_SAMPLE} quantity avg)
    set_tests_properties(cli_avg_quantity PROPERTIES
        PASS_REGULAR_EXPRESSION "avg\\(quantity\\) = 3\\.6")

    add_test(NAME cli_count_amount COMMAND streamforge agg ${SF_SAMPLE} amount count)
    set_tests_properties(cli_count_amount PROPERTIES
        PASS_REGULAR_EXPRESSION "count\\(amount\\) = 5")

    add_test(NAME cli_unknown_column COMMAND streamforge agg ${SF_SAMPLE} nope sum)
    set_tests_properties(cli_unknown_column PROPERTIES WILL_FAIL TRUE)
endif()

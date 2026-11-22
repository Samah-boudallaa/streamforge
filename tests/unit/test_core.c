#include "streamforge/config.h"
#include "streamforge/error.h"
#include "streamforge/log.h"
#include "streamforge/types.h"
#include "streamforge/version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                      \
    do {                                                       \
        if (!(condition)) {                                    \
            (void)fprintf(                                     \
                stderr,                                        \
                "FAIL: %s (line %d)\n",                         \
                #condition,                                    \
                __LINE__                                       \
            );                                                 \
            return EXIT_FAILURE;                               \
        }                                                      \
    } while (0)

static int test_types(void)
{
    CHECK(strcmp(sf_type_name(SF_TYPE_INT32), "INT32") == 0);
    CHECK(strcmp(sf_type_name(SF_TYPE_STRING), "STRING") == 0);
    CHECK(strcmp(sf_type_name(SF_TYPE_INVALID), "INVALID") == 0);

    CHECK(sf_type_size(SF_TYPE_INT32) == 4U);
    CHECK(sf_type_size(SF_TYPE_FLOAT64) == 8U);
    CHECK(sf_type_size(SF_TYPE_STRING) == 0U);
    CHECK(sf_type_size(SF_TYPE_INVALID) == 0U);

    CHECK(sf_type_is_numeric(SF_TYPE_INT64));
    CHECK(sf_type_is_numeric(SF_TYPE_FLOAT32));
    CHECK(!sf_type_is_numeric(SF_TYPE_BOOL));
    CHECK(!sf_type_is_numeric(SF_TYPE_STRING));

    return EXIT_SUCCESS;
}

static int test_errors(void)
{
    sf_error_t error;

    sf_error_clear(&error);

    CHECK(error.code == SF_STATUS_OK);
    CHECK(error.message[0] == '\0');

    sf_error_set(
        &error,
        SF_STATUS_INVALID_ARGUMENT,
        "Invalid value: %d",
        42
    );

    CHECK(error.code == SF_STATUS_INVALID_ARGUMENT);
    CHECK(strcmp(error.message, "Invalid value: 42") == 0);
    CHECK(strcmp(
        sf_status_string(SF_STATUS_OUT_OF_MEMORY),
        "Out of memory"
    ) == 0);

    sf_error_clear(&error);

    CHECK(error.code == SF_STATUS_OK);
    CHECK(error.message[0] == '\0');

    return EXIT_SUCCESS;
}

static int test_configuration(void)
{
    sf_config_t config;
    sf_error_t error;

    sf_config_init(&config);

    CHECK(config.memory_limit_bytes > 0U);
    CHECK(config.batch_size > 0U);
    CHECK(config.log_level == SF_LOG_INFO);
    CHECK(config.enable_parallel);

    CHECK(sf_config_validate(&config, &error) == SF_STATUS_OK);
    CHECK(error.code == SF_STATUS_OK);

    config.batch_size = 0U;

    CHECK(
        sf_config_validate(&config, &error) ==
        SF_STATUS_INVALID_ARGUMENT
    );
    CHECK(error.code == SF_STATUS_INVALID_ARGUMENT);
    CHECK(strstr(error.message, "Batch size") != NULL);

    config.batch_size = 2048U;
    config.memory_limit_bytes = 0U;

    CHECK(
        sf_config_validate(&config, &error) ==
        SF_STATUS_INVALID_ARGUMENT
    );

    CHECK(sf_config_validate(NULL, &error) == SF_STATUS_INVALID_ARGUMENT);

    return EXIT_SUCCESS;
}

static int test_logging(void)
{
    CHECK(sf_log_get_level() == SF_LOG_INFO);

    sf_log_set_level(SF_LOG_DEBUG);
    CHECK(sf_log_get_level() == SF_LOG_DEBUG);

    sf_log_set_level(SF_LOG_ERROR);
    CHECK(sf_log_get_level() == SF_LOG_ERROR);

    sf_log_set_level(SF_LOG_OFF);
    CHECK(sf_log_get_level() == SF_LOG_OFF);

    /* Invalid values must not change the current level. */
    sf_log_set_level((sf_log_level_t)99);
    CHECK(sf_log_get_level() == SF_LOG_OFF);

    sf_log_set_level(SF_LOG_INFO);

    return EXIT_SUCCESS;
}

static int test_version(void)
{
    CHECK(strcmp(sf_version_string(), "0.1.0") == 0);
    CHECK(SF_VERSION_MAJOR == 0);
    CHECK(SF_VERSION_MINOR == 1);
    CHECK(SF_VERSION_PATCH == 0);

    return EXIT_SUCCESS;
}

int main(void)
{
    if (test_types() != EXIT_SUCCESS ||
        test_errors() != EXIT_SUCCESS ||
        test_configuration() != EXIT_SUCCESS ||
        test_logging() != EXIT_SUCCESS ||
        test_version() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    (void)puts("All Core Foundations tests passed.");

    return EXIT_SUCCESS;
}

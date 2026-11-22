#ifndef STREAMFORGE_TABLE_H
#define STREAMFORGE_TABLE_H

#include <stdbool.h>
#include <stddef.h>

#include "streamforge/column.h"
#include "streamforge/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name; /* owned, NUL-terminated */
    sf_type_t type;
} sf_field_t;

typedef struct {
    size_t num_fields;
    sf_field_t *fields;
} sf_schema_t;

/* A table owns its schema and one column per schema field. */
typedef struct {
    sf_schema_t schema;
    sf_column_t *columns;
    size_t num_rows;
} sf_table_t;

void sf_schema_destroy(sf_schema_t *schema);

/* Releases everything owned by the table. Safe on a zeroed table. */
void sf_table_destroy(sf_table_t *table);

/* Finds a column by exact name. Returns false when absent. */
bool sf_table_find_column(const sf_table_t *table, const char *name, size_t *index);

#ifdef __cplusplus
}
#endif

#endif

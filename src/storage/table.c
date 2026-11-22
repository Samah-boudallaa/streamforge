#include "streamforge/table.h"

#include <stdlib.h>
#include <string.h>

void sf_schema_destroy(sf_schema_t *schema) {
    if (schema == NULL) {
        return;
    }
    if (schema->fields != NULL) {
        for (size_t i = 0; i < schema->num_fields; i++) {
            free(schema->fields[i].name);
        }
    }
    free(schema->fields);
    memset(schema, 0, sizeof *schema);
}

void sf_table_destroy(sf_table_t *table) {
    if (table == NULL) {
        return;
    }
    if (table->columns != NULL) {
        for (size_t i = 0; i < table->schema.num_fields; i++) {
            sf_column_destroy(&table->columns[i]);
        }
    }
    free(table->columns);
    sf_schema_destroy(&table->schema);
    memset(table, 0, sizeof *table);
}

bool sf_table_find_column(const sf_table_t *table, const char *name, size_t *index) {
    if (table == NULL || name == NULL) {
        return false;
    }
    for (size_t i = 0; i < table->schema.num_fields; i++) {
        if (strcmp(table->schema.fields[i].name, name) == 0) {
            if (index != NULL) {
                *index = i;
            }
            return true;
        }
    }
    return false;
}

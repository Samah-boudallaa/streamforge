#include "streamforge/csv.h"

#define PUT(ch)                                  \
    do {                                         \
        if (out >= scratch_cap) {                \
            return SF_CSV_ERR_SCRATCH_FULL;      \
        }                                        \
        scratch[out++] = (ch);                   \
    } while (0)

sf_csv_dialect_t sf_csv_default_dialect(void) {
    sf_csv_dialect_t d = {',', '"'};
    return d;
}

void sf_csv_reader_init(sf_csv_reader_t *r, const char *data, size_t len,
                        sf_csv_dialect_t dialect) {
    r->data = data;
    r->len = len;
    r->pos = 0;
    r->line = 1;
    r->record_line = 1;
    r->dialect = dialect;
}

sf_csv_status_t sf_csv_next_record(sf_csv_reader_t *r, char *scratch, size_t scratch_cap,
                                   sf_csv_field_t *fields, size_t max_fields,
                                   size_t *out_count) {
    const char delim = r->dialect.delimiter;
    const char quote = r->dialect.quote;

    /* Skip blank lines. */
    while (r->pos < r->len && (r->data[r->pos] == '\n' || r->data[r->pos] == '\r')) {
        if (r->data[r->pos] == '\r' && r->pos + 1 < r->len && r->data[r->pos + 1] == '\n') {
            r->pos++;
        }
        r->pos++;
        r->line++;
    }
    if (r->pos >= r->len) {
        return SF_CSV_EOF;
    }

    r->record_line = r->line;

    size_t out = 0;
    size_t n = 0;

    for (;;) {
        if (n >= max_fields) {
            return SF_CSV_ERR_TOO_MANY_FIELDS;
        }
        size_t start = out;

        if (r->pos < r->len && r->data[r->pos] == quote) {
            r->pos++; /* opening quote */
            for (;;) {
                if (r->pos >= r->len) {
                    return SF_CSV_ERR_UNTERMINATED_QUOTE;
                }
                char c = r->data[r->pos];
                if (c == quote) {
                    if (r->pos + 1 < r->len && r->data[r->pos + 1] == quote) {
                        PUT(quote);
                        r->pos += 2;
                        continue;
                    }
                    r->pos++; /* closing quote */
                    break;
                }
                if (c == '\n') {
                    r->line++;
                }
                PUT(c);
                r->pos++;
            }
            if (r->pos < r->len) {
                char c = r->data[r->pos];
                if (c != delim && c != '\n' && c != '\r') {
                    return SF_CSV_ERR_BAD_QUOTE;
                }
            }
        } else {
            while (r->pos < r->len) {
                char c = r->data[r->pos];
                if (c == delim || c == '\n' || c == '\r') {
                    break;
                }
                PUT(c);
                r->pos++;
            }
        }

        PUT('\0');
        fields[n].text = scratch + start;
        fields[n].len = out - 1 - start;
        n++;

        if (r->pos >= r->len) {
            break;
        }
        char c = r->data[r->pos];
        if (c == delim) {
            r->pos++;
            continue;
        }
        if (c == '\r' && r->pos + 1 < r->len && r->data[r->pos + 1] == '\n') {
            r->pos++;
        }
        r->pos++;
        r->line++;
        break;
    }

    *out_count = n;
    return SF_CSV_OK;
}

const char *sf_csv_status_string(sf_csv_status_t s) {
    switch (s) {
    case SF_CSV_OK:
        return "ok";
    case SF_CSV_EOF:
        return "end of input";
    case SF_CSV_ERR_UNTERMINATED_QUOTE:
        return "unterminated quoted field";
    case SF_CSV_ERR_BAD_QUOTE:
        return "unexpected character after closing quote";
    case SF_CSV_ERR_TOO_MANY_FIELDS:
        return "too many fields in record";
    case SF_CSV_ERR_SCRATCH_FULL:
        return "scratch buffer too small";
    }
    return "unknown";
}

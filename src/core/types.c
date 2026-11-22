#include "streamforge/types.h"

#include <stdint.h>

const char *sf_type_name(sf_type_t type)
{
    switch (type) {
        case SF_TYPE_BOOL:    return "BOOL";
        case SF_TYPE_INT8:    return "INT8";
        case SF_TYPE_INT16:   return "INT16";
        case SF_TYPE_INT32:   return "INT32";
        case SF_TYPE_INT64:   return "INT64";
        case SF_TYPE_UINT8:   return "UINT8";
        case SF_TYPE_UINT16:  return "UINT16";
        case SF_TYPE_UINT32:  return "UINT32";
        case SF_TYPE_UINT64:  return "UINT64";
        case SF_TYPE_FLOAT32: return "FLOAT32";
        case SF_TYPE_FLOAT64: return "FLOAT64";
        case SF_TYPE_STRING:  return "STRING";
        case SF_TYPE_BINARY:  return "BINARY";
        case SF_TYPE_INVALID:
        default:              return "INVALID";
    }
}

size_t sf_type_size(sf_type_t type)
{
    switch (type) {
        case SF_TYPE_BOOL:    return sizeof(bool);
        case SF_TYPE_INT8:    return sizeof(int8_t);
        case SF_TYPE_INT16:   return sizeof(int16_t);
        case SF_TYPE_INT32:   return sizeof(int32_t);
        case SF_TYPE_INT64:   return sizeof(int64_t);
        case SF_TYPE_UINT8:   return sizeof(uint8_t);
        case SF_TYPE_UINT16:  return sizeof(uint16_t);
        case SF_TYPE_UINT32:  return sizeof(uint32_t);
        case SF_TYPE_UINT64:  return sizeof(uint64_t);
        case SF_TYPE_FLOAT32: return sizeof(float);
        case SF_TYPE_FLOAT64: return sizeof(double);
        case SF_TYPE_STRING:
        case SF_TYPE_BINARY:
        case SF_TYPE_INVALID:
        default:              return 0;
    }
}

bool sf_type_is_numeric(sf_type_t type)
{
    switch (type) {
        case SF_TYPE_INT8:
        case SF_TYPE_INT16:
        case SF_TYPE_INT32:
        case SF_TYPE_INT64:
        case SF_TYPE_UINT8:
        case SF_TYPE_UINT16:
        case SF_TYPE_UINT32:
        case SF_TYPE_UINT64:
        case SF_TYPE_FLOAT32:
        case SF_TYPE_FLOAT64:
            return true;

        default:
            return false;
    }
}

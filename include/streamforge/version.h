#ifndef STREAMFORGE_VERSION_H
#define STREAMFORGE_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define SF_VERSION_MAJOR 0
#define SF_VERSION_MINOR 1
#define SF_VERSION_PATCH 0

#define SF_VERSION_STRING "0.1.0"

/* Returns the current StreamForge version string. */
const char *sf_version_string(void);

#ifdef __cplusplus
}
#endif

#endif

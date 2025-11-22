/* MicroPython configuration for Homeshell embedding */

// Include common MicroPython embed configuration.
#include <port/mpconfigport_common.h>

// Use a more complete configuration with useful features
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)

// Banner configuration
#define MICROPY_BANNER_NAME_AND_VERSION         "MicroPython"
#define MICROPY_BANNER_MACHINE                  "Homeshell embedded"

// Enable essential features
#define MICROPY_ENABLE_COMPILER                 (1)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_PY_GC                           (1)
#define MICROPY_PY_SYS                          (1)
#define MICROPY_PY_IO                           (0)  // Disabled - no file I/O support
#define MICROPY_PY_BUILTINS_STR_UNICODE         (1)

// Enable useful standard library modules
#define MICROPY_PY_ARRAY                        (1)
#define MICROPY_PY_COLLECTIONS                  (1)
#define MICROPY_PY_MATH                         (1)
#define MICROPY_PY_CMATH                        (1)
#define MICROPY_PY_SYS_STDFILES                 (0)  // Disabled - no stdfiles support
#define MICROPY_PY_SELECT                       (0)  // Disabled - requires I/O
#define MICROPY_PY_TIME                         (1)
#define MICROPY_PY_ERRNO                        (1)
#define MICROPY_PY_JSON                         (1)
#define MICROPY_PY_RE                           (1)
#define MICROPY_PY_BINASCII                     (1)
#define MICROPY_PY_HASHLIB                      (1)

// Enable REPL helpers
#define MICROPY_HELPER_REPL                     (1)
#define MICROPY_REPL_AUTO_INDENT                (1)

// Disable extra qstr pool (no frozen modules)
#define MICROPY_QSTR_EXTRA_POOL                 mp_qstr_const_pool


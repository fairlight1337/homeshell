// MicroPython port implementation for Homeshell
// Based on the minimal port example

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/lexer.h"
#include "py/mperrno.h"
#include "py/builtin.h"

// Required stubs for MicroPython

// Lexer from file - we don't support importing from files
mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    (void)filename;
    mp_raise_OSError(MP_ENOENT);
}

// Import stat - we don't support file imports
mp_import_stat_t mp_import_stat(const char *path) {
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
}

// Note: nlr_jump_fail, __fatal_error, and __assert_func are already
// provided by embed_util.c in the embed port, so we don't redefine them here

// Stub for io module (required even when disabled)
const mp_obj_module_t mp_module_io = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_builtins_globals,
};


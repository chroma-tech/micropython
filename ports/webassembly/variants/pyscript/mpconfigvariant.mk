# Enable btree and your manifest
MICROPY_PY_BTREE = 1
MICROPY_STREAMS_POSIX_API = 1
FROZEN_MANIFEST ?= variants/pyscript/manifest.py

# Emscripten runtime (optional)
JSFLAGS += -s ALLOW_MEMORY_GROWTH

# Make the <sys/cdefs.h> shim visible everywhere (incl. qstr)
INC += -I$(VARIANT_DIR)/shims
QSTR_GEN_EXTRA_CFLAGS += -I$(VARIANT_DIR)/shims \
                         -include $(VARIANT_DIR)/shims/sys/cdefs.h
CFLAGS_EXTRA += -include $(VARIANT_DIR)/shims/sys/cdefs.h


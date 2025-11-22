# Makefile to build MicroPython embed port for Homeshell

MICROPYTHON_TOP = $(CURDIR)/micropython
MPY_EMBED_DIR = micropython_embed

# Configuration
MPCONFIGPORT_H = $(CURDIR)/micropython_config/mpconfigport.h

.PHONY: all clean

all: $(MPY_EMBED_DIR)

$(MPY_EMBED_DIR):
	cd $(MICROPYTHON_TOP)/ports/embed && \
	MICROPYTHON_TOP=$(MICROPYTHON_TOP) MPCONFIGPORT_H=$(MPCONFIGPORT_H) \
	make -f embed.mk PACKAGE_DIR=$(CURDIR)/$(MPY_EMBED_DIR)

clean:
	rm -rf $(MPY_EMBED_DIR)


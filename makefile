# ----------------------------
# Makefile Options
# ----------------------------

NAME = BTDCE
ICON = icon.png
DESCRIPTION = "Bloons TD remake for the TI-84 Plus CE."
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

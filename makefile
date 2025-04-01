# Compiler and flags
CC = cc
CFLAGS = -Wall -Wextra

# Source files and targets
DP1_SRCS = DP-1/src/DP-1.c
DP2_SRCS = DP-2/src/DP-2.c
DC_SRCS = DC/src/DC.c

TARGETS = DP-1/bin/DP-1 DP-2/bin/DP-2 DC/bin/DC

# Default target (build all)
all: $(TARGETS)

# DP-1
DP-1/bin/DP-1: $(DP1_SRCS)
	$(CC) $(CFLAGS) -o $@ $<

# DP-2
DP-2/bin/DP-2: $(DP2_SRCS)
	$(CC) $(CFLAGS) -o $@ $<

# DC
DC/bin/DC: $(DC_SRCS)
	$(CC) $(CFLAGS) -o $@ $<

# Clean
clean:
	rm -f $(TARGETS)

.PHONY: all clean
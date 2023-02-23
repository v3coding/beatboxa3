OUTFILE = beatbox
OUTDIR = $(HOME)/cmpt433/public/myApps

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -Wshadow -pthread
all:
	$(CC_C) $(CFLAGS) main.c functions.c -o $(OUTDIR)/$(OUTFILE)
app:
	$(CC_C) $(CFLAGS) main.c functions.c $(OUTDIR)/$(OUTFILE)
clean:
	rm $(OUTDIR)/$(OUTFILE)
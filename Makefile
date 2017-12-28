# file: Makefile
# vim:fileencoding=utf-8:fdm=marker:ft=make
# This is the Makefile for dosrestore. A program to restore old DOS backups.
#
# Author: R.F. Smith <rsmith@xs4all.nl>
# Created: 2006-08-12 20:54:23 +0200
# Last modified: 2017-12-28 15:32:41 +0100

# If make complains about a missing file, run 'make depend' first

# Choose an appropriate CFLAGS and LFLAGS

# The next two lines are for building an executable suitable for debugging.
#CFLAGS = -pipe -Wall -g -O0
#LFLAGS = -pipe

# Other libraries to link against
LIBS +=

# Location where to install the binary.
BINDIR = /usr/local/bin

# Location where to install the manual-page.
MANDIR = /usr/local/man/man1

##### Maintainer stuff goes here:

# Package name and version: BASENAME-VMAJOR.VMINOR.tar.gz
BASENAME = dosrestore
VMAJOR   = 1
VMINOR   = 1

# Source files.
SRCS = dosrestore.c

##### No editing necessary beyond this point
# Object files.
OBJS = $(SRCS:.c=.o)

# Version number
VERSION = -DVERSION=\"$(VMAJOR).$(VMINOR)\"
# Program name
PACKAGE = -DPACKAGE=\"$(BASENAME)\"
# Add to CFLAGS
CFLAGS += $(VERSION) $(PACKAGE)

.PHONY: clean install uninstall dist backup all

all: $(BASENAME)

# builds a binary.
$(BASENAME): $(OBJS)
	$(CC) $(LFLAGS) $(LDIRS) -o $(BASENAME) $(OBJS) $(LIBS)

# Remove all generated files.
clean:;
	rm -f $(OBJS) $(BASENAME) *~ core gmon.out

# Install the program and manual page. You should be root to do this.
install: $(BASENAME)
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to install the program!"; \
		exit 1; \
	fi
	install -m 755 $(BASENAME) $(BINDIR)
	if [ -e $(BASENAME).1 ]; then \
		install -m 644 $(BASENAME).1 $(MANDIR); \
		gzip -f $(MANDIR)/$(BASENAME).1; \
	fi

uninstall:;
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to uninstall the program!"; \
		exit 1; \
	fi
	rm -f $(BINDIR)/$(BASENAME)
	rm -f $(MANDIR)/$(BASENAME).1*

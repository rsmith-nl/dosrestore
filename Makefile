# Package name and version: BASENAME-VMAJOR.VMINOR.VPATCH.tar.gz
BASENAME = dosrestore  ## Name for the project
VMAJOR   = 1
VMINOR   = 1
VPATCH   = 0

# Define the C compiler to be used, if not cc.
#CC = gcc

# Add appropriate CFLAGS and LFLAGS
CFLAGS = -Os ## Compiler flags for C
CFLAGS += -std=c11 -march=native -pipe -ffast-math
CFLAGS += -Wall -Wshadow -Wpointer-arith -Wstrict-prototypes
LFLAGS = -s -flto ## Linker flags
# For a static executable, add the following LFLAGS.
#LFLAGS += --static

# Other libraries to link against
#LIBS += -lm

PREFIX = ${HOME}/.local  ## Root for the installation dicrectory tree.
BINDIR = $(PREFIX)/bin  ## Location where the binary will be installed.
MANDIR = $(PREFIX)/man/man1 ## Location for the manual-page.
DOCSDIR= $(PREFIX)/share/doc/$(BASENAME)  ## Location for the documentation

##### Maintainer stuff goes here:
DISTFILES = Makefile  ## Files that need to be included in the distribution.
# Source files.
SRC = dosrestore.c  ## source code file.

##### No editing necessary beyond this point
all: $(BASENAME)  ## Compile the program. (default)

$(BASENAME): $(SRC) version.h
	$(CC) $(CFLAGS) $(LFLAGS) $(LDIRS) -o $(BASENAME) $(SRC) $(LIBS)

.PHONY: clean
clean:  ## Remove all generated files.
	rm -f $(BASENAME) *~ core gmon.out $(TARFILE)* version.h backup-*

.PHONY: install
install: $(BASENAME)  ## Install the program.
	install -d $(BINDIR)
	install -m 755 $(BASENAME) $(BINDIR)
	install -m 644 $(BASENAME).1 $(MANDIR)
	gzip -f -q $(MANDIR)/$(BASENAME).1

.PHONY: uninstall
uninstall:  ## Uninstall the program.
	rm -f $(BINDIR)/$(BASENAME)

version.h:
	echo '#define PACKAGE "'${BASENAME}'"' >version.h
	echo '#define VERSION "'${VMAJOR}"."${VMINOR}'"' >>version.h

.PHONY: style
style:  ## Reformat source code using astyle.
	astyle -n *.c

.PHONY: man
man:  ## Show the rendered manual page
	mandoc -Tutf8 $(BASENAME).1 | less

.PHONY: help
help:  ## List available commands
	@echo "make variables:"
	@echo
	@sed -n -e '/##/s/=.*\#\#/\t/p' Makefile
	@echo
	@echo "make targets:"
	@echo
	@sed -n -e '/##/s/:.*\#\#/\t/p' Makefile

# Predefined directory/file names
PKGDIR  = $(BASENAME)-$(VMAJOR).$(VMINOR).$(VPATCH)  ## Directory name in the package.
TARFILE = $(PKGDIR).tar.gz  ## Filename for the package.

dist: clean  # Build a tar distribution file
	rm -rf $(PKGDIR)
	mkdir -p $(PKGDIR)
	cp $(DISTFILES) $(XTRA_DIST) *.c *.h $(PKGDIR)
	tar -czf $(TARFILE) $(PKGDIR)
	rm -rf $(PKGDIR)

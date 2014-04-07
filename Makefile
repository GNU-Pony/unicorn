# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


# The package path prefix, if you want to install to another root, set DESTDIR to that root
PREFIX ?= /usr
# The command path excluding prefix
BIN ?= /bin
# The resource path excluding prefix
DATA ?= /share
# The command path including prefix
BINDIR ?= $(PREFIX)$(BIN)
# The resource path including prefix
DATADIR ?= $(PREFIX)$(DATA)
# The generic documentation path including prefix
DOCDIR ?= $(DATADIR)/doc
# The info manual documentation path including prefix
INFODIR ?= $(DATADIR)/info
# The license base path including prefix
LICENSEDIR ?= $(DATADIR)/licenses

# The name of the command as it should be installed
COMMAND ?= unicorn
# The name of the package as it should be installed
PKGNAME ?= unicorn

# Optimisation settings for C code compilation
OPTIMISE ?= -Og -g
# Warnings settings for C code compilation
WARN = -Wall -Wextra -pedantic -Wdouble-promotion -Wformat=2 -Winit-self -Wmissing-include-dirs \
       -Wfloat-equal -Wmissing-prototypes -Wmissing-declarations -Wtrampolines -Wnested-externs \
       -Wno-variadic-macros -Wdeclaration-after-statement -Wundef -Wpacked -Wunsafe-loop-optimizations \
       -Wbad-function-cast -Wwrite-strings -Wlogical-op -Wstrict-prototypes -Wold-style-definition \
       -Wvector-operation-performance -Wstack-protector -Wunsuffixed-float-constants -Wcast-align \
       -Wsync-nand -Wshadow -Wredundant-decls -Winline -Wcast-qual -Wsign-conversion -Wstrict-overflow
# The C standard for C code compilation
STD = c99

# Flags to compile with
FLAGS = -std=$(STD) $(WARN) $(OPTIMISE) $(CFLAGS) $(LDFLAGS) $(CPPFLAGS)


# Build rules

.PHONY: default
default: command # info shell

.PHONY: all
all: command # doc shell

.PHONY: command
command: bin/unicorn

bin/unicorn: src/unicorn.c
	@mkdir -p bin
	$(CC) $(FLAGS) -o $@ $^


# Build rules for documentation

#.PHONY: doc
#doc: info pdf dvi ps
#
#.PHONY: info
#info: unicorn.info
#%.info: info/%.texinfo info/fdl.texinfo
#	makeinfo $<
#
#.PHONY: pdf
#pdf: unicorn.pdf
#%.pdf: info/%.texinfo info/fdl.texinfo
#	mkdir -p obj
#	cd obj ; yes X | texi2pdf ../$<
#	mv info/$@ $@
#
#.PHONY: dvi
#dvi: unicorn.dvi
#%.dvi: info/%.texinfo info/fdl.texinfo
#	mkdir -p obj
#	cd obj ; yes X | $(TEXI2DVI) ../$<
#	mv info/$@ $@
#
#.PHONY: ps
#ps: unicorn.ps
#%.ps: info/%.texinfo info/fdl.texinfo
#	mkdir -p obj
#	cd obj ; yes X | texi2pdf --ps ../$<
#	mv obj/$@ $@


# Build rules for shell auto-completion

#.PHONY: shell
#shell: bash zsh fish
#
#.PHONY: bash
#bash: bin/unicorn.bash
#bin/unicorn.bash: src/completion
#	@mkdir -p bin
#	auto-auto-complete bash --output $@ --source $<
#
#.PHONY: zsh
#zsh: bin/unicorn.zsh
#bin/unicorn.zsh: src/completion
#	@mkdir -p bin
#	auto-auto-complete zsh --output $@ --source $<
#
#.PHONY: fish
#fish: bin/unicorn.fish
#bin/unicorn.fish: src/completion
#	@mkdir -p bin
#	auto-auto-complete fish --output $@ --source $<


# Install rules

.PHONY: install
install: install-base # install-info install-shell

.PHONY: install
install-all: install-base # install-doc install-shell

# Install base rules

.PHONY: install-base
install-base: install-command install-license

.PHONY: install-command
install-command: bin/unicorn
	install -dm755 -- "$(DESTDIR)$(BINDIR)"
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"

.PHONY: install-license
install-license:
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -m644 COPYING LICENSE -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"

# Install documentation

#.PHONY: install-doc
#install-doc: install-info install-pdf install-ps install-dvi install-examples
#
#.PHONY: install-examples
#install-examples: $(foreach E,$(EXAMPLES),examples/$(E))
#	install -dm755 -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME)/examples"
#	install -m644 $^ -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME)/examples"
#
#.PHONY: install-info
#install-info: unicorn.info
#	install -dm755 -- "$(DESTDIR)$(INFODIR)"
#	install -m644 $< -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"
#
#.PHONY: install-pdf
#install-pdf: unicorn.pdf
#	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
#	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"
#
#.PHONY: install-ps
#install-ps: unicorn.ps
#	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
#	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"
#
#.PHONY: install-dvi
#install-dvi: unicorn.dvi
#	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
#	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"

# Install shell auto-completion

#.PHONY: install-shell
#install-shell: install-bash install-zsh install-fish
#
#.PHONY: install-bash
#install-bash: bin/unicorn.bash
#	install -dm755 -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
#	install -m644 $< -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/$(COMMAND)"
#
#.PHONY: install-zsh
#install-zsh: bin/unicorn.zsh
#	install -dm755 -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
#	install -m644 $< -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_$(COMMAND)"
#
#.PHONY: install-fish
#install-fish: bin/unicorn.fish
#	install -dm755 -- "$(DESTDIR)$(DATADIR)/fish/completions"
#	install -m644 $< -- "$(DESTDIR)$(DATADIR)/fish/completions/$(COMMAND).fish"


# Uninstall rules

.PHONY: uninstall
uninstall:
	-rm -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/COPYING"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/LICENSE"
#	-rmdir -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME)/examples"
#	-rmdir -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME)"
#	-rm -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"
#	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"
#	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"
#	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"
#	-rm -- "$(DESTDIR)$(DATADIR)/fish/completions/$(COMMAND).fish"
#	-rmdir -- "$(DESTDIR)$(DATADIR)/fish/completions"
#	-rmdir -- "$(DESTDIR)$(DATADIR)/fish"
#	-rm -- "$(DESTDIR)$(DATADIR)/zsh/site-functions/_$(COMMAND)"
#	-rmdir -- "$(DESTDIR)$(DATADIR)/zsh/site-functions"
#	-rmdir -- "$(DESTDIR)$(DATADIR)/zsh"
#	-rm -- "$(DESTDIR)$(DATADIR)/bash-completion/completions/$(COMMAND)"
#	-rmdir -- "$(DESTDIR)$(DATADIR)/bash-completion/completions"
#	-rmdir -- "$(DESTDIR)$(DATADIR)/bash-completion"


# Clean rules

.PHONY: all
clean:
	-rm -r bin obj unicorn.{info,pdf,ps,dvi}


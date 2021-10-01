NAME := mtxorb
MAJOR := 1
MINOR := 0
VERSION = $(MAJOR).$(MINOR)

SRCDIR = .
OBJDIR = obj

LIBSO  = lib$(NAME).so
LIBSOM = $(LIBSO).$(MAJOR)
LIBSOV = $(LIBSO).$(VERSION)
LIBA   = lib$(NAME).a

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

CFLAGS := -std=c89 -pedantic -O2 -Wall -Wextra
LDFLAGS := -shared

CC = gcc
AR = ar
RM = rm

.SUFFIXES:
.SUFFIXES: .c .o .so.?
.PHONY: lib-static lib-shared clean help

default: help

lib-static: $(LIBA)
lib-shared: $(LIBSO)

$(LIBA): $(OBJS)
	@echo -n "Building static library..."
	@$(AR) rcs $@ $<
	@echo "Done"
	@echo "Copy $(LIBA) and mtxorb.h to your project"

# Create a symbolic link to the shared library
$(LIBSO): $(LIBSOV)
	@ln -s $< $@
	@echo "Copy $(LIBSO) and $(LIBSOV) to e.g. /usr/local/lib and mtxorb.h to /usr/local/include"

$(LIBSOV): $(OBJS)
	@echo -n "Building shared library..."
	@$(CC) $(CFLAGS) $(LDFLAGS) -fPIC -Wl,-soname,$(LIBSOM) $< -o $@
	@echo "Done"

# Create objects from C sources
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -rf $(OBJDIR) ./*.so* ./*.a
	@echo "All build files removed"

help:
	@echo "================= libmtxorb ================="
	@echo "make lib-static \tBuild static library"
	@echo "make lib-shared \tBuild shared library"
	@echo "make clean \t\tRemove all build files"

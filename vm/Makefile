SHELL = /bin/sh
CC    = gcc
 
FLAGS        = -std=gnu99 -Iinclude -w -I ../compiler/include
CFLAGS       = -pedantic -Wall -Wextra -march=native -ggdb3 -O3
DEBUGFLAGS   = #-O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -combine -fwhole-program
POSTFLAGS    = -ldl -lffi
 
TARGET  = ../eesk
SOURCES = $(shell echo src/*.c)
SOURCES += $(shell find ../compiler/src/*.c -type f | grep -v ../compiler/src/main.c)
COMMON  = 
HEADERS = $(shell echo include/*.h)
COMPILERHEADERS = $(shell echo ../compiler/include/*.h)
OBJECTS = $(SOURCES:.c=.o)
 
PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin
 
 
all: $(TARGET)
 
$(TARGET): $(OBJECTS) $(COMMON)
	$(CC) $(FLAGS) $(CFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(OBJECTS) $(POSTFLAGS)

release: $(SOURCES) $(HEADERS) $(COMMON)
	$(CC) $(FLAGS) $(CFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(SOURCES)

profile: CFLAGS += -pg
profile: $(TARGET)
 
 
install: release
	install -D $(TARGET) $(BINDIR)/$(TARGET)
 
install-strip: release
	install -D -s $(TARGET) $(BINDIR)/$(TARGET)
 
uninstall:
	-rm $(BINDIR)/$(TARGET)
 
 
clean:
	-rm -f $(OBJECTS)
	-rm -f gmon.out
 
distclean: clean
	-rm -f $(TARGET)
 
 
.SECONDEXPANSION:
 
$(foreach OBJ,$(OBJECTS),$(eval $(OBJ)_DEPS = $(shell gcc -MM $(OBJ:.o=.c) | sed s/.*://)))
%.o: %.c $$($$@_DEPS)
	$(CC) $(FLAGS) $(CFLAGS) $(DEBUGFLAGS) -c -o $@ $<
 
# %.o: %.c $(HEADERS) $(COMMON)
#    $(CC) $(FLAGS) $(CFLAGS) $(DEBUGFLAGS) -c -o $@ $<
 
 
.PHONY : all profile release \\
	install install-strip uninstall clean distclean

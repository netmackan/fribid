
COMMONCFLAGS=-O2 -g -Wall -Wextra -std=c99 -pedantic -Wno-unused-parameter
CCFLAGS=$(COMMONCFLAGS) -I../npapi/include -fPIC -fvisibility=hidden

OBJECTS=ipc.o npmain.o npobject.o plugin.o pipe.o npn_gate.o np_entry.o

all: libplugins.so

ipc.o: ../common/defines.h ../common/pipe.h plugin.h
npmain.o: npobject.h plugin.h
npobject.o: npobject.h plugin.h
pipe.o: ../common/pipe.h ../common/pipe.c ../common/defines.h
plugin.o: plugin.h

.c.o:
	$(CC) $(CCFLAGS) -c $< -o $@

libplugins.so: $(OBJECTS)
	$(CC) -shared -Wl,-z,defs,-soname,libplugins.so -o $@ $(OBJECTS) -lc

.PHONY: all clean
clean:
	-rm -f $(OBJECTS) libplugins.so


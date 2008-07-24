CXX=g++ 
CXXFLAGS=-g -Wall -ansi -O3 #-pedantic
LDLIBS=-lz -lstdc++
OBJECTS=datatypes.o io.o
EXECUTABLES=create merge query

all: $(EXECUTABLES)

create: $(OBJECTS)
merge: $(OBJECTS)
query: $(OBJECTS)

clean:
	-rm -f $(OBJECTS) $(EXECUTABLES)


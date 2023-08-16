# Paths
PATH_VXF = ../libvxf

# Which compiler should be used?
CXX = clang

UNAME_S := $(shell uname -s)

#####################################################################
# macOS
#####################################################################
ifeq ($(UNAME_S),Darwin)

# Which compiler options should be used?
CFLAGS = -std=c++11 -stdlib=libc++ -Wall -pedantic -Wextra -fpic -O3 -mmacosx-version-min=10.9

# Linker options
LFLAGS = -framework CoreFoundation -framework CoreServices -mmacosx-version-min=10.9 -lstdc++

# Where are the headers?
INCLUDE =  -I$(PATH_VXF)/include/ -I$(PATH_VXF)/prec/  -I$(PATH_VXF)/3rdparty/

endif

#####################################################################
# Linux
#####################################################################
ifeq ($(UNAME_S),Linux)

# Which compiler options should be used?
CFLAGS = -std=c++11 -stdlib=libstdc++ -Wall -pedantic -Wextra -fpic -O3

# Linker options
LFLAGS = -lstdc++ -lm  -lpthread -ldl

# Where are the headers?
INCLUDE =  -I$(PATH_VXF)/include/ -I$(PATH_VXF)/prec/  -I$(PATH_VXF)/3rdparty/

endif

#####################################################################
# BELOW THIS LINE THERE IS NO OS DEPENDENT CODE
#####################################################################

# List of sources
SOURCES = src/Main.cpp\
 $(PATH_VXF)/src/core/AutoreleasePool.cpp\
 $(PATH_VXF)/src/core/Date.cpp\
 $(PATH_VXF)/src/core/String.cpp\
 $(PATH_VXF)/src/core/Object.cpp\
 $(PATH_VXF)/src/core/File.cpp\
 $(PATH_VXF)/src/core/Stream.cpp\
 $(PATH_VXF)/src/core/Integer.cpp\
 $(PATH_VXF)/src/core/Math.cpp\
 $(PATH_VXF)/src/core/Exception.cpp\
 $(PATH_VXF)/src/core/Character.cpp\
 $(PATH_VXF)/src/core/Charset.cpp\
 $(PATH_VXF)/src/core/UTF8Decoder.cpp\
 $(PATH_VXF)/src/core/UTF16Decoder.cpp\
 $(PATH_VXF)/src/core/CharsetDecoder.cpp\
 $(PATH_VXF)/src/core/Windows1252Decoder.cpp\
 $(PATH_VXF)/src/core/MacRomanDecoder.cpp\
 $(PATH_VXF)/src/core/StringTokenizer.cpp\
 $(PATH_VXF)/src/core/Synchronized.cpp\
 $(PATH_VXF)/src/core/System.cpp

OBJECTS = $(SOURCES:.cpp=.o)

# Target = ALL
all: $(OBJECTS)
	$(CXX) $(LFLAGS) -o shpc $(OBJECTS)
	strip shpc
	mkdir -p bin
	mv shpc bin/shpc

static: $(OBJECTS)
	ar rcs libjameo.a $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -c $< -o $@

install:
	 cp bin/shpc /usr/local/bin/shpc

remove:
	rm /usr/local/bin/shpc

clean:
	rm -f $(OBJECTS)
	rm -Rf bin/*

# DO NOT DELETE

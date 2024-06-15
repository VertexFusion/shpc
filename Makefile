# Paths
PATH_CORE = ../libcore
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
INCLUDE = -I$(PATH_CORE)/include  -I$(PATH_VXF)/include/ -I$(PATH_VXF)/prec/  -I$(PATH_VXF)/3rdparty/

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
INCLUDE = -I$(PATH_CORE)/include -I$(PATH_VXF)/include/ -I$(PATH_VXF)/prec/  -I$(PATH_VXF)/3rdparty/

endif

#####################################################################
# BELOW THIS LINE THERE IS NO OS DEPENDENT CODE
#####################################################################

# List of sources
SOURCES = src/Main.cpp\
 $(PATH_CORE)/src/core/AutoreleasePool.cpp\
 $(PATH_CORE)/src/core/ByteArray.cpp\
 $(PATH_CORE)/src/core/Date.cpp\
 $(PATH_CORE)/src/core/Double.cpp\
 $(PATH_CORE)/src/core/String.cpp\
 $(PATH_CORE)/src/core/Object.cpp\
 $(PATH_CORE)/src/core/File.cpp\
 $(PATH_CORE)/src/core/Stream.cpp\
 $(PATH_CORE)/src/core/I18nBundle.cpp\
 $(PATH_CORE)/src/core/Integer.cpp\
 $(PATH_CORE)/src/core/Iterator.cpp\
 $(PATH_CORE)/src/core/Math.cpp\
 $(PATH_CORE)/src/core/Exception.cpp\
 $(PATH_CORE)/src/core/Hashtable.cpp\
 $(PATH_CORE)/src/core/Character.cpp\
 $(PATH_CORE)/src/core/Charset.cpp\
 $(PATH_CORE)/src/core/UTF8Decoder.cpp\
 $(PATH_CORE)/src/core/UTF16Decoder.cpp\
 $(PATH_CORE)/src/core/CharsetDecoder.cpp\
 $(PATH_CORE)/src/core/Preferences.cpp\
 $(PATH_CORE)/src/core/Windows1252Decoder.cpp\
 $(PATH_CORE)/src/core/MacRomanDecoder.cpp\
 $(PATH_CORE)/src/core/StringTokenizer.cpp\
 $(PATH_CORE)/src/core/Serializer.cpp\
 $(PATH_CORE)/src/core/Mutex.cpp\
 $(PATH_CORE)/src/core/System.cpp

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

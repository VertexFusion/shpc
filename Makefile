# Paths
PATH_CORE = ../libcore

# Which compiler should be used?
CXX = clang

UNAME_S := $(shell uname -s)

#####################################################################
# macOS
#####################################################################
ifeq ($(UNAME_S),Darwin)

C__ = clang

# Which compiler options should be used?
CFLAGS = -std=c++11 -stdlib=libc++ -Wall -pedantic -Wextra -fpic -O3
OCFLAGS= -g -Wall -pedantic -Wextra -Wno-long-long -fPIC -O3 -x objective-c++ -fobjc-arc

# Linker options
LFLAGS = -framework CoreFoundation -framework CoreServices -framework Foundation -lstdc++

# Where are the headers?
INCLUDE = -I$(PATH_CORE)/include -I$(PATH_CORE)/prec/  -I$(PATH_CORE)/3rdparty/

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
INCLUDE = -I$(PATH_CORE)/include -I$(PATH_CORE)/prec/  -I$(PATH_CORE)/3rdparty/

endif

#####################################################################
# BELOW THIS LINE THERE IS NO OS DEPENDENT CODE
#####################################################################

# List of sources
SOURCES = src/Main.cpp\
 $(PATH_CORE)/src/core/AutoreleasePool.cpp\
 $(PATH_CORE)/src/core/ByteArray.cpp\
 $(PATH_CORE)/src/core/Date.cpp\
 $(PATH_CORE)/src/core/String.cpp\
 $(PATH_CORE)/src/core/StringList.cpp\
 $(PATH_CORE)/src/core/Object.cpp\
 $(PATH_CORE)/src/core/File.cpp\
 $(PATH_CORE)/src/core/Stream.cpp\
 $(PATH_CORE)/src/core/I18nBundle.cpp\
 $(PATH_CORE)/src/core/Integer.cpp\
 $(PATH_CORE)/src/core/Iterator.cpp\
 $(PATH_CORE)/src/core/Math.cpp\
 $(PATH_CORE)/src/core/Resource.cpp\
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

ifeq ($(UNAME_S),Darwin)
 MMSOURCES=$(PATH_CORE)/src/core/MacBindings.mm
endif

#####################################################################
# BELOW THIS LINE THERE IS NO OS DEPENDENT CODE
#####################################################################

OBJECTS = $(SOURCES:.cpp=.o) $(MMSOURCES:.mm=.o)

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

%.o: %.mm
	$(C__) $(OCFLAGS) $(INCLUDE) -c $< -o $@

install:
	 cp bin/shpc /usr/local/bin/shpc

remove:
	rm /usr/local/bin/shpc

clean:
	rm -f $(OBJECTS)
	rm -Rf bin/*

# DO NOT DELETE


# Compiler & tools
CC      = gcc
CFLAGS  = -Wall -Wextra

# ===================== External Libraries ===============
# Include paths
INCLUDES = \
    -Iinclude \
    -Iexternal/civetweb/include \
    -Iexternal/webview/core/include \
    -Iexternal/cJSON

# Library search paths
LIB_PATHS = \
    -Lexternal/civetweb \
    -Lexternal/webview/build/core

# Link libraries (no file extensions, works cross-platform)
LIBS = -lcivetweb -lwebview

# =====================================================


# Other LIBS (BUILT IN)   TODO: WINDOWS ONLY lws2_32 is. need to fix.
LIBS += -lws2_32 -pthread


# Project files
SRC_DIR     = src
SOURCES     = $(wildcard $(SRC_DIR)/*.c) external/cJSON/cJSON.c   # all .c in src + cJSON (External)
OBJECTS     = $(SOURCES:.c=.o)
TARGET      = bin/webserver

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LIB_PATHS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

 
# To compile civetweb with websocket support:
# make -C external/civetweb lib WITH_WEBSOCKET=1 # making a static lib


# when downloading repo 
# git submodule update --init --recursive

# civetweb using commands
# cmake -G Ninja -B build -S . -D CMAKE_BUILD_TYPE=Release
# cmake --build build


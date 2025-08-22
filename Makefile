
CC = gcc
CFLAGS = -Wall -Iexternal/civetweb/include -Iexternal/cJSON -I./include  -Isrc
SRC = src/main.c src/server.c src/web_socket.c src/file_tracker.c external/cJSON/cJSON.c 
OBJ = external/civetweb/out/src/civetweb.o external/civetweb/out/resources/res.o
OUT = bin/main.exe

all: $(OUT)

$(OUT): $(SRC) $(OBJ)
	$(CC) $(CFLAGS) $(SRC) $(OBJ) -o $(OUT) -lws2_32 -pthread

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)

rebuild: clean all

.PHONY: all run clean rebuild

# To compile civetweb with websocket support:
# make -C external/civetweb WITH_WEBSOCKET=1


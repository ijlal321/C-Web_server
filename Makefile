CC = gcc
CFLAGS = -Wall
SRC = main.c
OBJ = external/civetweb/out/src/civetweb.o external/civetweb/out/resources/res.o
OUT = main.exe

$(OUT): $(SRC) $(OBJ)
	$(CC) $(CFLAGS) $(SRC) $(OBJ) -o $(OUT)  -lws2_32

# $ gcc main.c external/civetweb/out/src/civetweb.o external/civetweb/out/resources/res.o -o main.exe -lws2_32
# gcc ws_main.c external/civetweb/out/src/civetweb.o external/civetweb/out/resources/res.o -o ws_main.exe -lws2_32


# to compile civetweb 
# make WITH_WEBSOCKET=1


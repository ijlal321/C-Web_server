CC = gcc
CFLAGS = -Wall
SRC = main.c
OBJ = external/civetweb/out/src/civetweb.o external/civetweb/out/resources/res.o
OUT = main.exe

$(OUT): $(SRC) $(OBJ)
	$(CC) $(CFLAGS) $(SRC) $(OBJ) -o $(OUT)  -lws2_32
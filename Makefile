CC = gcc
CFLAGS = -Wall -O2
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

TARGET = game
SRC = game.c

all:
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

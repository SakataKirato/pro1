CC = gcc
CFLAGS = -Wall -O2 -I.
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

TARGET = game
SRC = game.c cJSON.c

all:
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)

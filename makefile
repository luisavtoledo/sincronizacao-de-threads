CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = ex1

.PHONY: all clean

all: build

build: $(TARGET)

$(TARGET): ep1.o passa_tempo.o
	$(CC) $(CFLAGS) $^ -o $@

principal.o: ep1.c passa_tempo.h
	$(CC) $(CFLAGS) -c $< -o $@

passa_tempo.o: passa_tempo.c passa_tempo.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o
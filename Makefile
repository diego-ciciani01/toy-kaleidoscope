CC = gcc
FLEGS = -Wall

# obj file to build
OBJC = frontend.c
TARGET = frontend.o

all: $(TARGET)

$(TARGET): $(OBJC)
	$(CC) $(FLEGS) $< -o $@

clean:
	rm -f *.o 

.PHONY: all clean

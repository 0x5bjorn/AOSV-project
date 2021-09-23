CC = gcc
CFLAGS = -pthread

TARGET = module_test

all: 
	$(CC) $(CFLAGS) -o $(TARGET) module_test.c ums_lib.c ums_lib.h

clean:
	rm -rf $(TARGET)
CC = gcc
CFLAGS = -pthread

TARGET = sample_test

all: 
	$(CC) $(CFLAGS) -o $(TARGET) sample_test.c ums_lib.c ums_lib.h

clean:
	rm -rf $(TARGET)
CC = gcc
CFLAGS = -Wall -pthread
TARGET = parking_lot

all: $(TARGET)

$(TARGET): main_linux.c parking_lot.h
	$(CC) $(CFLAGS) -o $(TARGET) main_linux.c

clean:
	rm -f $(TARGET) parking_log.log

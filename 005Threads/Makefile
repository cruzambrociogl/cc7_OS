CC = gcc
CFLAGS = -Wall -pthread
TARGET = log_analyzer

all: $(TARGET)

$(TARGET): main_linux.c log_processor.c
	$(CC) $(CFLAGS) -o $(TARGET) main_linux.c log_processor.c

clean:
	rm -f $(TARGET)

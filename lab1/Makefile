CC = gcc
CFLAGS = -Wall -O2
TARGET = ioctl_eject

all: $(TARGET)

$(TARGET): eject_cdrom.c
	$(CC) $(CFLAGS) -o $(TARGET) eject_cdrom.c

clean:
	rm -f $(TARGET)

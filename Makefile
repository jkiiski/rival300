TARGET = rival300
CFLAGS ?= -g -Wall -Werror
LDFLAGS = -lusb-1.0
INSTALL_DIR ?= /usr/local/bin

all: $(TARGET)

$(TARGET): rival300.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

install: $(TARGET)
	# SUID bit for non-root users to access USB devices
	install -m 4755 $(TARGET) $(INSTALL_DIR)

clean:
	rm -f $(TARGET)

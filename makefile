CC = gcc
CFLAGS = -w `pkg-config --cflags gtk+-3.0` -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto `pkg-config --libs gtk+-3.0`
TARGET = cia-chat
SRCS = client.c wcp_clt.c

.PHONY: all clean

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

# Detect the operating system
UNAME_S := $(shell uname -s)

# Default to gcc; change if needed
CC := gcc

# Flags for both CFLAGS and LDFLAGS
ifeq ($(UNAME_S),Linux)
    CFLAGS := -Wall `pkg-config --cflags gtk+-3.0`
    LDFLAGS := -L/usr/local/lib `pkg-config --libs gtk+-3.0` -lssl -lcrypto
endif
ifeq ($(UNAME_S),Darwin) # Darwin is the system name for macOS
    CFLAGS = -w `pkg-config --cflags gtk+-3.0` -I/opt/homebrew/opt/openssl@3/include
	LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto `pkg-config --libs gtk+-3.0`
endif

TARGET := cia-chat
SRCS := client.c wcp_clt.c
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

# Clean up
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean

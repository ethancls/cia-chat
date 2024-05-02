# Detect the operating system
UNAME_S := $(shell uname -s)

# Default to gcc; change if needed
CC := gcc

# Flags for both CFLAGS and LDFLAGS
ifeq ($(UNAME_S),Linux)
    CFLAGS := -w `pkg-config --cflags gtk+-3.0` -lpthread
    LDFLAGS := -L/usr/local/lib `pkg-config --libs gtk+-3.0` -lssl -lcrypto
endif
ifeq ($(UNAME_S),Darwin) # Darwin is the system name for macOS
    CFLAGS = -w `pkg-config --cflags gtk+-3.0` -I/opt/homebrew/opt/openssl@3/include
	LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto `pkg-config --libs gtk+-3.0`
endif

TARGET := cia-chat
TARGET_SRV := wcp_serv
SRCS := client.c wcp_clt.c
SRCS_SRV := wcp_srv.c

all: $(TARGET) $(TARGET_SRV)

$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

$(TARGET_SRV):
	$(CC) $(CFLAGS) $(SRCS_SRV) $(LDFLAGS) -o $(TARGET_SRV)

# Clean up
clean:
	rm -f $(TARGET) $(TARGET_SRV)

# Phony targets
.PHONY: all clean


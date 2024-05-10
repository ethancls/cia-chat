UNAME_S := $(shell uname -s)

CC := gcc
ifeq ($(UNAME_S),Linux)

    CFLAGS := -w `pkg-config --cflags gtk+-3.0` -lpthread -g

    LDFLAGS := -L/usr/local/lib `pkg-config --libs gtk+-3.0` -lssl -lcrypto
    PKG_MANAGER := sudo apt-get install -y
    PKG_UPDATE := sudo apt-get update
    REQUIRED_PKGS := libgtk-3-dev libssl-dev pkg-config
endif
ifeq ($(UNAME_S),Darwin)
    CFLAGS = -w `pkg-config --cflags gtk+-3.0` -I/opt/homebrew/opt/openssl@3/include
    LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto `pkg-config --libs gtk+-3.0`
    PKG_MANAGER := brew install
    REQUIRED_PKGS := gtk+3 openssl@3 pkg-config
endif

TARGET := cia-chat
TARGET_SRV := tcp_srv
SRCS := tcp_clt.c gtk_ui.c
SRCS_SRV := tcp_srv.c

all: check-libs clean $(TARGET) $(TARGET_SRV)

$(TARGET):
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

$(TARGET_SRV):
	$(CC) $(CFLAGS) $(SRCS_SRV) $(LDFLAGS) -o $(TARGET_SRV)

# Check if necessary libraries are installed and install them if they are not
check-libs:
	@echo "Updating package lists..."
	@$(PKG_UPDATE)
	@echo "Checking and installing necessary libraries..."
	@$(PKG_MANAGER) $(REQUIRED_PKGS) || (echo "Installation failed, please check the availability of the packages." && exit 1)

# Clean up
clean:
	rm -f $(TARGET) $(TARGET_SRV)

# Phony targets
.PHONY: all clean check-libs

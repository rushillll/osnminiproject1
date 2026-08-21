CC = /opt/homebrew/bin/gcc-16

CFLAGS = -std=c23 \
         -D_POSIX_C_SOURCE=200809L \
         -D_XOPEN_SOURCE=700 \
         -Wall -Wextra -Werror \
         -Wno-unused-parameter \
         -fno-asm

CPPFLAGS = -Iinclude

SRC = src/main.c src/prompt.c

TARGET = shell.out

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

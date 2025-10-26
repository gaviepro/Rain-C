CC      := gcc
CFLAGS  := -D_GNU_SOURCE -std=c17 -O2 -Wall -Wextra -Isrc/include
LDFLAGS := -lcrypto
SRC     := src/main.c src/menu.c src/control_T3C.c src/control_dict.c src/hash.c
BIN     := lab/rainc

.PHONY: all clean

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p lab
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	$(RM) $(BIN)

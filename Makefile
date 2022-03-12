
CC=gcc
SOURCE_DIRS_COMMON := src/lib src/lib/texture src/noise
SOURCE_DIRS_PELTAR := src
SOURCE_DIRS_TEST := test
CFLAGS := -I.. `sdl-config --cflags` -Wall -Wextra -std=c99 -pedantic -g
LFLAGS := `sdl-config --libs` -lSDL_image -lm -g
OFLAGS := -O3

all: peltar

test: \
	test-planet \
	test-texture \
	test-starscape \
	test-level \
	test-cli

SRC_COMMON = $(foreach dir, $(SOURCE_DIRS_COMMON), $(wildcard $(dir)/*.c))
OBJ_COMMON = $(patsubst %.c, %.o, $(SRC_COMMON))

SRC_PELTAR = $(foreach dir, $(SOURCE_DIRS_PELTAR), $(wildcard $(dir)/*.c))
OBJ_PELTAR = $(patsubst %.c, %.o, $(SRC_PELTAR))

SRC_TEST = $(foreach dir, $(SOURCE_DIRS_TEST), $(wildcard $(dir)/*.c))
OBJ_TEST = $(patsubst %.c, %.o, $(SRC_TEST))

peltar: $(OBJ_COMMON) $(OBJ_PELTAR)
	$(CC) $^ $(LFLAGS) -o $@

test-planet: $(OBJ_COMMON) test/test-planet.o
	$(CC) $^ $(LFLAGS) -o $@

test-texture: $(OBJ_COMMON) test/test-texture.o
	$(CC) $^ $(LFLAGS) -o $@

test-starscape: $(OBJ_COMMON) test/test-starscape.o
	$(CC) $^ $(LFLAGS) -o $@

test-level: $(OBJ_COMMON) test/test-level.o
	$(CC) $^ $(LFLAGS) -o $@

test-cli: src/lib/cli.o test/test-cli.o
	$(CC) $^ $(LFLAGS) -o $@

$(OBJ_COMMON) : %.o : %.c
	$(CC) $(CFLAGS) $(OFLAGS) -c -o $@ $<

$(OBJ_PELTAR) : %.o : %.c
	$(CC) $(CFLAGS) $(OFLAGS) -c -o $@ $<

$(OBJ_TEST) : %.o : %.c
	$(CC) $(CFLAGS) $(OFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o
	rm -f src/lib/*.o
	rm -f src/lib/texture/*.o
	rm -f src/noise/*.o
	rm -f test/*.o
	rm -f peltar
	rm -f test-*


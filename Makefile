EXE=stuff
SRC=$(wildcard *.c)
HDR=$(wildcard *.h)
CFLAGS=-Wall -Werror -g
CC=gcc
DEL=rm -f


.PHONY: all
all: $(EXE)


$(EXE): clean $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o $(EXE)


.PHONY: clean
clean:
	$(DEL) $(EXE)
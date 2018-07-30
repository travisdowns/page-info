CPPFLAGS=-g -Os -Wall -Wextra

BINARIES := page-info-test

all: $(BINARIES)

page-info-test : page-info.o

clean:
	rm -rf *.o
	rm -f $(BINARIES)

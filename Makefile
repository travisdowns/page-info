CPPFLAGS=-g -Os -Wall -Wextra

BINARIES := page-info-test malloc-demo

all: $(BINARIES)

page-info-test : page-info.o
malloc-demo : page-info.o

clean:
	rm -rf *.o
	rm -f $(BINARIES)

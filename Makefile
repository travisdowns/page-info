CPPFLAGS=-g -Os -Wall -Wextra

BINARIES := page-info-test malloc-demo

all: $(BINARIES)

page-info.o : page-info.h

$(BINARIES) : % : %.o page-info.o page-info.h
	$(CC) $(CFLAGS) -o $@ $< page-info.o

clean:
	rm -rf *.o
	rm -f $(BINARIES)

.PHONY   := clean test valgrind
CPPFLAGS := -g -Os -Wall -Wextra -D_GNU_SOURCE=1
CFLAGS   := -std=c11
CXXFLAGS := -std=c++11
BINARIES := page-info-test malloc-demo

all: $(BINARIES)

page-info.o : page-info.h

$(BINARIES) : % : %.o page-info.o page-info.h
	$(CC) $(CFLAGS) -o $@ $< page-info.o

clean:
	rm -f *.o $(BINARIES)

test: valgrind

valgrind: $(BINARIES)
	     valgrind --error-exitcode=1 --leak-check=full ./malloc-demo 1000
	sudo valgrind --error-exitcode=1 --leak-check=full ./malloc-demo 1000
	     MAX_KIB=4096 valgrind --error-exitcode=1 --leak-check=full ./page-info-test
	sudo MAX_KIB=4096 valgrind --error-exitcode=1 --leak-check=full ./page-info-test


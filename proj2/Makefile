# Stanislav Nechutny - xnechu01
# Project #2 for IOS

CFLAGS = -std=gnu99 -Wall -g -Wextra -Werror -pedantic -lpthread -lrt

all: rivercrossing

rivercrossing: rivercrossing.c
	gcc rivercrossing.c $(CFLAGS) -o rivercrossing

zip: rivercrossing.c Makefile
	zip xnechu01.zip rivercrossing.c Makefile

clean:
	rm -rf xnechu01.zip rivercrossing

test: rivercrossing
	./rivercrossing 4 6 6 5

valgrind: rivercrossing
	valgrind -v --leak-check=full  ./rivercrossing 10 0 0 0 2>&1  | grep -i "error"

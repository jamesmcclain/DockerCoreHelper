CC ?= gcc
CFLAGS ?= -Wall -Werror -O1
LDFLAGS += -static

docker_core_helper: helper.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f *.o

cleaner: clean
	rm -f docker_core_helper

cleanest: cleaner

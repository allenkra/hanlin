CC=gcc
CFLAGS=-ggdb3 -c -g -Wall -Werror -std=gnu99
LDFLAGS=-pthread
SOURCES=proxyserver.c safequeue.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=proxyserver

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

test:
	~cs537-1/tests/P6/runtests

CFLAGS = -Wall -pedantic -std=gnu99

all: daemon

daemon:
	gcc -o build/daemond $(CFLAGS) `pkg-config --cflags --libs libnotify` daemon.c 
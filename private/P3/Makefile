# Variables
CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=gnu18
LOGIN = hanlin
SUBMITPATH = /home/cs537-1/handin/hanlin

# Targets
.PHONY: all
all: wsh

wsh: wsh.c wsh.h
	$(CC) $(CFLAGS) -o $@ $^

run: wsh
	./wsh

pack:
	tar -czvf $(LOGIN).tar.gz wsh.h wsh.c Makefile README.md

submit: pack
	cp $(LOGIN).tar.gz $(SUBMITPATH)

# Add your own test target
test: wsh
	# Add commands to run your tests here
	# For example: ./wsh < test_script.wsh

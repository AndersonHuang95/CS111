#  CS111 Lab 4 Part 1 Makefile 

CC = gcc
CFLAGS = -g -Wall -Wextra -Wno-unused-parameter -pthread
LDLIBS = -lgcc_s -lrt 
all: addtest

addtest_objects = addtest.o
addtest: $(addtest_objects)
	$(CC) $(CFLAGS) -o $@ $(addtest_objects) $(LDLIBS)

addtest.o: addtest.c
#srt.tgz: $(sources)
#	tar cf - --transform='s|^|srt/|' $(sources) | gzip -9 >$@

clean:
	rm -f *.o addtest 

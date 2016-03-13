#  CS111 Lab 4 Part 2 Makefile 

CC = gcc
CFLAGS = -g -w -pthread
LDLIBS = -lgcc_s -lrt 
all: sltest 

sltest_objects = sltest.o SortedList.o
sltest: $(sltest_objects)
	$(CC) $(CFLAGS) -o $@ $(sltest_objects) $(LDLIBS)

sltest.o: SortedList.h sltest.c

SortedList.o: SortedList.h SortedList.c 

#srt.tgz: $(sources)
#	tar cf - --transform='s|^|srt/|' $(sources) | gzip -9 >$@

clean:
	rm -f *.o sltest 

# CS111 Winter 2016
# Eggert
# Simpleton Shell Makefile 

CC = gcc
CFLAGS = -g3 -Wall -Wextra 
# LDLIBS = -ldl -Wl,-rpath=$PWD

default: simpsh 

simpsh_objects = simpsh.c
simpsh: $(simpsh_objects)
	$(CC) $(CFLAGS) -o $@ $(simpsh_objects)  
	
check: test.sh
	./test.sh

dist: lab1-andersonhuang.tar.gz
	./test.sh
sources = simpsh.c Makefile README test.sh 
lab1-andersonhuang.tar.gz: $(sources)
	tar -czvf $@ $(sources) 

clean: 
	rm -f *.o *.txt simpsh lab1-andersonhuang.tar.gz
#! /bin/bash

# CS111 Lab1b test cases 

gcc -o simpsh simpsh.c || echo "Program could not be compiled." 

#1: Test file-opening flags 

./simpsh --creat --rdonly a.txt || exit
./simpsh --creat --trunc --append --cloexec --directory --dsync --excl \
--nofollow --nonblock --rsync --sync --trunc --wronly b.txt || exit 
./simpsh --creat --trunc --nonblock --rdwr c.txt || exit 

#2: Test verbose flag 

./simpsh --verbose --creat --trunc --append --cloexec --directory --dsync --excl \
--nofollow --nonblock --rsync --sync --trunc --wronly d.txt > e.txt || exit 

#3: Test subcommands & wait 

echo "THIS IS FILE F.TXT" > f.txt
echo "File g is write-only" > g.txt
touch h.txt 
./simpsh --rdonly f.txt --wronly g.txt --wronly h.txt --command 0 1 2 cat || exit 
cat g.txt f.txt > i.txt || exit  

echo "ZZZ" > g.txt 
./simpsh --rdonly g.txt --pipe --pipe --creat --trunc --wronly j.txt --creat --append --wronly k.txt --command 3 5 6 cat g.txt - --command 0 2 6 sort --command 1 4 6 sed 's/Z/Replaced a Z\n/g' --wait || exit 

#output of the above test case should be 
#0 sort 
#0 cat g.txt - 
#0 sed s/Z/Replaced a Z\n/g 

#4: Test miscellaneous options 

./simpsh --catch 11 --abort 
# Should output "Caught signal 11. Exiting simpsh." 
./simpsh --rdonly a.txt --wronly b.txt --wronly c.txt --close 0 --close 1 --close 2 || exit 

# Options default, ignore, pause cannot be tested cleanly within script file 



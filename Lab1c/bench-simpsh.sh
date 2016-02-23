#! /bin/bash

# Benchmark for Simpleton Shell 
# All testing is done on files with garbage text pre-generated using Linux
# commands such as dd, base64 in conjunction with /dev/urandom /dev/zero 

# Create files
base64 /dev/zero | head -c 100000000 > zeros.txt
base64 /dev/urandom | head -c 100000000 > junk.txt
base64 /dev/urandom | head -c 100000000 > junk2.txt
echo > sortzeros.txt
echo > sortjunk.txt
echo > sortjunk2.txt
echo > errors.txt 

# Benchmark 1: Sorting/Filtering large files(~100MB), either with random text or all zeros

echo Benchmark 1A
./simpsh --profile --wait --rdonly zeros.txt --wronly sortzeros.txt --wronly errors.txt --command 0 1 2 sort 
./simpsh --profile --wait --rdonly junk.txt --wronly sortjunk.txt --wronly errors.txt --command 0 1 2 sort

# with piping 

echo Benchmark 1B
./simpsh --profile --wait --rdonly zeros.txt --pipe --pipe --wronly sortzeros.txt --wronly errors.txt --command 3 5 6 cat - --command 0 2 6 sort --command 1 4 6 tr 'A-Z' 'a-z'
./simpsh --profile --wait --rdonly junk.txt --pipe --pipe --wronly sortjunk.txt --wronly errors.txt --command 3 5 6 cat - --command 0 2 6 sort --command 1 4 6 tr 'A-Z' 'a-z'

# uniq command

echo Benchmark 1C
./simpsh --profile --wait --rdonly zeros.txt --wronly sortzeros.txt --wronly errors.txt --command 0 1 2 uniq -u
./simpsh --profile --wait --rdonly junk.txt --wronly sortjunk.txt --wronly errors.txt --command 0 1 2 uniq -u

# Benchmark 2: Filtering large files continued -- using stream editor(sed), comm, diff 

# stream editor 
echo Benchmark 2A
./simpsh --profile --wait --rdonly junk.txt --wronly sortjunk.txt --wronly errors.txt --command 0 1 2 sed 's/a*b//g' 
./simpsh --profile --wait --rdonly junk.txt --wronly sortjunk.txt --wronly errors.txt --command 0 1 2 sed '/^z$/d' 

# comm & diff 
echo Benchmark 2B
sort junk.txt > sortjunk.txt
sort junk2.txt > sortjunk2.txt
./simpsh --profile --wait --rdonly sortjunk.txt --rdonly sortjunk2.txt --creat --wronly commjunk.txt --wronly errors.txt --command 0 2 3 comm sortjunk.txt sortjunk2.txt 
./simpsh --profile --wait --rdonly junk.txt --rdonly junk2.txt --creat --wronly commjunk.txt --wronly errors.txt --command 0 2 3 diff junk.txt junk2.txt 

# Benchmark 3: Compressing Files with tar, zip, bzip2 

echo Benchmark 3A/B/C
./simpsh --profile --wait --rdonly junk.txt --creat --trunc --wronly junk.bz2 --wronly errors.txt --command 0 1 2 bzip2 -9 -c junk.txt 
./simpsh --profile --wait --rdonly junk.txt --creat --trunc --wronly junk.zip --wronly errors.txt --command 0 1 2 gzip -9 -c junk.txt 
./simpsh --profile --wait --rdonly junk.txt --creat --trunc --wronly junk.tgz --wronly errors.txt --command 0 1 2 tar -c -z -v -f junk.tar.gz junk.txt 

#! /bin/bash

# Benchmark for Bash 
# All testing is done on files with garbage text pre-generated using Linux
# commands such as dd, base64 in conjunction with /dev/urandom /dev/zero 

# Benchmark 1: Sorting/Filtering large files(~100MB), either with random text or all zeros

# sort command 
echo Benchmark 1A
time sort zeros.txt > sortzeros.txt 2>errors.txt
time sort junk.txt > sortjunk.txt 2>errors.txt

# with piping 
echo Benchmark 1B
time sort zeros.txt | tr 'A-Z' 'a-z' | cat - > sortzeros.txt 2>errors.txt
time sort junk.txt | tr 'A-Z' 'a-z' | cat - > sortjunk.txt 2>errors.txt

# uniq command
echo Benchmark 1C
time uniq -u zeros.txt > sortzeros.txt 2>errors.txt
time uniq -u junk.txt > sortjunk.txt 2>errors.txt

# Benchmark 2: Filtering large files continued -- using stream editor(sed), comm, diff 

# stream editor 
echo Benchmark 2A
time sed 's/a*b//g' junk.txt > sortjunk.txt 2>errors.txt 
time sed '/^z$/d' junk.txt > sortjunk.txt 2>errors.txt  

# comm & diff 
echo Benchmark 2B/C
sort junk.txt > sortjunk.txt
sort junk2.txt > sortjunk2.txt
time comm sortjunk.txt sortjunk2.txt > commjunk.txt 2>errors.txt
time diff junk.txt junk2.txt > commjunk.txt 2>errors.txt 

# Benchmark 3: Compressing Files with tar, gzip, bzip2 
echo Benchmark 3A/B/C
time bzip2 -9 -c junk.txt > junk.bz2
time gzip -9 -c junk.txt > junk.zip
time tar -czvf junk.tgz junk.txt 

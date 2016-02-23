#! /usr/local/cs/execline-2.1.4.5/bin/execlineb

# Benchmark 1: Sorting/Filtering large files(~100MB), either with random text or all zeros

# sort command 
foreground { printf "Benchmark 1A\n" }
foreground { time -p redirfd -w 1 sortzeros.txt redirfd -w 2 errors.txt sort zeros.txt }
foreground { time -p redirfd -w 1 sortjunk.txt redirfd -w 2 errors.txt sort junk.txt }

# with piping 

foreground { printf "Benchmark 1B\n" }
foreground { time -p redirfd -w 1 sortzeros.txt redirfd -w 2 errors.txt pipeline { pipeline { sort zeros.txt } tr 'A-Z' 'a-z' } cat - } 
foreground { time -p redirfd -w 1 sortjunk.txt redirfd -w 2 errors.txt pipeline { pipeline { sort junk.txt } tr 'A-Z' 'a-z' } cat - }

# uniq command
foreground { printf "Benchmark 1C\n" }
foreground { time -p redirfd -w 1 sortzeros.txt redirfd -w 2 errors.txt uniq -u zeros.txt }
foreground { time -p redirfd -w 1 sortjunk.txt redirfd -w 2 errors.txt uniq -u junk.txt }

# Benchmark 2: Filtering large files continued -- using stream editor(sed), comm, diff 

# stream editor 
foreground { printf "Benchmark 2A\n" }
foreground { time -p redirfd -w 1 sortjunk.txt redirfd -w 2 errors.txt sed s/a*b//g junk.txt }
foreground { time -p redirfd -w 1 sortjunk.txt redirfd -w 2 errors.txt sed /^z$/d junk.txt }

# comm & diff 
foreground { printf "Benchmark 2B/C\n" }
foreground { redirfd -w 1 sortjunk.txt sort junk.txt }
foreground { redirfd -w 1 sortjunk2.txt sort junk2.txt }
foreground { time -p redirfd -w 1 commjunk.txt redirfd -w 2 errors.txt comm sortjunk.txt sortjunk2.txt }
foreground { time -p redirfd -w 1 commjunk.txt redirfd -w 2 errors.txt diff sortjunk.txt sortjunk2.txt }

# Benchmark 3: Compressing Files with tar, gzip, bzip2 

foreground { printf "Benchmark 3A/B/C\n" }
foreground { time -p redirfd -w 1 junk.bz2 redirfd -w 2 errors.txt bzip2 -9 -c junk.txt }
foreground { time -p redirfd -w 1 junk.zip redirfd -w 2 errors.txt gzip -9 -c junk.txt }
foreground { time -p redirfd -w 2 errors.txt tar -czvf junk.tgz junk.txt }



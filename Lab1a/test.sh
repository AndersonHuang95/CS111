touch a.txt
touch b.txt
touch c.txt
./simpsh --rdonly --wronly b.txt 
./simpsh --verbose --rdonly c.txt --wronly b.txt 
./simpsh --rdonly a.txt --wronly b.txt --wronly c.txt --command 0 1 2 ls -al 
./simpsh --verbose --rdonly a.txt --wronly b.txt --wronly c.txt --command 0 1 2 cat a.txt 
./simpsh --rdonly a.txt --wronly b.txt --wronly c.txt --command 0 1 2 cat a.txt \
--command 0 1 2 cat c.txt 

echo "This is file d." > d.txt 
./simpsh --verbose --rdonly a.txt --wronly b.txt --wronly c.txt --command 0 1 2 \
sed 's/This/That/g' d.txt 

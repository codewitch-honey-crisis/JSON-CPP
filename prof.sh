#!/bin/sh
rm ./main
/usr/bin/g++ -Iinclude -pg ./src/main.cpp -o ./main
./main
gprof ./main | more

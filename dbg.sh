#!/bin/sh
rm ./main
/usr/bin/g++ -Wall -Iinclude -g ./src/main.cpp -o ./main
/usr/bin/g++ -Wall -Iinclude -g -S -fverbose-asm ./src/main.cpp -o ./main.S
./main
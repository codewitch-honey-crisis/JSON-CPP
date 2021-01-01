#!/bin/sh
rm ./main
/usr/bin/g++ -Iinclude -Wall -O3 -flto -falign-functions=16 -falign-loops=16 -msse4 ./src/main.cpp -o ./main
/usr/bin/g++ -Iinclude -Wall -O3 -flto -falign-functions=16 -falign-loops=16 -msse4 -S -fverbose-asm ./src/main.cpp -o ./main.S
./main
#!/bin/bash
gcc -O2 main.c -lutil -o stub -Wall
strip stub
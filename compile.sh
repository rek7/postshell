#!/bin/bash
gcc -O2 main.c -lutil -o stub
strip stub

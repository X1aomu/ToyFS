#!/bin/bash
g++ -I. -I.. -c -o filesystem.o ../filesystem.cc
g++ -I. -I.. -c -o disk.o ../disk.cc
g++ -I. -I.. -o testfilesystem testfilesystem.cc filesystem.o disk.o

#!/bin/bash
./nob run || (cc -o nob nob.c && ./nob run)

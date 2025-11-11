#!/bin/bash
# This script will rebuild a Makefile suitable to compile TaHomaCtl

LFMakeMaker -v +f=Makefile -cc='cc -Wall -pedantic -O2' --opts='-lreadline -lhistory' *.c -t=TaHomaCtl > Makefile

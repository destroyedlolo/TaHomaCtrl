# makefile created automaticaly by LFMakeMaker
# LFMakeMaker 1.6 (Apr 26 2025 11:57:56) (c)LFSoft 1997

gotoall: all


#The compiler (may be customized for compiler's options).
cc=cc -Wall -pedantic -O2
opts=

TaHomaCtl.o : TaHomaCtl.c TaHomaCtl.h Makefile 
	$(cc) -c -o TaHomaCtl.o TaHomaCtl.c $(opts) 

TaHomaCtl : TaHomaCtl.o Makefile 
	 $(cc) -o TaHomaCtl TaHomaCtl.o $(opts) 

all: TaHomaCtl 

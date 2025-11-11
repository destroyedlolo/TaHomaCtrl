# makefile created automaticaly by LFMakeMaker
# LFMakeMaker 1.6 (Apr 26 2025 11:57:56) (c)LFSoft 1997

gotoall: all


#The compiler (may be customized for compiler's options).
cc=cc -Wall -pedantic -O2
opts=-lreadline -lhistory $(pkg-config --cflags --libs avahi-client)

AvahiScaning.o : AvahiScaning.c TaHomaCtl.h Makefile 
	$(cc) -c -o AvahiScaning.o AvahiScaning.c $(opts) 

TaHomaCtl.o : TaHomaCtl.c TaHomaCtl.h Makefile 
	$(cc) -c -o TaHomaCtl.o TaHomaCtl.c $(opts) 

TaHomaCtl : TaHomaCtl.o AvahiScaning.o Makefile 
	 $(cc) -o TaHomaCtl TaHomaCtl.o AvahiScaning.o $(opts) 

all: TaHomaCtl 

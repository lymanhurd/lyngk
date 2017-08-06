CXX=g++
CPPFLAGS=-g -Wall
COMMONOBJS=game.o board2d.o chkuser.o file.o gamefile.o lists.o ratings.o sendmail.o strutl.o

all: lyngk

lyngk: lyngk_main.o lyngk.o $(COMMONOBJS)
	$(CXX) $(CPPFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

.PHONY:	clean
clean:
	rm -f yinsh lyngk *.o *~ \#*

.PHONY:	cleangames
cleangames:
	rm -f /home/lhurd/pbmhome/etc/Lyngk.list /home/lhurd/pbmhome/games/Lyngk/*

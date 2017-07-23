CXX=g++
CPPFLAGS=-g -Wall
COMMONOBJS=game.o board2d.o chkuser.o file.o gamefile.o lists.o ratings.o sendmail.o strutl.o

all: yinsh lyngk

yinsh: yinsh_main.o yinsh.o hexhexboard.o $(COMMONOBJS)
	$(CXX) $(CPPFLAGS) $^ -o $@

lyngk: lyngk_main.o lyngk.o $(COMMONOBJS)
	$(CXX) $(CPPFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

.PHONY:	clean
clean:
	rm -f yinsh lyngk *.o *~ \#*

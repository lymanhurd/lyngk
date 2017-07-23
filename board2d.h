#ifndef BOARD2D_H
#define BOARD2D_H

#include <stdio.h>
#include <string.h>

#include "game.h"
#include "gamefile.h"

const int Unused = 0xFFFF; // More than enough

class Board:public ReadWriteStringList {
protected:
	const char *Name(void) { return "board"; }
	int OneLiner(void) { return 0; }
};

class Zero:public ReadWriteIntList {
protected:
	const char *Name(void) { return "zero"; }
};

class Board2D: public Game {
protected:
	Board board;
	Zero zero;

public:
	virtual char Blank(void) { return '.'; }
	virtual int Fixed(void) { return 1; }
	int Init(int h, int w, char hexagonal='\0');
	void Grow(int drow, int dcol);
	void Shrink(int drow, int dcol);
	void MakeBorder(int width);
	virtual int ReadBoard(GameFile &game);
	virtual int WriteBoard(GameFile &game);
	virtual void PrintForHTML(FILE *fp);

	const char *DecodeAN1Quad(const char *move, int &row, int &col,
		int term=1);

	char GetAt(int r, int c)
		{ return !InsideBoard(r,c)?(Fixed()?'\0':Blank()):board[r+zero[0]][c+zero[1]]; }
	void PutAt(int r, int c, char ch)
		{	if (InsideBoard(r,c))
				board[r+zero[0]][c+zero[1]] = ch;
			else if (!Fixed()) {
				if (r < MinRow()) Grow(r-MinRow(),0);
				if (c < MinCol()) Grow(0,c-MinCol());
				if (r > MaxRow()) Grow(r-MaxRow(),0);
				if (c > MaxCol()) Grow(0,c-MaxCol());
				board[r+zero[0]][c+zero[1]] = ch;
			}
		}
	int IsBlank(int r, int c)
		{ return GetAt(r,c) == Blank(); }
	int Width(void)
		{ return strlen(board[0]); }
	int Height(void)
		{ return board.Count(); }

	int InsideBoard(int r, int c)
		{ return r>=MinRow() && r<=MaxRow() && c>=MinCol() && c<=MaxCol(); }
	int MinRow(void)
		{ return -zero[0]; }
	int MaxRow(void)
		{ return Height()-zero[0]-1; }
	int MinCol(void)
		{ return -zero[1]; }
	int MaxCol(void)
		{ return Width()-zero[1]-1; }
};

#endif

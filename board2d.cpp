#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "board2d.h"

int Board2D::Init(int h, int w, char hexagonal)
{
	if (w < 1 || h < 1)
		return 0;
	board.Init();
	char *row = new char[w+1];
	memset(row,Blank(),w);
	row[w] = '\0';
	int r;
	for (r = 0; r < h; r++)
		board.Add(row);
	delete [] row;

	if (hexagonal) {
		int i,j;
		for (i=0 ; i<h/2 ; i++)
			for (j=0 ; j<h/2-i ; j++)
				board[i][w-j-1] = board[h-i-1][j] = hexagonal;
	}

	zero.Init();
	zero.Add(0);
	zero.Add(0);

	return 1;
}

void Board2D::Grow(int drow, int dcol)
{
	if (Fixed())
		return; // We should actually warn here
	// If drow < 0 expand up, if drow > 0 expand down.
	// If dcol < 0 expand left, if dcol > 0 expand right
	// Make sure that we keep 0,0 in the correct place.
	if (dcol) {
		int w = Width();
		int len = w+ABS(dcol);
		char *row = new char[len+1];
		memset(row,Blank(),len);
		row[len] = '\0';
		int r;
		for (r = 0; r < Height(); r++) {
			memcpy(row-(dcol<0?dcol:0), board[r], w);
			board.Change(r,row);
		}
		delete [] row;
		if (dcol < 0)
			zero[1] -= dcol;
	}
	if (drow) {
		int len = Width();
		char *row = new char[len+1];
		memset(row,Blank(),len);
		row[len] = '\0';
		if (drow > 0) {
			while (drow--)
				board.Add(row);
		} else {
			zero[0] -= drow;
			while (drow++)
				board.Prepend(row);
		}
		delete [] row;
	}
}

void Board2D::Shrink(int drow, int dcol)
{
	if (Fixed())
		return; // We should actually warn here
	// If drow < 0 shrink top, if drow > 0 shrink bottom.
	// If dcol < 0 shrink left, if dcol > 0 shrink right
	// Make sure that we keep 0,0 in the correct place.
	if (dcol) {
		int w = Width();
		int len = w-ABS(dcol);
		char *row = new char[len+1];
		row[len] = '\0';
		int r;
		for (r = 0; r < Height(); r++) {
			memcpy(row, board[r]-(dcol<0?dcol:0), len);
			board.Change(r,row);
		}
		delete [] row;
		if (dcol < 0)
			zero[1] += dcol;
	}
	if (drow) {
		if (drow > 0) {
			while (drow--)
				board.DeleteIndex(Height()-1);
		} else {
			zero[0] += drow;
			while (drow++)
				board.DeleteIndex(0);
		}
	}
}

void Board2D::MakeBorder(int width)
{
	if (Fixed())
		return; // We should actually warn here
	int border;
	// Ensure that we have a blank border of exactly "width" positions on
	// all sides
	char *row = new char[Width()+1];
	memset(row,Blank(),Width());
	row[Width()] = '\0';
	// Check top
	for (border = 0; border < Height(); border++)
		if (strcmp(row, board[border]))
			break;
	// border is first non-blank row
	if (border < width)
		Grow(border-width,0);
	else if (border > width)
		Shrink(width-border,0);
	// Check bottom
	for (border = 0; border < Height(); border++)
		if (strcmp(row, board[Height()-border-1]))
			break;
	// border is first non-blank row
	if (border < width)
		Grow(width-border,0);
	else if (border > width)
		Shrink(border-width,0);
	delete [] row;
	// Check left
	border=Width();
	int r,c;
	for (r = 0; r < Height(); r++) {
		row = board[r];
		for (c = 0; c < border; c++)
			if (*row++ != Blank())
				break;
		if (border > c) {
			border = c;
			if (border == 0)
				break;
		}
	}
	if (border < width)
		Grow(0,border-width);
	else if (border > width)
		Shrink(0,width-border);
	// Check right
	border=Width();
	for (r = 0; r < Height(); r++) {
		row = board[r]+Width()-1;
		for (c = 0; c < border; c++)
			if (*row-- != Blank())
				break;
		if (border > c) {
			border = c;
			if (border == 0)
				break;
		}
	}
	if (border < width)
		Grow(0,width-border);
	else if (border > width)
		Shrink(0,border-width);
}

int Board2D::ReadBoard(GameFile &game)
{
#if 1
	int br = board.Read(game);
	int zr = 1;
	if (!Fixed()) zero.Read(game);
	return br*zr;
#else
	return board.Read(game) || zero.Read(game);
#endif
}

int Board2D::WriteBoard(GameFile &game)
{
	int result = board.Write(game);
	if (zero[0] != 0 || zero[1] != 0)
		result *= zero.Write(game);
	return result;

}

// DecodeAN1Quad is a general routine for parsing coordinates in one
// quadrant (input, both positive;  internal, both nonegative).
// parameter term is optional;  default (term = 1) requires the coordinate
// be a complete move;  term = 0 parses coordinates embedded in compound moves.
//
const char *Board2D::DecodeAN1Quad(const char *move, int &row, int &col, int term)
{
	static char newmove[20];

	int len = DecodeLetters(move,col);
	int ix = len;
	len = DecodeDigits(move+ix,row);
	if (ix == 0) { // DecodeLetters failed
		if (len == 0) // Decode Digits failed
			return (char*)Error("Invalid move \"%s\" entered",move);
		ix += len;
		len = DecodeLetters(move+ix,col);
	}
	if (len == 0 || (term && move[ix+len] != '\0')) // Last Decode failed or string too long
		return (char*)Error("Invalid move \"%s\" entered",move);

	// Re-create the move in newmove in canonical form
	strcpy(newmove, EncodeLetters(col));
	strcat(newmove, EncodeDigits(row));

	// translate between external origin 1 and internal origin 0
	row -= 1;
	col -= 1;

	return newmove;
}

void Board2D::PrintForHTML(FILE *fp)
{

  fprintf(fp,"html print begin:\n");
  fprintf(fp,"board\n");
  fprintf(fp,"%i\n",MaxRow()-MinRow()+1);
  for (int i=MinRow(); i<=MaxRow(); i++) {
    for (int j=MinCol(); j<=MaxCol(); j++)
      fprintf(fp,"%c",GetAt(i,j));
    fprintf(fp,"\n");
  }
}

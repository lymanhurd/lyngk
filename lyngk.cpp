//
// Lyngk implementation by Lyman P. Hurd
// gamerz user id lyman
//
//
// Lyngk client version 1.00 updated 7/17/2017 LPH

#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#include <assert.h>
#include <sys/types.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "lyngk.h"
#include "sendmail.h"
#include "strutl.h"

#define MAX_STACK 5
#define NUM_COLS 9
#define NUM_ROWS 7
#define EMPTY '.'
#define JOKER 'J'

#define NUM_LINES 21
#define LINE_LENGTH 7

const char* COLOR_LETTERS = "-IBRGKJ";

// Initialize game.
int Lyngk::Init() {
  player_colors_.Init().Add(0).Add(0).Add(0).Add(0);
  stacks_.Init().Add(0).Add(0);
  bool stack6_mode = false;
  for (int i = 0 ; i < parameters.Count(); i++) {
    if (abbrev(parameters[i], "-stack6", 2)) {
      stack6_mode = true; 
    } else {
      return Error("Invalid %s parameter '%s'", GameType(), parameters[i]);
    }
  }
  game_mode_.Init().Add(stack6_mode);

  if (players.Count() != 2) {
    return Error("%s is a two player game.  %d cannot play.",
		 GameType(), players.Count());
  }

  // Set up board.  The board is roughly star-shaped but since it uses
  // a different notation than, Yinsh, for example (more like Zertz), we
  // do not use hexhexboard as a base.
  Board2D::Init(NUM_ROWS, NUM_COLS * MAX_STACK);
  int pieces[43]; 
  int idx = 0;
  for (int color = 0; color < 5; color++) {
    for (int count = 0; count < 8; count++) {
      pieces[idx++] = 1 + color;
    }
  }

  for (int count = 0; count < 3; count++) {
    pieces[idx++] = 6;
  }

  idx = 0;
  // shuffle pieces
  ShufflePieces(pieces, sizeof(pieces)/sizeof(int));
  for (int row = 0; row < NUM_ROWS; row++) {
    for (int col = 0; col < NUM_COLS; col++) {
      if (OnBoard(row, col)) {
	PutAt(row, MAX_STACK * col,
	      COLOR_LETTERS[pieces[idx++]]);
      }
    }
  }
  return 1;
}

void Lyngk::ShufflePieces(int* array, int len) {
  while (len > 1) {
    int idx = ::random() % len;
    int t = array[len - 1];
    array[len - 1] = array[idx];
    array[idx] = t;
    len--;
  }
}

// Routines that serialize and deserialize the state of the game.
// The state consists of the board position, as well as the list
// of colors (up to two) claimed by each player as well as whether
// the game is being played with the six stack optional rule (not
// yet implemented).

// Read in the current game state.
int Lyngk::ReadBoard(GameFile &game) {
  int result = Board2D::ReadBoard(game);
  result *= player_colors_.Read(game);
  result *= stacks_.Read(game);
  result *= game_mode_.Read(game);
  return result;
}

// Write the game state.
int Lyngk::WriteBoard(GameFile &game) {
  int result = Board2D::WriteBoard(game);
  result *= player_colors_.Write(game);
  result *= stacks_.Write(game);
  result *= game_mode_.Write(game);
  return result;
}

// Routines to implement performing a move.  A move has two parts.
//
// First the player may optionally claim a color if they have not
// already claimed two colors.  Only the five basic colors may be
// claimed and a player cannot claim a previously claimed color.
//
// Next the player specifies the location of a source and destination
// stack and the source stack is placed on top of the destinaion stack.
// The two stacks must be in a straight line or connected via a Lyngk
// move explaned below.  Furthermore, the combined stack cannot exceed
// height 5 (if the base game) and colors cannot be duplicated except
// that jokers can represent any color not already present and a stack
// can have multiple jokers.

const char* COLOR_NAMES[] = {"None", "Ivory", "Blue", "Red",
			     "Green","blacK", "Joker"};

// MakeMove - the input move is reformatted into the array newmove.
const char *Lyngk::MakeMove(const char *move) {
  static char new_move[8];
  strncpy(new_move, move, 8);
  char claimed = EMPTY;
  char src_col_char;
  char src_row_char;
  char dest_col_char;
  char dest_row_char;
  if (sscanf(move, "%c,%c%c-%c%c", &claimed, &src_col_char, &src_row_char,
	     &dest_col_char, &dest_row_char) == 5) {
    claimed = toupper(claimed);
    src_col_char = tolower(src_col_char);
    dest_col_char = tolower(dest_col_char);
    sprintf(new_move, "%c,%c%c-%c%c", claimed, src_col_char, src_row_char, dest_col_char, dest_row_char);

    if (claimed == 'J' || claimed == '-' || ColorIndex(claimed) == -1) {
      return (char*) Error("Invalid claim.  Color %c is not in (IBRGK).", claimed);
    }
    if (ClaimColor(claimed) == 0) {
      return 0;
    }
  } else if (sscanf(move, "%c%c-%c%c", &src_col_char, &src_row_char, &dest_col_char, &dest_row_char) == 4) {
    src_col_char = tolower(src_col_char);
    dest_col_char = tolower(dest_col_char);
    sprintf(new_move, "%c%c-%c%c", src_col_char, src_row_char, dest_col_char, dest_row_char);
  } else {
    return (char*) Error("Format is: [ClaimedColor,]RowCol-RowCol");
  }
  int result = MoveStack(CurrentPlayer(), src_row_char - '1', src_col_char- 'a', dest_row_char - '1',
			 dest_col_char - 'a');
  if (result == 0) {
    return 0;
  }  
  return new_move;
}

// The game ends when neither player can move.
int Lyngk::IsGameOver(const char *&winner) {
  if (CannotMove(0) && CannotMove(1)) {
    int winning_player = GetWinner();
    if (winning_player == -1) { // draw
      winner = 0;
    } else {
      winner = players[winning_player];
    }
    return 1;
  }
  return 0;
}

// The winner is the person who has the most stacks of 5.  If that is
// a tie look at stacks of 4 etc.  If all stack sizes are tied, the game
// is a tie.
int Lyngk::GetWinner() {
  if (stacks_[0] != stacks_[1]) {
    return stacks_[0] > stacks_[1] ? 0 : 1;
  }
  for (int height = 4; height > 0; --height) {
    int s0 = StackCount(0, height);
    int s1 = StackCount(1, height);
    if (s0 != s1) {
      return s0 > s1 ? 0 : 1;
    }
  }
  return -1;
}

// TODO(lyman): Implement.
int Lyngk::StackCount(int player, int height) {
  return 0;
}

bool Lyngk::CannotMove(int player) {
  // Try every possible combination of moves.
  for (int src_row = 0; src_row < NUM_ROWS; src_row++) {
    for (int src_col = 0; src_col < NUM_COLS; src_col++) {
      if (OnBoard(src_row, src_col)) {
	for (int dest_row = 0; dest_row < NUM_ROWS; dest_row++) {
	  for (int dest_col = 0; dest_col < NUM_COLS; dest_col++) {
	    if (OnBoard(dest_row, dest_col)) {
	      // Use special function that allows a player to claim an unclaimed stack.
	      if (CanMoveStack(player, src_row, src_col, dest_row, dest_col, true)) {
		return false;
	      }
	    }
	  }
	}
      }
    }
  }
  return true;
}

// Utility functions.
int COLUMN_HEIGHTS[NUM_COLS] = {1, 4, 7, 6, 7, 6, 7, 4, 1};

int Lyngk::OnBoard(int row, int col) {
  return row < COLUMN_HEIGHTS[col];
}

//----------
// MustSkip
//  Detects forced passes
//----------
int Lyngk::MustSkip() {
  return CannotMove(CurrentPlayer()); 
};

int Lyngk::StackHeight(int row, int col) {
  if (!OnBoard(row, col)) {
    return -1;
  }
  int stack_height = 0;
  for (int i = 0; i < MAX_STACK; i++) {
    if (GetAt(row, col * MAX_STACK + i) != EMPTY) {
      stack_height++;
    }
  }
  return stack_height;
}

void Lyngk::ClaimStack(int player, int row, int col) {
  if (StackHeight(row, col) == MAX_STACK && StackOwner(row, col) == player) {
    stacks_[player] += 1;
    for (int i = 0; i < MAX_STACK; i++) {
      PutAt(row, col*MAX_STACK + i, EMPTY);
    }
  }
}

int Lyngk::CanMoveStack(int player,
			int src_row,
			int src_col,
			int dest_row,
			int dest_col,
			bool test_mode) {
  int src_height = StackHeight(src_row, src_col);
  int dest_height = StackHeight(dest_row, dest_col);
  char base_error[26];
  sprintf(base_error, "Cannot move stack from %c%c to %c%c",
	  'a' + src_col,
	  '1' + src_row,
	  'a' + dest_col,
	  '1' + dest_row);

  // check stack heights
  if (src_height + dest_height > MAX_STACK) {
    if (test_mode) {
      return 0;
    } else {
      return Error("%s: %s", base_error, "Combined stack exceeds maximum stack height.");
    }
  }
  if (src_height  == 0 || dest_height == 0) {
    if (test_mode) {
      return 0;
    } else {
      return Error("%s: %s", base_error, "Source and desination stacks cannot be empty.");
    }
  }
  int owner = StackOwner(src_row, src_col);

  if (owner == -1) {
    // When checking if a player has any legal moves, we check if claiming a color would
    // make a move legal.
    bool bypass_check = test_mode && player_colors_[2 * CurrentPlayer() + 1] == 0;
    if (src_height < dest_height && !bypass_check) {
      if (test_mode) {
	return 0;
      } else {
	return Error("%s: %s", base_error, "Neutral stacks can only be moved onto equal or smaller stacks.");
      }
    }
  } else if (owner != player) {
    if (test_mode) {
      return 0;
    } else {
      return Error("%s: %s", base_error, "Source stack belongs to your opponent.");
    }
  }
  // check stack contents
  for (int i = 0; i < src_height; i++) {
    char marker = GetAt(src_row, src_col * MAX_STACK + i);
    if (marker != JOKER && marker != EMPTY) {
      for (int j = 0; j < dest_height; j++) {
	if (marker == GetAt(dest_row, dest_col * MAX_STACK + j)) {
	  if (test_mode) {
	    return 0;
	  } else {
	    return Error("%s: %s", base_error, "Combined stack has duplicate colors.");
	  }
	}
      }
    }
  }
  if (!InUnobstructedLine(src_row, src_col, dest_row, dest_col)) {
    if (owner != player) {
      if (test_mode) {
	return 0;
      } else {
	return Error("%s: %s", base_error,
		     "Source and destination are not connected by an unobstructed line.");
      }
    } else if (!ConnectedByLyngkMove(src_col, src_col, dest_row, dest_col)) {
      if (test_mode) {
	return 0;
      } else {
	return Error("%s: %s", base_error,
		     "Source and destination are not connected by an unobstructed line"
		     " or Lyngk move.");
      }
    }
  }
  return 1;
}

// Expressed in the order (row, col) and indexed at 0 so that, for example,
// {0, 1} = b1 and {3, 6} = g4.
int LINES[NUM_LINES][LINE_LENGTH][2] = 
  {{{0, 1}, {1, 1}, {2, 1}, {3, 1}, {-1, -1}, {-1, -1}, {-1, -1}},
   {{0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2}, {6, 2}},
   {{0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3}, {-1, -1}},
   {{0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 4}},
   {{0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 5}, {-1, -1}},
   {{0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}},
   {{0, 7}, {1, 7}, {2, 7}, {3, 7}, {-1, -1}, {-1, -1}, {-1, -1}},
   {{0, 4}, {0, 5}, {1, 6}, {0, 7}, {-1, -1}, {-1, -1}, {-1, -1}},
   {{0, 2}, {0, 3}, {1, 4}, {1, 5}, {2, 6}, {1, 7}, {0, 8}},
   {{1, 2}, {1, 3}, {2, 4}, {2, 5}, {3, 6}, {2, 7}, {-1, -1}},
   {{0, 1}, {2, 2}, {2, 3}, {3, 4}, {3, 5}, {4, 6}, {3, 7}},
   {{1, 1}, {3, 2}, {3, 3}, {4, 4}, {4, 5}, {5, 6}, {-1, -1}},
   {{0, 0}, {2, 1}, {4, 2}, {4, 3}, {5, 4}, {5, 5}, {6, 6}},
   {{3, 1}, {5, 2}, {5, 3}, {6, 4}, {-1, -1}, {-1, -1}, {-1, -1}},
   {{0, 1}, {1, 2}, {0, 3}, {0, 4}, {-1, -1}, {-1, -1}, {-1, -1}},
   {{0, 0}, {1, 1}, {2, 2}, {1, 3}, {1, 4}, {0, 5}, {0, 6}},
   {{2, 1}, {3, 2}, {2, 3}, {2, 4}, {1, 5}, {1, 6}, {-1, -1}},
   {{3, 1}, {4, 2}, {3, 3}, {3, 4}, {2, 5}, {2, 6}, {0, 7}},
   {{5, 2}, {4, 3}, {4, 4}, {3, 5}, {3, 6}, {1, 7}, {-1, -1}},
   {{6, 2}, {5, 3}, {5, 4}, {4, 5}, {4, 6}, {2, 7}, {0, 8}},
   {{6, 4}, {5, 5}, {5, 6}, {3, 7}, {-1, -1}, {-1, -1}, {-1, -1}}};

bool Lyngk::InUnobstructedLine(int src_row, int src_col, int dest_row, int dest_col) {
  if (src_row == dest_row && src_col == dest_col) {
    return false;
  }
  for (unsigned int line = 0; line < NUM_LINES; line++) {
    bool started = false;
    for (unsigned int point = 0; point < LINE_LENGTH; point++) {
      if (LINES[line][point][0] == -1) {
	continue;
      }
      // This if statement means we have found either the source or the destination in a line.
      if ((LINES[line][point][0] == src_row && LINES[line][point][1] == src_col) ||
	  (LINES[line][point][0] == dest_row && LINES[line][point][1] == dest_col)) {
	// If this is the second endpoint and we have not yet broken out of the loop,
	// return true.
	if (started) {
	  return true;
	} else {
	  started = true;
	}
      } else if (started && GetAt(LINES[line][point][0],
				  LINES[line][point][1] * MAX_STACK) != EMPTY) {
	// If we have started and not reached the finish, all intervening intersections
	// must be empty.
	started = false;
	break;
      }
    }
  }
  return false;
}

bool Lyngk::ConnectedByLyngkMove(int src_row, int src_col, int dest_row, int dest_col) {
  return false;
}

int Lyngk::MoveStack(int player,
		     int src_row,
		     int src_col,
		     int dest_row,
		     int dest_col) {
  if (CanMoveStack(player, src_row, src_col, dest_row, dest_col) == 0) {
    return 0;
  }
  int src_height = StackHeight(src_row, src_col);
  int dest_height = StackHeight(dest_row, dest_col);
  // move tokens in destination to make room for the src.
  for (int i = dest_height - 1; i >= 0; i--) {
    PutAt(dest_row, dest_col * MAX_STACK + i + src_height,
	  GetAt(dest_row, dest_col * MAX_STACK + i));
  }
  // Copy the source markers over and remove them from source.
  for (int i = 0; i < src_height; i++) {
    PutAt(dest_row, dest_col * MAX_STACK + i,
	  GetAt(src_row, src_col * MAX_STACK + i));
    PutAt(src_row, src_col * MAX_STACK + i, EMPTY);
  }
  // Remove a stack of if appropriate.
  ClaimStack(player, dest_row, dest_col);
  return true;
}

int Lyngk::ClaimColor(char color) {
  if (player_colors_[2 * CurrentPlayer() + 1]) {
    return Error("You have already claimed two colors.");
  }
  int idx = ColorIndex(color);
  if (player_colors_[0] == idx ||
      player_colors_[1] == idx ||
      player_colors_[2] == idx ||
      player_colors_[3] == idx) {
    return Error("Color %s has already been claimed.", COLOR_NAMES[idx]);
  }
  if (player_colors_[2 * CurrentPlayer()] == 0) {
    player_colors_[2 * CurrentPlayer()] = idx;
  } else {
    player_colors_[2 * CurrentPlayer() + 1] = idx;
  }
  // Update to claim any stacks of five with this color. 
  for (int row = 0; row < NUM_ROWS; row++) {
    for (int col = 0; col < NUM_COLS; col++) {
      ClaimStack(CurrentPlayer(), row, col);
    }
  }
  return 1;
}

int Lyngk::ColorIndex(char color) {
  int idx = -1;
  const char* ptr = strchr(COLOR_LETTERS, color);
  if (ptr) {
    idx = ptr - COLOR_LETTERS;
  }
  return idx;
}

int Lyngk::StackOwner(int row, int col) {
  int idx = ColorIndex(GetAt(row, col * MAX_STACK));
  if (idx == player_colors_[0] || idx == player_colors_[1]) {
    return 0;
  } else if (idx == player_colors_[2] || idx == player_colors_[3]) {
    return 1;
  } else {
    return -1;
  }
}

//    c7 e7 g7
//      d6 f6
//    c6 e6 g6
//   b4 d5 f5 h4
//    c5 e5 g5
//   b3 d4 f4 h3
// a1 c4 e4 g4 i1
//   b2 d3 f3 h2
//    c3 e3 g3
//   b1 d2 f2 h1
//    c2 e2 g2
//      d1 f1
//    c1 e1 g1

// PrintBoard.
void Lyngk::PrintBoard(FILE *fp) {
  PrintBoardASCII(fp);
}

void Lyngk::PrintBoardASCII(FILE *fp) {
  if (game_mode_[0]) {
    fprintf(fp, "Stack 6 Mode\n");
  }
  fprintf(fp, "Available Colors: (%s)\n", AvailableColors());
  fprintf(fp, "%s: %d stacks (%s) claimed.\n", players[0], stacks_[0],
	  ClaimedColors(0));
  fprintf(fp, "%s: %d stacks (%s) claimed.\n\n", players[1], stacks_[1],
	  ClaimedColors(1));
  fprintf(fp, "   A   B   C   D   E   F   G   H   I\n");
  fprintf(fp, "   1   4   7   6   7   6   7   4   1\n\n");
  for (int i = 0; i < 13; i++) {
    PrintDisplayRow(fp, i);
  }
  fprintf(fp, "\n");
  fprintf(fp, "   A   B   C   D   E   F   G   H   I\n");
  fprintf(fp, "   1   1   1   1   1   1   1   1   1\n");
}

int DISPLAY_TABLE[13][5][2] = {{{-1, -1}, {6, 2}, {6, 4}, {6, 6},   {-1, -1}},
			       {{-1, -1}, {5, 3}, {5, 5}, {-1, -1}, {-1, -1}},
			       {{-1, -1}, {5, 2}, {5, 4}, {5, 6},   {-1, -1}},
			       {{3, 1},   {4, 3}, {4, 5}, {3, 7},   {-1, -1}},
			       {{-1, -1}, {4, 2}, {4, 4}, {4, 6},   {-1, -1}},
			       {{2, 1},   {3, 3}, {3, 5}, {2, 7},   {-1, -1}},
			       {{0, 0},   {3, 2}, {3, 4}, {3, 6},   {0, 8}},
			       {{1, 1},   {2, 3}, {2, 5}, {1, 7},   {-1, -1}},
			       {{-1, -1}, {2, 2}, {2, 4}, {2, 6},   {-1, -1}},
			       {{0, 1},   {1, 3}, {1, 5}, {0, 7},   {-1, -1}},
			       {{-1, -1}, {1, 2}, {1, 4}, {1, 6},   {-1, -1}},
			       {{-1, -1}, {0, 3}, {0, 5}, {-1, -1}, {-1, -1}},
			       {{-1, -1}, {0, 2}, {0, 4}, {0, 6},   {-1, -1}}};

// Print each stack in a field of width 7.
void Lyngk::PrintDisplayRow(FILE* fp, int display_row) {
  if (display_row % 2) {
    fprintf(fp, "    ");  // 4 spaces offset for every other row
  }
  for (int display_col = 0; display_col < 5; display_col++) {
    int row = DISPLAY_TABLE[display_row][display_col][0];
    int col = DISPLAY_TABLE[display_row][display_col][1];
    if (row == -1) {
      fprintf(fp, "        ");  // 8 spaces
    } else {
      int h = StackHeight(row, col);
      if (h == 0) { // treat empty intersection separately
	fprintf(fp, "   -    ");
      } else {
	for (int i = 0; i < (7 - h + 1) / 2; i++) {  // padding to 7 bytes rounding down
	  fprintf(fp, " ");
	}
	for (int i = 0; i < h; i++) {
	  fprintf(fp, "%c", GetAt(row, col * MAX_STACK + i));
	}
	for (int i = 0; i <= (7 - h) / 2; i++) {  // the other side rounds up
	  fprintf(fp, " ");
	}
      }
    }
  }
  fprintf(fp, "\n");
}

const char* Lyngk::ClaimedColors(int player) {
  static char claim[12];
  if (player_colors_[2 * player] == 0) {
    return "";
  } else if (player_colors_[2 * player + 1] == 0) {
    sprintf(claim, "%s", COLOR_NAMES[player_colors_[2 * player]]);
    return claim;
  } else {
    sprintf(claim, "%s,%s", COLOR_NAMES[player_colors_[2 * player]],
	    COLOR_NAMES[player_colors_[2 * player + 1]]);
    return claim;
  }
};

const char* Lyngk::AvailableColors() {
  static char colors[21];
  colors[0] = '\0';
  for (int i = 1; i < 6; i++) {
    bool found = false;
    for (int j = 0; j < 4; j++) {
      if (player_colors_[j] == i) {
	found = true;
      }
    }
    if (!found) {
      if (colors[0] == '\0') {
	strncat(colors, COLOR_NAMES[i], 8);
      } else {
	strncat(colors, ",", 1);
	strncat(colors, COLOR_NAMES[i], 8);
      }
    }
  }
  return colors;
}

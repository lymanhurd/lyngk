//
// Lyngk implementation by Lyman P. Hurd
// gamerz user id lyman
//
//
// Lyngk client version 1.00 updated 7/17/2017 LPH

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

int COLUMN_HEIGHTS[NUM_COLS] = {1, 4, 7, 6, 7, 6, 7, 4, 1};
const char* COLOR_NAMES[] = {"None", "Ivory", "Blue", "Red",
			     "Green","blacK", "Joker"};
const char* COLOR_LETTERS = "-IBRGKJ";

void Lyngk::shuffle(int* array, int len) {
  while (len > 1) {
    int idx = ::random() % len;
    int t = array[len - 1];
    array[len - 1] = array[idx];
    array[idx] = t;
    len--;
  }
}

// Read in the game state.
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

// MakeMove - the input move is reformatted into the array newmove.
const char *Lyngk::MakeMove(const char *move) {
  static char new_move[8];
  strncpy(new_move, move, 8);
  char claimed = EMPTY;
  char src_col;
  char src_row;
  char dest_col;
  char dest_row;
  if (sscanf(move, "%c,%c%c-%c%c", &claimed, &src_col, &src_row, &dest_col, &dest_row) == 5) {
    claimed = toupper(claimed);
    src_col = tolower(src_col);
    dest_col = tolower(dest_col);
    sprintf(new_move, "%c,%c%c-%c%c", claimed, src_col, src_row, dest_col, dest_row);
    if (!claim_color(claimed)) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
      return (char*) Error("Color %s cannot be claimed right now.", COLOR_NAMES[color_index(claimed)]);
    }
  } else if (sscanf(move, "%c%c-%c%c", &src_col, &src_row, &dest_col, &dest_row) == 4) {
    src_col = tolower(src_col);
    dest_col = tolower(dest_col);
    sprintf(new_move, "%c%c-%c%c", src_col, src_row, dest_col, dest_row);
  } else {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    return (char*) Error("Format is: [ClaimedColor,]RowCol-RowCol");
  }
  if (!move_stack(src_row - '1', src_col- 'a', dest_row - '1', dest_col - 'a')) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    return (char*) Error("Cannot move stack from %c%c to %c%c.", src_col, src_row, dest_col, dest_row);
  }    
  return new_move;
  // return (char *) new_move;
}

// The game ends when neither player can move.
int Lyngk::IsGameOver(const char *&winner) {
  return CannotMove(CurrentPlayer()) && CannotMove(1 - CurrentPlayer());
}

int Lyngk::OnBoard(int row, int col) {
  return row < COLUMN_HEIGHTS[col];
}

// Initialize game.

int Lyngk::Init() {
  player_colors_.Init().Add(0).Add(0).Add(0).Add(0);
  stacks_.Init().Add(0).Add(0);
  bool stack6_mode = false;
  for (int i = 0 ; i < parameters.Count(); i++) {
    if (abbrev(parameters[i], "-stack6", 2)) {
      stack6_mode = true; 
    } else {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
      return Error("Invalid %s parameter '%s'", GameType(), parameters[i]);
    }
  }
  game_mode_.Init().Add(stack6_mode);

  if (players.Count() != 2) {
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
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
  shuffle(pieces, sizeof(pieces)/sizeof(int));
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

// PrintBoard.
void Lyngk::PrintBoard(FILE *fp) {
  if (game_mode_[0]) {
    fprintf(fp, "Stack 6 Mode\n");
  }
  fprintf(fp, "Available Colors: (%s)\n", available());
  fprintf(fp, "%s: %d stacks (%s) claimed.\n", players[0], stacks_[0],
	  claimed(0));
  fprintf(fp, "%s: %d stacks (%s) claimed.\n\n", players[1], stacks_[1],
	  claimed(1));
  Board2D::PrintForHTML(fp);
}

bool Lyngk::CannotMove(int player) {
  return false;
}

//----------
// MustSkip
//  Detects forced passes
//----------
int Lyngk::MustSkip() {
  return CannotMove(CurrentPlayer()); 
};

// Claiming colors...
const char* Lyngk::claimed(int player) {
  static char claim[12];
  if (player_colors_[2*player] == 0) {
    return "";
  } else if (player_colors_[2*player+1] == 0) {
    sprintf(claim, "%s", COLOR_NAMES[player_colors_[2*player]]);
    return claim;
  } else {
    sprintf(claim, "%s,%s", COLOR_NAMES[player_colors_[2*player]],
	    COLOR_NAMES[player_colors_[2*player+1]]);
    return claim;
  }
};

const char* Lyngk::available() {
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

int Lyngk::height(int row, int col) {
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

bool Lyngk::player_owns(int row, int col) {
  char top = GetAt(row, col * MAX_STACK);
  return top == COLOR_LETTERS[player_colors_[2 * CurrentPlayer()]] ||
    top == COLOR_LETTERS[player_colors_[2 * CurrentPlayer() + 1]];
}

bool Lyngk::claim_stack(int row, int col) {
  if (height(row, col) == MAX_STACK && player_owns(row, col)) {
    stacks_[CurrentPlayer()] += 1;
    for (int i = 0; i < MAX_STACK; i++) {
      PutAt(row, col*MAX_STACK + i, EMPTY);
    }
    return true;
  }
  return false;
}

bool Lyngk::move_stack(int src_row, int src_col,
		       int dest_row, int dest_col) {
  int src_height = height(src_row, src_col);
  int dest_height = height(dest_row, dest_col);
  // check stack heights
  if (src_height + dest_height > MAX_STACK) {
    return false;
  }
  if (src_height  == 0 ||  dest_height == 0) {
    return false;
  }
  // check stack contents
  for (int i = 0; i < src_height; i++) {
    char marker = GetAt(src_row, src_col * MAX_STACK + i);
    if (marker != JOKER && marker != EMPTY) {
      for (int j = 0; j < dest_height; j++) {
	if (marker == GetAt(dest_row, dest_col * MAX_STACK + j)) {
	  return false;
	}
      }
    }
  }
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
  claim_stack(dest_row, dest_col);
  return true;
}

bool Lyngk::claim_color(char color) {
  if (player_colors_[2 * CurrentPlayer() + 1]) {
    return false;  // player has already claimed two colors
  }
  int idx = color_index(color);
  if (player_colors_[2 * CurrentPlayer()] == idx ||
      player_colors_[2 * (1 - CurrentPlayer())] == idx ||
      player_colors_[2 * (1 - CurrentPlayer()) + 1] == idx) {
      return false;  // color has already been claimed
  }
  if (player_colors_[2 * CurrentPlayer()] == 0) {
    player_colors_[2 * CurrentPlayer()] = idx;
  } else {
    player_colors_[2 * CurrentPlayer() + 1] = idx;
  }
  return true;
}

int Lyngk::color_index(char color) {
  int idx = 0;
  const char* ptr = strchr(COLOR_LETTERS, color);
  if (ptr) {
    idx = ptr - COLOR_LETTERS;
  }
  return idx;
}

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

#define NUM_COLS 9
#define NUM_ROWS 7
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

int Lyngk::WriteBoard(GameFile &game) {
  int result = Board2D::WriteBoard(game);
  result *= player_colors_.Write(game);
  result *= stacks_.Write(game);
  result *= game_mode_.Write(game);
  return result;
}

// MakeMove - the input move is reformatted into the array newmove.

const char *Lyngk::MakeMove(const char *move) {
  static char new_move[128];
  new_move[0] = '\0';
  return (char *) new_move;
}

// The game ends when neither player can move.

int Lyngk::IsGameOver(const char *&winner) {
  return CannotMove(CurrentPlayer()) && CannotMove(1 - CurrentPlayer());
}

int Lyngk::OnBoard(int row, int col) {
  return row <= COLUMN_HEIGHTS[col - 1];
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
  Board2D::Init(NUM_ROWS, NUM_COLS);
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
  for (int row = 1; row <= NUM_ROWS; row++) {
    for (int col = 1; col <= NUM_COLS; col++) {
      if (OnBoard(row, col)) {
	PutAt(row - 1, col - 1, COLOR_LETTERS[pieces[idx++]]);
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

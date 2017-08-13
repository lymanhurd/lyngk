#ifndef Lyngk_H
#define Lyngk_H

#include "board2d.h"

// Lyngk client version 1.00 updated 7/19/2017 Lymam P. Hurd

#define NUM_ROWS 7
#define NUM_COLS 9

// Colors claimed by the players 4 total (two for each).

class PlayerColors : public ReadWriteIntList {
 protected:
  const char *Name() { return "PlayerColors"; };
};

// Number of stacks removed by each player.
class Stacks : public ReadWriteIntList {
 protected:
  const char *Name() { return "Stacks"; };
};

// Game mode (0 = normal, 1 = 6 stack variant).
class GameMode : public ReadWriteIntList {
 protected:
  const char *Name() { return "GameMode"; };
};

// Main class.

class Lyngk: public Board2D {
 public:
  virtual const char *GameType() { return "Lyngk";}
  virtual int MoveWidth() { return 7;}  // e.g., I,c1-c2
  virtual const char *PlayerNames(int n) { return "";}
  virtual const char *MakeMove(const char *move);
  virtual int IsGameOver(const char *&winner);
  virtual int Init();

  virtual int ReadBoard(GameFile &game);
  virtual int WriteBoard(GameFile &game);
  
  virtual void PrintBoard(FILE *fp);
  virtual int OnBoard(int row, int col);

  // if you cannot move, your turn is skipped   
  virtual int MustSkip();

 private:

  // ASCII Display Routines
  virtual void PrintBoardASCII(FILE *fp);
  void PrintDisplayRow(FILE* fp, int display_row);
  const char* ClaimedColors(int player);
  const char* AvailableColors();

  void ShufflePieces(int* array, int len);
  int StackHeight(int row, int col);

  void ClaimStack(int player, int row, int col);

  // In the case in which we are running this function
  // to determine if a player has any legal move, we
  // allow for the possiblity that they could claim
  // a neutral color.
  int CanMoveStack(int player,
		   int src_row,
		   int src_col,
		   int dest_row,
		   int dest_col,
		   bool can_claim_color=false);

  bool InUnobstructedLine(int src_row, int src_col, int dest_row, int dest_col);

  bool ConnectedByLyngkMove(int src_row, int src_col, int dest_row, int dest_col,
			      bool (&visited)[NUM_ROWS][NUM_COLS]);

  int MoveStack(int player,
		int src_row,
		int src_col,
		int dest_row,
		int dest_col);

  int ClaimColor(char color);

  int ColorIndex(char color);

  int StackOwner(int row, int col);

  bool CannotMove(int player);
  int GetWinner();  // - 1 signifies a draw.
  int StackCount(int player, int height);

  // Player 0 claims player_colors_[0], [1] and Player 1 [2], [3].
  PlayerColors player_colors_;
  Stacks stacks_;

  // Only 0 (normal game) is implemented currently.
  GameMode game_mode_;
};
#endif  // Lyngk_H

#ifndef Lyngk_H
#define Lyngk_H

#include "board2d.h"

// Lyngk client version 1.00 updated 7/19/2017 Lymam P. Hurd

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
  bool ClaimStack(int row, int col);
  bool MoveStack(int src_row, int src_col,
		  int dest_row, int dest_col);
  bool ClaimColor(char color);

  int ColorIndex(char color);

  bool IsStackOwnedByCurrentPlayer(int row, int col);

  bool IsStackNeutral(int row, int col);

  bool CannotMove(int player);

  PlayerColors player_colors_;
  Stacks stacks_;
  GameMode game_mode_;
};
#endif  // Lyngk_H

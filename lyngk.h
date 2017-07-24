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
  virtual int MoveWidth() { return 7;}  // e.g., I,C1-C7
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
  const char* claimed(int player);
  const char* available();
  void shuffle(int* array, int len);
  int height(int row, int col);
  bool player_owns(int row, int col);
  bool claim_stack(int row, int col);
  bool move_stack(int src_row, int src_col,
		  int dest_row, int dest_col);
  bool claim_color(char color);

  int color_index(char color);

  bool CannotMove(int player);

  PlayerColors player_colors_;
  Stacks stacks_;
  GameMode game_mode_;
};
#endif  // Lyngk_H

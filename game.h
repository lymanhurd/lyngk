#ifndef GAME_H
#define GAME_H

#include <time.h>
#include <string.h>

#include "lists.h"
#include "ratings.h"
#include "sendmail.h"

#define ABS(x)   ((x)<0?-(x):(x))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

class Clock:public ReadWriteIntList {
protected:
    const char *Name(void) { return "clock"; }
};

class Timer:public ReadWriteIntList {
protected:
    const char *Name(void) { return "timer"; }
};

class Random:public ReadWriteIntList {
protected:
    const char *Name(void) { return "random"; }
};

class Players:public ReadWriteStringList {
protected:
    const char *Name(void) { return "players"; }
};

class SyncMoves:public ReadWriteStringList {
protected:
    const char *Name(void) { return "syncmoves"; }
    int OneLiner(void) { return 0; }
};

class HasMoved:public ReadWriteIntList {
protected:
    const char *Name(void) { return "hasmoved"; }
};

class Resigned:public ReadWriteIntList {
protected:
    const char *Name(void) { return "resigned"; }
};

class Pending:public ReadWriteIntList {
protected:
    const char *Name(void) { return "pending"; }
};

class Proposed:public ReadWriteIntList {
protected:
    const char *Name(void) { return "proposed"; }
};

class Swapped:public ReadWriteIntList {
protected:
    const char *Name(void) { return "swapped"; }
};

class Moves:public ReadWriteStringList {
protected:
    const char *Name(void) { return "moves"; }
    int OneLiner(void) { return 0; }
};

class Comment:public ReadWriteStringList {
protected:
    const char *Name(void) { return "comment"; }
    int OneLiner(void) { return 0; }
public:
    void Print(FILE *fp);
    void GetNew(const char *userid, int addon=0);
};

class Subscribers:public ReadWriteStringList {
protected:
    const char *Name(void) { return "subscribers"; }
    int OneLiner(void) { return 0; }
public:
    void Print(FILE *fp);
};

class BoardNotifiers:public ReadWriteStringList {
protected:
    const char *Name(void) { return "notifiers"; }
    int OneLiner(void) { return 0; }
public:
    void Print(FILE *fp, int spacer);
};

class Status:public ReadWriteStringList {
protected:
    const char *Name(void) { return "status"; }
public:
    void SetWinner(const char *winner, int points);
    int GetWinner(const char *&winner, int &points);
    int GetTie(void);
    void SetHasLeft(const char *hasleft);
    int GetHasLeft(const char *&hasleft);
    void SetSwapped(const char *swapped);
    int GetSwapped(const char *&swapped);
    void SetProposed(const char *proposed);
    int GetProposed(StringList &who);
    void SetUndo(const char *who);
    int GetUndo(StringList &who);
    void SetNormal(void);
    int GetNormal(void);
    int IsActive(void);
};

class RatingNames:public ReadWriteStringList {
protected:
    const char *Name(void) { return "ratingnames"; }
};

class RatingOld:public ReadWriteIntList {
protected:
    const char *Name(void) { return "ratingold"; }
};

class RatingNow:public ReadWriteIntList {
protected:
    const char *Name(void) { return "ratingnow"; }
};

class Submitted:public ReadWriteStringList {
protected:
    const char *Name(void) { return "submit"; }
};

class Options:public ReadWriteStringList {
protected:
    const char *Name(void) { return "options"; }
};

class Parameters:public ReadWriteStringList {
protected:
    const char *Name(void) { return "parameters"; }
};

class Notifiers:public SingleFileStringList {
protected:
    const char *Name(void) { return "notify"; }
    const char *FileName(void) { return ".notify"; }
};

class TotalTimeLimit:public ReadWriteIntList {
protected:
    const char *Name(void) { return "totaltimelimit"; }
};

class ForfeitTimeLimit:public ReadWriteIntList {
protected:
    const char *Name(void) { return "forfeittimelimit"; }
};

class NagTimeLimit:public ReadWriteIntList {
protected:
    const char *Name(void) { return "nagtimelimit"; }
};


typedef enum {
        AS_UNKNOWN = 0,
        AS_PLAYER = 1,
        AS_SUBSCRIBER = 2,
        AS_NOTIFIER = 3
} ShowAsWhat;

//  changes:
//   1. changed Print to PrintAs, added some arguments
//   2. added inline Print that calls PrintAs
//   3. added inline PrintBoardAs that calls PrintBoard
//  the 'As' versions of the above functions have the following new args:
//      ShowAsWhat what, int which
//  for a player, what = AS_PLAYER -> players[which]
//      (eg. 0 = player 1, white in chess; 1 = player 2, black in chess; etc)
//  for a subscriber, what = AS_SUBSCRIBER -> subscribers[which]
//  for a notify recipient, what = AS_NOTIFIER -> notifiers[which]
//  for others (eg. a 'show'-er), what = 0.
//  ->  naturally this should be used to view the board to the player's
//      perspective, so what = 0, 2 or 3 should view either as players[0]
//      or as players[CurrentPlayer()], up to each game class...
//  override PrintBoardAs to be able to set the perspective; otherwise,
//  for old games, overriding PrintBoard is also supported (but not
//  encouraged for new games)
//  - akur
class Game {
protected:
    virtual const char *PlayerNames(int n) { return "Ohs (o)\0Eks (x)"+n*8; }
    virtual const char PlayerPieces(int n) { return "ox"[n]; }
    virtual int Player(char piece);
    virtual const char *GameType(void) = 0;
    virtual int MinPlayers(void) { return 2; }
    virtual const int MaxPlayers(void) { return 2; }
    virtual const char *SkipMove(void) { return "--"; }
    virtual int isSkipMove(const char *s) { return !strcmp(s,SkipMove()); }
    virtual int SwapAfterMove(void) { return 0; }
    virtual int CanPass(void) { return 0; }
    virtual int CanDraw(void) { return 1; }
    virtual int CanUndo(void) { return 1; }
    virtual int CanPreview(void) { return (SequentialMoves() ? 1 : 0); }
    virtual int PreviewLimit(void) { return 0; }
    virtual int SequentialMoves(void) { return 1; }
    virtual int DuplicateUseridsAllowed(void) { return (SequentialMoves() ? 1 : 0); }
    virtual int MoveWidth(void) { return 9; }
    virtual int PrintColoumns(void) { return 1; }
    virtual int MoveHeight(void) { return 4; }
    virtual int IgnoreCase(void) { return 1; }
    virtual const char *MoveComment(int) { return ""; }

#if defined(WIN32) && !defined(unix)
	virtual int NagTime(void)	{ return 7; }
	virtual int ForfeitTime(void)	{ return 21; }
    virtual int TotalTimeAllowed(void)	{ return 365; }
	int Kill(int forfeit,int who) { return 0; }
#else
	virtual int NagTime(void);
	virtual int ForfeitTime(void);
    virtual int TotalTimeAllowed(void);
	int Kill(int forfeit,int who);
#endif

    virtual int FirstMove(void) { return 0; };
    char *boardno;
    int official_time;
    int previous_moves;
    int print_board_on_error;
    int is_previewing;
    int is_submitting;
    int is_regenerating;
    char *perspective;
    Clock clock;
    Random random;
    Timer timer;
    Players players;
    const char *player;
    int current_player;
    HasMoved hasmoved;
    SyncMoves syncmoves;
    Resigned resigned;
    Resigned oldresigned;
    Pending pending;
    Swapped swapped;
    Moves moves;
    Status status;
    RatingNames ratingnames;
    RatingOld ratingold;
    RatingNow ratingnow;
    Submitted submitted;
    Subscribers subscribers;
	BoardNotifiers boardnotifiers;
    Comment comment;
    Options options;
    Parameters parameters;
    StringList opts;
    StringList parms;
    TotalTimeLimit totaltimelimit;
    ForfeitTimeLimit forfeittimelimit;
    NagTimeLimit nagtimelimit;

    Notifiers notifiers;
    Limits limits;
    FILE *lfp;

    Game()
        {
            official_time = (int)time(NULL);
            print_board_on_error = 0;
            previous_moves = 0;
            is_previewing = 0;
            is_submitting = 0;
            is_regenerating = 0;
            player = NULL;
            current_player = 0;
            perspective = NULL;
            boardno = NULL;
        }
    virtual ~Game()
        { }

    int LockFile(void);
    void UnlockFile(void);

    int Read(char *b);
    int Regenerate(int nmoves);
    int Write(void);
    void Print(FILE *fp, const char *type, int all=0)
        { PrintAs(fp, type, AS_UNKNOWN, 0, all); }
    virtual void PrintAs(FILE *fp, const char *type, int what, int which, int all=0);

    void InitClock(int mtime);
    void UpdateClock(void);
    int TotalTime(void);
    int BoardAge(void);
    void ConvertTime(int timer, int &days, int &hours, int &minutes, int &seconds);

    virtual int NextPlayer(const char *&winner);
public:
    virtual int CurrentPlayer(int rel=0)
        { return players.Count()?(moves.Count()+rel+players.Count())%players.Count():0; }
protected:

    void MassageArgs(int &argc, char **argv);

    int GetOption(char *option,int dflt=0);
    char *GetOption(char *option,char *dflt=NULL);

    void  SetOption(char *option,int value);
    void  SetOption(char *option,char *value);
    void PrintGameListLine(FILE *wp);
    int  RebuildGameList();
    void UpdateGameList(int removeit);

    virtual int isSpecialMove(const char *move) { (void)move; return 0; }
    virtual const char *doSpecialMove(const char *move, const char *who)
                { (void)move; (void)who; return 0; }

    virtual int doResign(const char *who, int forfeit = 0);
    virtual int doMove(const char *move);
    virtual int doSwap(const char *who);
    virtual int doProposeDraw(const char *who);
    virtual int doProposeUndo(const char *who);
    virtual int doAccept(const char *who);
    virtual int doReject(const char *who);
    virtual int doUndo(int numdo=0);

    int Scan(int argc,char **argv,int (Game::*fct)(char *who,FILE *fp),char *who,FILE *fp,StringList &success,StringList &failure,int restrict=0);
    int doNotify(char *who, FILE *fp);
    int doSubscribe(char *who, FILE *fp);
    int doUnsubscribe(char *who, FILE *fp);
    int doShow(char *who, FILE *fp);
    int doGames(char *who, FILE *fp);
    int RebuildActiveCount();
    int doActiveScan(char *who, FILE *fp);
    void UpdateActiveCount(int isactive);

public:
    void GameOver(const char *winner);
    int Main(int argc, char **argv);
    int Challenge(int argc, char **argv);
    int Standings(int argc, char **argv);
    int Show(int argc, char **argv);
    int ShowAs(int argc, char **argv);
    int Resign(int argc, char **argv);
    int Move(char *movetype,int argc, char **argv);
    int Chat(int argc, char **argv);
    int Submit(int argc, char **argv);
    int Confirm(int argc, char **argv);
    virtual int Swap(int argc, char **argv);
    int Preview(int argc, char **argv);
    int History(int argc, char **argv);
    int Games(int argc, char **argv);
    int Help(int argc, char **argv);
    int Subscribe(int argc, char **argv);
    int Unsubscribe(int argc, char **argv);
    int Accept(int argc, char **argv);
    int Reject(int argc, char **argv);
    int Limit(int argc, char **argv);
    int Seek(int argc, char **argv);
    int Notify(int argc, char **argv);   
	int Nag(int who);
    int Cleanup(int argc, char **argv);
    virtual int Replay(int argc, char **argv);
    int Error(const char *msg, ...);
    virtual int UnknownCommand(int argc, char **argv)
        { (void)argc; (void)argv; return Error("Unknown command"); }

// Board specific functions that must be overloaded:
    virtual const char *MakeMove(const char *move) = 0;
    virtual void ProcessRound(void) {};
    virtual int IsGameOver(const char *&winner) = 0;
    virtual int GameValue(const char *winner)
        { (void)winner; return 1; }
    virtual int MustSkip(void)
        { return 0; };
    virtual const char *ForcedMove(void)
        { return NULL; };

    virtual int Init(void) = 0;
    virtual int ReadBoard(GameFile &game) = 0;
    virtual int WriteBoard(GameFile &game) = 0;
    virtual void PrintBoard(FILE *fp) { (void)fp; return; }
    virtual void PrintBoardAs(FILE *fp, int what, int which)
        { (void)what; (void)which; PrintBoard(fp); }
    virtual void PrintForHTML(FILE *fp) { (void)fp; return; }
    virtual void PrepareMove(void)
        { return; }
    virtual int IsRated(void);
    virtual int VisibleMoves(void) { return 1; }

// Helper functions
    int DecodeDigits(const char *s, int &value);
    int DecodeLetters(const char *s, int &value);
    const char *EncodeDigits(int value);
    const char *EncodeLetters(int value, int uppercase=1);

    int DecodeDigitsPosNeg(const char *s, int &value);
    int DecodeLettersPosNeg(const char *s, int &value);
    const char *EncodeDigitsPosNeg(int value);
    const char *EncodeLettersPosNeg(int value, int uppercase=1);
};

#endif

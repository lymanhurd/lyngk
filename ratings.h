#pragma GCC diagnostic ignored "-Wwrite-strings"
#ifndef RATINGS_H
#define RATINGS_H

#include <stdio.h>
#include "lists.h"

#define ACCEPTALL (-1)
#define REJECTALL 0

class Limits {
protected:
        StringList name;
        IntList limit;
	IntList active;
	IntList seek;
	IntList inactiveplayer;
        const char *FileName(void) { return ".limit"; }
        const char *Name(void) { return "limit"; }

public:
        void Init(void)
                { name.Init(); active.Init(); limit.Init(); seek.Init(); inactiveplayer.Init(); }
	void ResetActive(void)
		{ active.Init(name.Count()); }
        void Update(const char *n, int a, int l, int s, int ip);
        void UpdateLimit(const char *n, int l);
        void UpdateActive(const char *n, int a);
	void UpdateSeek(const char *n, int s);
	void UpdateInactivePlayer(const char *n, int ip);
        void DeleteIndex(int ix)
                { name.DeleteIndex(ix); active.DeleteIndex(ix); limit.DeleteIndex(ix); seek.DeleteIndex(ix); inactiveplayer.DeleteIndex(ix); }
	int Limit(const char *n) 
		{ int x = name.Has(n); return(x<0 ? ACCEPTALL : limit[x]); }
	int Active(const char *n)
		{ int x = name.Has(n); return(x<0 ? 0 : active[x]); }
	int Seek(const char *n)
		{ int x = name.Has(n); return(x<0 ? 0 : seek[x]); }
	int InactivePlayer(const char *n)
		{ int x = name.Has(n); return(x<0 ? 0 : inactiveplayer[x]); }
	const char *Name(int idx) { return name[idx]; }
	int Limit(int idx) { return limit[idx]; }
	int Active(int idx) { return active[idx]; }
	int Seek(int idx) { return seek[idx]; }
	int InactivePlayer(int idx) { return inactiveplayer[idx]; }

        int Count(void)
                { return name.Count(); }
        int Has(const char *n)
                { return name.Has(n); }
        int &operator[](int ix)
                { return limit[ix]; }

        int Read(GameFile &game);   
        int Write(GameFile &game);
        int ReadFile(const char * gameType);
        int WriteFile(const char * gameType); 
};

class RatingList {
protected:
	StringList name;
	IntList won, lost, tie, rating;
public:
	void Add(const char *n,int w,int l,int t,int r)
		{ name.Append(n); won.Append(w); lost.Append(l); tie.Append(t); rating.Append(r); }
	void DeleteIndex(int ix)
		{ name.DeleteIndex(ix); won.DeleteIndex(ix); lost.DeleteIndex(ix); tie.DeleteIndex(ix); rating.DeleteIndex(ix); }
	void Init()
		{ name.Init(); won.Init(); lost.Init(); tie.Init(); rating.Init(); }
	const char *Name(int i)
		{ return name[i]; }
	int Won(int i)
		{ return won[i]; }
	int Lost(int i)
		{ return lost[i]; }
	int Tie(int i)
		{ return tie[i]; }
	int Games(int i)
		{ return Won(i)+Lost(i)+Tie(i); }
	int Rating(int i)
		{ return rating[i]; }
	int HasName(const char *n)
		{ return name.Has(n); }
	int Count(void)
		{ return name.Count(); }
};

class Ratings {
protected:
	const char *gametype;
	void Read(StringList &players);
	void Write(void);
	RatingList before;
	RatingList after;
public:
	Ratings(const char *game)
		{ gametype = game; before.Init(); after.Init(); }
	int NewRating(const char *name, int games, int rating, int outcome,
                      int handicap = 0);
	void GameOver(const char *winner, StringList &players, IntList &resigned, int points, const char *board, int history = 1);
	void GetChange(StringList &names, IntList &old, IntList &now);
	void DisplayLine(FILE *fp, int i, int anyties, int rank, const char *name, int rating, int won, int lost, int tie, int active, int limit, int seek);
	void Display(FILE *fp);
};

class StandingList {
protected:
	StringList pool, name;
	IntList won, tie, lost;
public:
	void Add(const char *p,const char *n,int w,int l,int t)
		{ pool.Append(p); name.Append(n); won.Append(w); lost.Append(l); tie.Append(t); }
	void DeleteIndex(int ix)
		{ pool.DeleteIndex(ix); name.DeleteIndex(ix); won.DeleteIndex(ix); lost.DeleteIndex(ix); tie.DeleteIndex(ix); }
	void Init()
		{ pool.Init(); name.Init(); won.Init(); lost.Init(); tie.Init(); }
	const char *Pool(int i)
		{ return pool[i]; }
	const char *Name(int i)
		{ return name[i]; }
	int Won(int i)
		{ return won[i]; }
	int Lost(int i)
		{ return lost[i]; }
	int Tie(int i)
		{ return tie[i]; }
	void IncrWon(int ix)
		{ won[ix]++; }
	void IncrLost(int ix)
		{ lost[ix]++; }
	void IncrTie(int ix)
		{ tie[ix]++; }
	int HasName(const char *n)
		{ return name.Has(n); }
	int Count(void)
		{ return name.Count(); }
	void Swap(int i,int j)
		{ pool.Swap(i,j); name.Swap(i,j); won.Swap(i,j); lost.Swap(i,j); tie.Swap(i,j); }
};

class TournamentStandings {
protected:
	const char *gametype;
	char *pool;
	StandingList standings;
public:
	TournamentStandings(const char *game, char *p="*") : gametype(game), pool(p)
		{ standings.Init(); }
	void Read(char *pool="*");
	void GameOver(const char *winner, StringList &players);
	void Write(char *pool="*");
	void Sort(void);
	void Display(FILE *fp);
};

#endif

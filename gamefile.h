#ifndef GAMEFILE_H
#define GAMEFILE_H

#include <stdio.h>

class GameFile {
protected:
	FILE *file;
	FILE *lock;
	int mode;
	int eof;
	int one;
	char line[512];
	char *tok;
	int inentry;
	int modified;

	static char begin;
	static char end;
	static char escape;

	void GetLine(void);
	void CloseEntry(void);

public:
	GameFile();
	~GameFile();

	int Modified() { return modified; }

	int Read(const char *gametype, const char *game);
	void SkipLine(void)
		{ GetLine(); }
	int GetEntry(const char *entry);
	const char *GetToken(void);
	int IsEof(void)
		{ return eof; }

	int Write(const char *gametype, const char *game);
	GameFile &PutEntry(const char *entry, int oneliner);
	GameFile &PutToken(const char *token);
	GameFile &PutToken(const int token);

	FILE *Lock(const char *gametype, const char *game);
	void Unlock();

	int Close(void);
};


#endif

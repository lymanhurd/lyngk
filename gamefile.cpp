#pragma GCC diagnostic ignored "-Wparentheses"

#include <stdio.h>
#include "file.h"
#include <sys/stat.h>
#if defined(WIN32) && !defined(unix)
#define stat _stat
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "gamefile.h"


char GameFile::begin = '(';
char GameFile::end = ')';
char GameFile::escape = '\\';

GameFile::GameFile()
{
	modified = 0;
	file = NULL;
	mode = 0;
	eof = 0;
}

GameFile::~GameFile()
{
	Close();
}

int GameFile::Read(const char *gametype, const char *game)
{
	sprintf(line, "%s/games/%s/%s",getenv("PBMHOME"),gametype,game);
	Close();
	file = f_open(line,"r");
	if (file) {
		struct stat fs;
		stat(line,&fs);
		modified = fs.st_mtime;
		mode = 1; // read
		GetLine();
	}
	return file!=NULL;
}

void GameFile::GetLine(void)
{
	if (mode != 1)
		return;
	if (!fgets(line,sizeof(line),file))
		eof = 1;
	else
		line[strlen(line)-1] = '\0';
}

int GameFile::GetEntry(const char *entry)
{
	if (mode != 1)
		return 0;
	if (eof)
		return 0;
	while (line[0] != begin && !eof)
		GetLine();
	if (eof || strncmp(line+1,entry,strlen(entry)))
		return 0;
	one = (strlen(line+1) != strlen(entry));
	if (one) {
		line[0] = '[';
		line[strlen(line)-1] = '\0'; // remove the last ')'
		tok = strtok(line+1+strlen(entry)," ");
	}
	return 1;
}

const char *GameFile::GetToken(void)
{
	if (mode != 1)
		return NULL;
	if (one) {
		char *t = tok;
		if (tok)
			tok = strtok(NULL," ");
		return t;
	} else {
		GetLine();
		if (eof || (line[0]==end && line[1]=='\0'))
			return NULL;
		if (line[0] == escape && (line[1] == begin || line[1] == end || line[1] == escape))
			return line+1;
	}
	return line;
}


int GameFile::Write(const char *gametype, const char *game)
{
	sprintf(line, "%s/games/%s/%s", getenv("PBMHOME"),gametype,game);
	sprintf(line+strlen(line)+1, "%s/games/%s/.%s", getenv("PBMHOME"),gametype,game);
	unlink(line+strlen(line)+1);
	rename(line,line+strlen(line)+1);
	Close();
	file = f_open(line,"w");
	if (file) {
		mode = 2; // write
		inentry = 0;
	}
	return file!=NULL;
}

GameFile &GameFile::PutEntry(const char *entry, int oneliner)
{
	if (mode != 2)
		return *this;
	CloseEntry();
	one = oneliner;
	fputc(begin, file);
	fputs(entry, file);
	if (!one)
		fputc('\n',file);
	inentry = 1;
	return *this;
}

GameFile &GameFile::PutToken(const char *token)
{
	if (mode != 2)
		return *this;
	if (one)
		fputc(' ', file);
	if (!one && token[0] == begin || token[0] == end || token[0] == escape)
		fputc(escape, file);
	fputs(token, file);
	if (!one)
		fputc('\n', file);
	return *this;
}

GameFile &GameFile::PutToken(const int token)
{
	if (mode != 2)
		return *this;
	if (one)
		fputc(' ', file);
	fprintf(file,"%d",token);
	if (!one)
		fputc('\n', file);
	return *this;
}

void GameFile::CloseEntry(void)
{
	if (mode != 2)
		return;
	if (inentry) {
		fputc(end, file);
		fputc('\n', file);
		inentry = 0;
	}
}


int GameFile::Close(void)
{
	CloseEntry();
	if (file && mode) {
		f_close(file);
		file = NULL;
		mode = 0;
	}
	return 1;
}

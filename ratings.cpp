#include <stdio.h>
#include "file.h"
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#if defined(WIN32) && !defined(unix)
#define popen(str,mode) (printf("%s\n",str),*(mode)=='w'?stdout:stdin)
#define pclose(fp)
#define lockf(fp,mode,x)
#else
#include <unistd.h>
#include <dirent.h>

# ifdef F_LOCK
extern "C" { int lockf(int, int, long); };
# else
// workaround for lockf vs flock in BSD
#include <fcntl.h>
#define F_LOCK  LOCK_EX
#define F_ULOCK LOCK_UN
#define lockf(a,b,c)  flock(a,b)
# endif

#endif
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "sendmail.h"
#include "strutl.h"
#include "chkuser.h"
#include "ratings.h"
#include "gamefile.h"

#include <math.h>
#include <string.h>
#include <time.h>
#if defined(WIN32) && !defined(unix)
#define TIMEFORMAT "%b %d %H:%M"
#else
#define TIMEFORMAT "%b %e %R"
#include <unistd.h>
#endif

#include "ratings.h"
#include "sendmail.h"

static void UpdateHistory(const char *gametype, const char *winner,
	StringList &players, int points, const char *board)
{
	// Also write to the history list. The format is new because
	// of multi player games:
	// "time board winner points players"
	char fn[256];
	sprintf(fn,"%s/history/%s",getenv("PBMHOME"),gametype);
	FILE *fp = f_open(fn,"a");
	time_t clock = time(NULL);
	struct tm *ts = localtime(&clock);
	strftime(fn,sizeof(fn),TIMEFORMAT,ts);
	fprintf(fp,"%s %-5s %-10s %2d",fn,board,winner?winner:"TIE",points);
	int i;
	for (i = 0; i < players.Count(); i++)
		fprintf(fp," %-10s",players[i]);
	fputc('\n',fp);
	f_close(fp);
}


void Ratings::Read(StringList &players)
{
	StringList p;
	int i;

	int add_all = (players.Count()==0);

	for (i = 0; i < players.Count(); i++)
		p.Add(players[i]);

	char fn[100];
	sprintf(fn,"%s/ratings/%s", getenv("PBMHOME"), gametype);

	before.Init();

	FILE *fp;
	if ((fp = f_open(fn,"r")) != NULL) {
		before.Init();
		char line[256];
        char grepcmd[1024];
		while (fgets(line,sizeof(line),fp)) {
			char tname[256];
			int twins,tlosses,tties,trating;
			sscanf(line,"%s %d %d %d %d",tname,&twins,&tlosses,&tties,&trating);
            sprintf(grepcmd,"grep -q ^%s: %s/etc/users",tname,getenv("PBMHOME"));
            if (system(grepcmd) == 0) {
                int ix = p.Has(tname);
                if (ix >= 0)
                    p.DeleteIndex(ix);
                if (ix >= 0 || add_all) {
                    before.Add(tname,twins,tlosses,tties,trating);
                }
            }
		}
		f_close(fp);
	}
	for (i = 0; i < p.Count(); i++)
		before.Add(p[i],0,0,0,1600);
}

void Ratings::Write(void)
{
	RatingList aftercopy;
	
	int i;
	aftercopy.Init();
	for (i = 0; i < after.Count(); i++)
		aftercopy.Add(after.Name(i),after.Won(i),after.Lost(i),after.Tie(i),after.Rating(i));

	StringList p;
	for (i = 0; i < after.Count(); i++)
		p.Add(after.Name(i));

	char fn1[100], fn2[100];
	sprintf(fn1,"%s/ratings/%s", getenv("PBMHOME"),gametype);
	sprintf(fn2,"%s.new", fn1);

	FILE *fp1, *fp2;
	fp1 = f_open(fn1,"r");
	fp2 = f_open(fn2,"w");
	if (fp1 != NULL && fp2 != NULL) {
		char line[256];
		while (fgets(line,sizeof(line),fp1)) {
			char tname[256];
			int twins,tlosses,tties,trating;
			sscanf(line,"%s %d %d %d %d",tname,&twins,&tlosses,&tties,&trating);
			int ix = p.Has(tname);
			if (ix >= 0) {
				p.DeleteIndex(ix);
				continue;
			}
			do {
				ix = -1;
				int r = trating;
				int i;
				for (i = 0; i < after.Count(); i++)
					if (after.Rating(i) > r) {
						r = after.Rating(i); /* Find highest rating */
						ix = i;
					}
				if (ix >= 0) {
					fprintf(fp2,"%-32s %4d %4d %4d %6d\n",
						after.Name(ix),after.Won(ix),after.Lost(ix),after.Tie(ix),after.Rating(ix));
					after.DeleteIndex(ix);
				}
			} while (ix >= 0);
			fputs(line,fp2);
		}
	}

	while (after.Count()) {
		int ix = -1;
		int r = 0;
		int i;
		for (i = 0; i < after.Count(); i++)
			if (after.Rating(i) > r) {
				r = after.Rating(i); /* Find highest rating */
				ix = i;
			}
		fprintf(fp2,"%-32s %4d %4d %4d %6d\n",
			after.Name(ix),after.Won(ix),after.Lost(ix),after.Tie(ix),after.Rating(ix));
		after.DeleteIndex(ix);
	}

	if (fp1) f_close(fp1);
	if (fp2) f_close(fp2);

	unlink(fn1);
	rename(fn2,fn1);

	after.Init();
	for (i = 0; i < aftercopy.Count(); i++)
		after.Add(aftercopy.Name(i),aftercopy.Won(i),aftercopy.Lost(i),aftercopy.Tie(i),aftercopy.Rating(i));
}

/* Calculate the new rating for Name (against another player). The Outcome must
 * be either -1 for loss, 0 for draw or 1 for win (as seen from Name's
 * point of view) */
int Ratings::NewRating(const char *name, int games2, int rating2, int outcome,
                       int handicap)
{
	int ix1 = before.HasName(name);
	int rating1 = before.Rating(ix1);
	int games1 = before.Games(ix1);
        int effrat2 = rating2 - 100 * handicap;

        /* To prevent the ratings from taking too wild jumps, this line has
         * been introduced. */
        outcome = outcome < 0 ? -1 : outcome > 0 ? 1 : 0;

	int newrating;

	if (games1 < 20) {		/* Provisional against... */
		int value;
		if (games2 < 20)	/* provisional */
			value = (rating1+effrat2)/2 + 200*outcome;
		else			/* established */
			value = effrat2 + 400*outcome;
		newrating = (rating1*games1+value)/(games1+1) + (1720-rating1)/5;
	} else {			/* Established against... */
		double K = 32.;		/* established */
		double w = .5 * (outcome+1);
		if (games2 < 20)	/* provisional */
			K *= games2/20.;
		newrating = rating1 + (int) (K * (w-1/(1+pow(10.,((double) (effrat2-rating1))/400))));
	}

	return newrating;
}

void Ratings::GameOver(const char *winner, StringList &allplayers,
                       IntList &resigned, int points, const char *board,
                       int history)
{
	StringList players; /* This list will hold each player only once */
	int p;
	for (p = 0; p < allplayers.Count(); p++)
		if (players.Has(allplayers[p]) < 0)
			players.Add(allplayers[p]);
	/* If less than two players actually played don't update */
	if (players.Count() < 2)
		return;
	Read(players);

	after.Init();
	
	int i;
	int games=0, rating=0, num=0;
	for (i = 0; i < players.Count(); i++) {
		int ix = before.HasName(players[i]);
		games += before.Games(ix);
		rating += before.Rating(ix);
		num++;
	}
	if (winner == NULL) {
		// It's a tie
		for (i = 0; i < players.Count(); i++) {
			int ix = before.HasName(players[i]); /* before index of player */
			/* Does player I have any non-resigned games? */
			int stillin=0;
			int j;
			for (j=0 ; !stillin && j<allplayers.Count() ; j++)
				if (!strcmp(players[i],allplayers[j]) && !resigned[j])
					stillin=1;
			if (!stillin) {
				after.Add(players[i], before.Won(ix), before.Lost(ix)+1, before.Tie(ix),
					NewRating(players[i],
						(games-before.Games(ix))/(num-1),
						(rating-before.Rating(ix))/(num-1), -points));
			}
			else {
				after.Add(players[i], before.Won(ix), before.Lost(ix), before.Tie(ix)+1,
					NewRating(players[i],
						(games-before.Games(ix))/(num-1),
						(rating-before.Rating(ix))/(num-1), 0));
			}
				
		}
	} else {
		int wix = before.HasName(winner); /* before index of winner */
		for (i = 0; i < players.Count(); i++) {
			int ix = before.HasName(players[i]); /* before index of player */
			if (ix != wix) {
				/* Instead of "players[i]" we can use "before.Name(ix)" in the following */
				after.Add(players[i], before.Won(ix), before.Lost(ix)+1, before.Tie(ix),
					NewRating(players[i],
						(games-before.Games(ix))/(num-1),
						(rating-before.Rating(ix))/(num-1), -points));
			}
		}
		after.Add(winner, before.Won(wix)+1, before.Lost(wix), before.Tie(wix),
			NewRating(winner,
				(games-before.Games(wix))/(num-1),
				(rating-before.Rating(wix))/(num-1), points));
	}

	Write();

	if (history)
		UpdateHistory(gametype, winner, players, points, board);
}

void Ratings::DisplayLine(FILE *fp, int i, int anyties, int rank, const char *name, int rating, int won, int lost, int tie, int active, int limit, int seek)
{
  fprintf(fp,"%3d  %-16s %6d %4d %4d", rank, name, rating, won, lost);
  if (anyties)
    fprintf(fp," %4d", tie);
  if (active>0)
    fprintf(fp,"   %5d",active);
  else
    fprintf(fp,"        ");
  if (limit==0)
    fprintf(fp," /reject");
  else if (limit!=ACCEPTALL)
    fprintf(fp," / %-4d ",limit);
  else
    fprintf(fp,"        ");
  if (seek > 0)
    fprintf(fp," %7d",seek);
  fprintf(fp,"\n");
}

void Ratings::Display(FILE *fp)
{
	StringList empty;
	Limits limits;
	limits.ReadFile(gametype);
	const char *name;

	Read(empty);

	fprintf(fp,"Subject: %s Standings\n\n",gametype);
	MIME_Header(fp);
	int established = 0;
	int rank = 1;
	int anyties=0;
	int i;
	for (i = 0; !anyties && i < before.Count(); i++) {
		if (before.Tie(i))
			anyties++;
	}
	for (i = 0; i < before.Count(); i++) {
		if (before.Games(i) >= 20 && !limits.InactivePlayer(before.Name(i))) {
			if (!established) {
				fprintf(fp,"\n                Established\n");
				fprintf(fp,"                ===========\n");
                                fprintf(fp,"Rank Name             Rating  Won Lost%s%s\n",(anyties?" Ties":""),"   Active/Limit  Seeking");
                                fprintf(fp,"---- ---------------- ------ ---- ----%s%s\n",(anyties?" ----":""),"   ------------  -------");
				established = 1;
			}
			name = before.Name(i);
			DisplayLine(fp, i, anyties, rank, name, before.Rating(i), before.Won(i), before.Lost(i), before.Tie(i), limits.Active(name), limits.Limit(name), limits.Seek(name));
			rank++;
		}
	}
	int provisionals = 0;
	rank = 1;
	for (i = 0; i < before.Count(); i++) {
		if (before.Games(i) < 20 && !limits.InactivePlayer(before.Name(i))) {
			if (!provisionals) {
				fprintf(fp,"\n\n                Provisionals\n");
				fprintf(fp,"                ============\n");
                                fprintf(fp,"Rank Name             Rating  Won Lost%s%s\n",(anyties?" Ties":""),"   Active/Limit  Seeking");
                                fprintf(fp,"---- ---------------- ------ ---- ----%s%s\n",(anyties?" ----":""),"   ------------  -------");
				provisionals = 1;
			}
			name = before.Name(i);
			DisplayLine(fp, i, anyties, rank, name, before.Rating(i), before.Won(i), before.Lost(i), before.Tie(i), limits.Active(name), limits.Limit(name), limits.Seek(name));
			rank++;
		}
	}
	int inactiveE = 0;
	rank = 1;
	for (i = 0; i < before.Count(); i++) {
		if (before.Games(i) >= 20 && limits.InactivePlayer(before.Name(i))) {
			if (!inactiveE) {
				fprintf(fp,"\n\n                Inactive (established)\n");
				fprintf(fp,    "                ======================\n");
                                fprintf(fp,"Rank Name             Rating  Won Lost%s%s\n",(anyties?" Ties":""),"   Active/Limit  Seeking");
                                fprintf(fp,"---- ---------------- ------ ---- ----%s%s\n",(anyties?" ----":""),"   ------------  -------");
				inactiveE = 1;
			}
			name = before.Name(i);
			DisplayLine(fp, i, anyties, rank, name, before.Rating(i), before.Won(i), before.Lost(i), before.Tie(i), limits.Active(name), limits.Limit(name), limits.Seek(name));
			rank++;
		}
	}
	int inactiveP = 0;
	rank = 1;
	for (i = 0; i < before.Count(); i++) {
		if (before.Games(i) < 20 && limits.InactivePlayer(before.Name(i))) {
			if (!inactiveP) {
				fprintf(fp,"\n\n                Inactive (provisional)\n");
				fprintf(fp,    "                ======================\n");
                                fprintf(fp,"Rank Name             Rating  Won Lost%s%s\n",(anyties?" Ties":""),"   Active/Limit  Seeking");
                                fprintf(fp,"---- ---------------- ------ ---- ----%s%s\n",(anyties?" ----":""),"   ------------  -------");
				inactiveP = 1;
			}
			name = before.Name(i);
			DisplayLine(fp, i, anyties, rank, name, before.Rating(i), before.Won(i), before.Lost(i), before.Tie(i), limits.Active(name), limits.Limit(name), limits.Seek(name));
			rank++;
		}
	}
	if (before.Count()) {
		if (established + provisionals + inactiveE + inactiveP > 1) {
			fprintf(fp,"\n\n         Established & Provisionals\n");
			fprintf(fp,    "         ==========================\n");
                                fprintf(fp,"Rank  Name             Rating  Won Lost%s%s\n",(anyties?" Ties":""),"   Active/Limit  Seeking");
                                fprintf(fp,"----- ---------------- ------ ---- ----%s%s\n",(anyties?" ----":""),"   ------------  -------");
			int i;
			for (i = 0; i < before.Count(); i++) {
				fprintf(fp,"%3d %c %-16s %6d %4d %4d",
					i+1,(limits.InactivePlayer(before.Name(i))?'I':(before.Games(i)<20?'P':'E')),before.Name(i),before.Rating(i),before.Won(i),before.Lost(i));
				if (anyties)
					fprintf(fp," %4d",before.Tie(i));
			int n;
			n = limits.Active(before.Name(i));
			if (n>0)
				fprintf(fp,"   %5d",n);
			else
				fprintf(fp,"        ");
			n = limits.Limit(before.Name(i));
			if (n==0)
				fprintf(fp," /reject");
			else if (n!=ACCEPTALL)
				fprintf(fp," / %-4d",n);
			else
				fprintf(fp,"        ");
			n = limits.Seek(before.Name(i));
			if (n > 0)
				fprintf(fp," %7d",n);
			fprintf(fp,"\n");
			}
		}
	} else
		fprintf(fp,"\n\nNo games have finished yet\n");
}

void Ratings::GetChange(StringList &names, IntList &old, IntList &now)
{
	int i;
        int o, n;
	names.Init();
	old.Init();
	now.Init();
	for (i = 0; i < before.Count(); i++) {
	    names.Add(before.Name(i));
            o = before.Rating(i);
            n = after.Rating(after.HasName(before.Name(i)));
            if (before.Games(i) < 20) {
	        old.Add(-1);
	        now.Add(n);
            } else if (before.Games(i) == 20) {
	        old.Add(n);
	        now.Add(n);
            } else {
	        old.Add(o);
	        now.Add(n);
	    }
        }
}



void TournamentStandings::Read(char *board)
{
	char fn[100];
	sprintf(fn,"%s/standings/%s", getenv("PBMHOME"), gametype);
	char pool[256];
	char *ch=pool;

	FILE *fp;

	strcpy(pool,board);
	while (!isdigit(*ch))
		ch++;
	*ch = '\0';
	if ((fp = f_open(fn,"r")) != NULL) {
		standings.Init();
		char line[256];
		while (fgets(line,sizeof(line),fp)) {
			char tpool[8],tname[256];
			int twins,tlosses,tties=0;
			sscanf(line,"%[^:]:%[^:]:%d:%d:%d",tpool,tname,&twins,&tlosses,&tties);
			if (!strcmp(pool,"*") || !strcmp(pool,tpool))
				standings.Add(tpool,tname,twins,tlosses,tties);
		}
		f_close(fp);
	}
}

void TournamentStandings::Write(char *board)
{
	char pool[256];
	char *ch=pool;
	strcpy(pool,board);
	while (!isdigit(*ch))
		ch++;
	*ch = '\0';

	int putpool=0;
	char fn1[100], fn2[100];
	sprintf(fn1,"%s/standings/%s", getenv("PBMHOME"),gametype);
	sprintf(fn2,"%s.new", fn1);

	FILE *fp1, *fp2;
	fp1 = f_open(fn1,"r");
	fp2 = f_open(fn2,"w");
	if (fp1 != NULL && fp2 != NULL) {
		char line[256];
		while (fgets(line,sizeof(line),fp1)) {
			if (!strncmp(pool,line,strlen(pool)) && line[strlen(pool)] == ':') {
				if (!putpool) {
					putpool=1;
					int i;
					for (i=0 ; i<standings.Count() ; i++)
						fprintf(fp2,"%s:%s:%d:%d:%d\n",
							standings.Pool(i),standings.Name(i),standings.Won(i),standings.Lost(i),standings.Tie(i));
				}
			}
			else 
				fputs(line,fp2);
		}
	}

	if (fp1) f_close(fp1);
	if (fp2) f_close(fp2);

	unlink(fn1);
	rename(fn2,fn1);
}

void TournamentStandings::Sort(void)
{
	int i,j;
	for (i=0; i<standings.Count()-1; i++)
		for (j=i+1; j<standings.Count() && standings.Pool(i)[0] == standings.Pool(j)[0]; j++){
			int score1i = 2*standings.Won(i)+standings.Tie(i);
			int score2i = 2*standings.Lost(i)+standings.Tie(i);
			int score1j = 2*standings.Won(j)+standings.Tie(j);
			int score2j = 2*standings.Lost(j)+standings.Tie(j);
			if (score1i < score1j || (score1i == score1j && score2i > score2j))
				standings.Swap(i,j);
		}
}

void TournamentStandings::GameOver(const char *winner, StringList &players)
{
	Read(pool);

	if (winner == NULL) {
		// It's a tie
		int i;
		for (i = 0; i < players.Count(); i++) {
			int ix = standings.HasName(players[i]);
			standings.IncrTie(ix);
		}
	} else {
		int i;
		int wix = standings.HasName(winner);
		standings.IncrWon(wix);
		for (i = 0; i < players.Count(); i++)
			if (strcmp(players[i],winner))
				standings.IncrLost(standings.HasName(players[i]));
	}

	Sort();
	Write(pool);
}

void TournamentStandings::Display(FILE *fp)
{
	char tpool[8];
	int anyties=0;

	strcpy(tpool,"*");
	Read();

	int i;
	for (i=0 ; !anyties && i<standings.Count() ; i++)
		if (standings.Tie(i))
			anyties++;
	if (standings.Count()) {
		fprintf(fp,"\n\nPool  Name              W   L %s\n",(anyties?"  T ":""));
		fprintf(fp,"----  ---------------- --- ---%s\n",(anyties?" ---":""));
		for (i = 0; i < standings.Count(); i++) {
			if (strcmp(tpool,"") && strcmp(tpool,standings.Pool(i)))
				fprintf(fp,"\n");
			strcpy(tpool,standings.Pool(i));
			fprintf(fp,"  %s   %-16s %3d %3d",
						standings.Pool(i),standings.Name(i),standings.Won(i),standings.Lost(i));
			if (anyties)
				fprintf(fp," %3d", standings.Tie(i));
			fprintf(fp,"\n");
		}
	}
}


int Limits::Read(GameFile &game)
{
    char grepcmd[1024];
	if (!game.GetEntry(Name()))
		return 0;
	Init();
	const char * buf;
	char tname[256];
	int  tactive,tlimit,tseek,tinactiveplayer;
	int  nm;
	while ((buf = game.GetToken())) {
		nm = sscanf(buf,"%s %d %d %d %d",tname,&tlimit,&tactive,&tseek,&tinactiveplayer);
		if (nm<2) tlimit = REJECTALL;	// compatible with old .reject file
		if (nm<3) tactive = 0;
		if (nm<4) tseek = 0;
		if (nm<5) tinactiveplayer = 0;

		sprintf(grepcmd,"grep -q ^%s: %s/etc/users",tname,getenv("PBMHOME"));
		if (system(grepcmd) == 0) {
		  name.Add(tname);
		  limit.Add(tlimit);
		  active.Add(tactive);
		  seek.Add(tseek);
		  inactiveplayer.Add(tinactiveplayer);
		}
	}
	game.Close();
	return 1;
}

int Limits::Write(GameFile &game)
{
	if (!Count())
		return 1;
	game.PutEntry(Name(),0);
	int i;
	char buf[256];
	for (i=0; i < Count(); i++) {
		if (active[i]==0 && limit[i]==ACCEPTALL && seek[i]==0 && inactiveplayer[i]==0) 
			continue;
		if (active[i]==0 && CheckUserid(name[i],"")==BAD_USER)
			continue;
		sprintf(buf,"%-32s %10d %10d %10d %10d",name[i],limit[i],active[i],seek[i],inactiveplayer[i]);
		game.PutToken(buf);
	}
	game.Close();
	return 1;
}


int Limits::ReadFile(const char * gameType)
{
	GameFile game;
	game.Read(gameType,FileName());
	return Read(game);      // Will automatically do game.Close()
}

int Limits::WriteFile(const char * gameType)
{
	GameFile game;
	game.Write(gameType,FileName());
	return Write(game);     // Will automatically do game.Close()
}

void Limits::Update(const char *n, int a, int l, int s, int ip)
{
	int idx = name.Has(n);
	if (a==0 && l==ACCEPTALL && s==0 && ip==0) {
		if (idx >= 0)
			DeleteIndex(idx);
		return;
	}
	if (idx >= 0) {
		active[idx] = a;
		limit[idx] = l;
		seek[idx] = s;
		inactiveplayer[idx] = ip;
	} else {
		name.Add(n);
		active.Add(a);
		limit.Add(l);
		seek.Add(s);
		inactiveplayer.Add(ip);
	}
}

void Limits::UpdateLimit(const char *n, int l)
{
	int idx = name.Has(n);
	if (idx >= 0) {
		Update(n,active[idx],l,seek[idx],inactiveplayer[idx]);
	} else {
		Update(n,0,l,0,0);
	}
}

void Limits::UpdateActive(const char *n, int a)
{
	int idx = name.Has(n);
	if (idx >= 0) {
		Update(n,a,limit[idx],seek[idx],inactiveplayer[idx]);
	} else {
		Update(n,a,ACCEPTALL,0,0);
	}
}

void Limits::UpdateSeek(const char *n, int s)
{
	int idx = name.Has(n);
	if (idx >= 0) {
		Update(n,active[idx],limit[idx],s,inactiveplayer[idx]);
	} else {
		Update(n,0,ACCEPTALL,s,0);
	}
}

void Limits::UpdateInactivePlayer(const char *n, int ip)
{
	int idx = name.Has(n);
	if (idx >= 0) {
		Update(n,active[idx],limit[idx],seek[idx],ip);
	} else {
		Update(n,0,ACCEPTALL,0,ip);
	}
}

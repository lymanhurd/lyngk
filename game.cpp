#pragma GCC diagnostic ignored "-Wswitch"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "file.h"
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ctype.h>
#include <time.h>
#if defined(WIN32) && !defined(unix)
#define popen(str,mode) (printf("%s\n",str),*(mode)=='w'?stdout:stdin)
#define pclose(fp)
#define lockf(fp,mode,x)
#include <process.h>
#define strcasecmp(x,y) stricmp(x,y)
#define strncasecmp(x,y,z) strnicmp(x,y,z)
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
#ifdef __CYGWIN32__
#define flock(a,b)
#endif
# endif

#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "game.h"
#include "sendmail.h"
#include "strutl.h"
#include "chkuser.h"
#include "ratings.h"
#include "gamefile.h"

static inline int RandomSeed()
{

	//int fd = open("/dev/random",O_RDONLY,0);
    int rnd=time(NULL);

    //read(fd,&rnd,sizeof(int));
	rnd = rnd ^ (getpid() + (getpid() << 15));

	//close(fd);
	return rnd;
}

int Game::LockFile(void)
{
    char fn[256];
    if (getenv("PBMHOME")==NULL) {
      printf("PBMHOME must be set!");
      assert(0);
    }
    sprintf(fn,"%s/games/%s/.lockfile",getenv("PBMHOME"),GameType());
    lfp = f_open(fn,"w");
    if (!lfp) return Error("Server Error: cannot open %s",fn);
    lockf(fileno(lfp),F_LOCK,1);
    return 1;
}

void Game::UnlockFile(void)
{
    lockf(fileno(lfp),F_ULOCK,1);
    f_close(lfp);
}

void Comment::Print(FILE *fp)
{
    if (Count()) {
        fprintf(fp,"\n\n************************************************************\n\n");
        int i;
        for (i = 0; i < Count(); i++)
            fprintf(fp,"%s\n",(*this)[i]);
    }
}

void Comment::GetNew(const char *userid, int addon)
{
    char _line[1024];
    char *line = _line;
    int blank = 0;
    int skip = 1;
    int first = 1;
	int start=0;
	int mark;

	if (getenv("MIME_BOUNDARY") == NULL) {
		Init();
		return;
	}

	if (!addon) Init();
	mark = Count();

	if (addon) {
		Add("");
		Add("------------------------------------------------------------");
		Add("");
	}

    sprintf(line, "%s writes:", userid);
    Add(line);

	char *type   = getenv("CONTENTTYPE");
	char *how    = getenv("CONTENTDISPOSITION");
	char *encode = getenv("CONTENTTRANSFERENCODING");
	char *boundry = getenv("MIME_BOUNDRY");

	if (type) {
		if (strcasecmp(type,"text/plain")  &&
            strncasecmp(type,"text/plain;",11)) {
            sprintf(line,"--%s",getenv("MIME_BOUNDARY"));
            Add(line);
			sprintf(line,"Content-Type: %s",type);
			Add(line);
			if (how) {
				sprintf(line,"Content-Disposition: %s",how);
				Add(line);
			}
			if (encode) {
				sprintf(line,"Content-Transfer-Encoding: %s",encode);
				Add(line);
			}
			Add("");
		}
	}
	start = Count();

    FILE *fp = f_open(getenv("INPUTFILE"),"r");
	if (fp) {
        while (fgets(_line,sizeof(_line),fp)) {
    		char *idx = line+strlen(line)-1;
            if (*idx == '\n')
                *idx-- = '\0';
            while (line <= idx && strchr(" \t", *idx))
                idx--;
            if (line <= idx) { /* Something on this line */
                if (first && strncmp(line, " FROM:", 6) == 0) {
                    line = _line+1; /* There is a funny header, cut it */
                } else if (!first)
                    while (blank--) /* Did we save up blank lines? */
                        Add("");
                if (!skip) {
                    Add(line); /* Put the line into the comment */
                    blank = first = 0;
                }
            } else 
                blank++, skip = 0; /* Count the blank lines */
        }
        f_close(fp);
	}

    // Check if the only line is "<user> writes:"
    if (Count() == start) {
        if (mark==0)
			Init();
		else
			while (Count() > mark)
				DeleteIndex(mark);
	}
}

void Subscribers::Print(FILE *fp)
{
    if (Count()) {
        fprintf(fp,"\n\nSubscribers:");
        int i;
        for (i = 0; i < Count(); i++)
            fprintf(fp," %s",(*this)[i]);
        fprintf(fp,"\n");
    }
}

void BoardNotifiers::Print(FILE *fp,int spacer)
{
    if (Count()) {
	if (spacer) fprintf(fp,"\n\n");
        fprintf(fp,"Notifiers:");
        int i;
        for (i = 0; i < Count(); i++)
            fprintf(fp," %s",(*this)[i]);
        fprintf(fp,"\n");
    }
}

void Status::SetWinner(const char *who, int points)
{
    if (who == NULL) {
        Init().Add("tie");
    } else {
        Init().Add("winner").Add(who);
        char pts[10];
        sprintf(pts,"%d",points);
        Add(pts);
    }
}

int Status::GetWinner(const char *&who, int &points)
{
    if (Strcmp((*this)[0],"winner")) return 0;
    who = (*this)[1];
    sscanf((*this)[2],"%d",&points);
    return 1;
}

int Status::GetTie(void)
{
    return !Strcmp((*this)[0],"tie");
}

void Status::SetHasLeft(const char *who)
{
    Init().Add("hasleft").Add(who);
}

int Status::GetHasLeft(const char *&who)
{
    if (Strcmp((*this)[0],"hasleft")) return 0;
    who = (*this)[1];
    return 1;
}

void Status::SetProposed(const char *who)
{
	if (Strcmp((*this)[0],"proposed"))
		Init().Add("proposed");
	if (Has(who,1) < 0)
		Add(who);
}

int Status::GetProposed(StringList &who)
{
	if (Strcmp((*this)[0],"proposed")) return 0;
	who.Init(*this).DeleteIndex(0);
	return 1;
}

void Status::SetUndo(const char *who)
{
	if (Strcmp((*this)[0],"undo"))
		Init().Add("undo");
	if (Has(who,1) < 0)
		Add(who);
}

int Status::GetUndo(StringList &who)
{
	if (Strcmp((*this)[0],"undo")) return 0;
	who.Init(*this).DeleteIndex(0);
	return 1;
}

void Status::SetSwapped(const char *who)
{
    Init().Add("swapped").Add(who);
}

int Status::GetSwapped(const char *&who)
{
    if (Strcmp((*this)[0],"swapped")) return 0;
    who = (*this)[1];
    return 1;
}

void Status::SetNormal(void)
{
    Init().Add("normal");
}

int Status::GetNormal(void)
{
    return !Strcmp((*this)[0],"normal");
}

int Status::IsActive(void)
{
    if (!Strcmp((*this)[0],"winner")) return 0;
    if (!Strcmp((*this)[0],"tie")) return 0;
    return 1;
}

void Game::MassageArgs(int &argc, char **argv)
{
    char *s;

    /* alter argv[0] to contain just its' basename... */
    if ((s=strrchr(argv[0],'/')) != NULL)
        strcpy(argv[0],++s);

    /* save --xxx=yyy to options[] */
    /* save -xxx=yyy to parameters[] */
    opts.Init();
    parms.Init();

    int i;
    for (i=1 ; i<argc ; i++) {
		if (isSkipMove(argv[i]))
			continue;
        if (argv[i][0] == '-') {
            if (argv[i][1] == '-')
                opts.Add(argv[i]);
            else
                parms.Add(argv[i]);
            int j;
            for (j=i+1 ; j<argc ; j++)
                argv[j-1] = argv[j];
            argc--;
            i--;
        } else if (!Strcmp(argv[i], "**")) {
			/* Throw away "**" */
            int j;
            for (j=i+1 ; j<argc ; j++)
                argv[j-1] = argv[j];
            argc--;
            i--;
		}
    }
}

int Game::GetOption(char *option, int dflt)
{
    static FILE *fp=NULL;
    static char line[1024];      /* for each user on a separate line, in*/
    char fn[256];                /* the form userid:size with no spaces.*/
    char cmp[256];               /* size is either "small", "medium",   */
    char *s;                     /* or "big".                           */

    if (!player)
        return dflt;

    sprintf(fn,"%s/etc/%s.options",getenv("PBMHOME"),GameType());
    if (!fp) {
        fp = f_open(fn,"r");
	if (!fp) return dflt;
    }
    else
        rewind(fp);

    sprintf(cmp,"%s:%s:",player,option);
    while (fgets(line,sizeof(line),fp)) {
        s = strchr(line,'\n');
        if (s)
            *s = '\0';
        if (!strncmp(line,cmp,strlen(cmp))) {
            s = strrchr(line,':');
            s++;
            return atoi(s);
        }
    }
    return dflt;
}

char *Game::GetOption(char *option, char *dflt)
{
    static FILE *fp=NULL;
    static char line[1024];      /* for each user on a separate line, in*/
    char fn[256];                /* the form userid:size with no spaces.*/
    char cmp[256];               /* size is either "small", "medium",   */
    char *s;                     /* or "big".                           */

    if (!player)
        return dflt;

    sprintf(fn,"%s/etc/%s.options",getenv("PBMHOME"),GameType());
    if (!fp) {
        fp = f_open(fn,"r");
	if (!fp) return dflt;
    }
    else
        rewind(fp);

    sprintf(cmp,"%s:%s:",player,option);
    while (fgets(line,sizeof(line),fp)) {
        s = strchr(line,'\n');
        if (s)
            *s = '\0';
        if (!strncmp(line,cmp,strlen(cmp))) {
            s = line+strlen(cmp);
            return s;
        }
    }
    return dflt;
}

void Game::SetOption(char *option, int value)
{
    static char line[1024];   /*This routine creates a temp file, the    */
    char fn[256], tmpn[256];  /*first line of which tells the user's new */
    char cmp[256];            /*size, followed by every line in the size */
    char *s;                  /*file which DOESN'T refer to this user,   */
    FILE *rp,*wp;             /*then temp replaces the size file.        */

    sprintf(fn,"%s/etc/%s.options",getenv("PBMHOME"),GameType());
    sprintf(tmpn,"%s/etc/tmp%s",getenv("PBMHOME"),GameType());
    rp = f_open(fn,"r");
    wp = f_open(tmpn,"w");

    fprintf(wp,"%s:%s:%d\n",player,option,value);

    if (rp) {
        sprintf(cmp,"%s:%s:",player,option);
        while (fgets(line,sizeof(line),rp)) {
            s = strchr(line,'\n');
            if (s)
                *s = '\0';
            if (strncmp(line,cmp,strlen(cmp))) {    /*NOTE: If the comparison*/
                fprintf(wp,"%s\n",line);    /*FAILS, copy this line  */
            }                    /*into twixtmp. Else,    */
        }                        /*skip this line.        */
        f_close(rp);
        remove(fn);
    }

    f_close(wp);
    rename(tmpn,fn);
}

void Game::SetOption(char *option, char *value)
{
    static char line[1024];   /*This routine creates a temp file, the    */
    char fn[256], tmpn[256];  /*first line of which tells the user's new */
    char cmp[256];            /*size, followed by every line in the size */
    char *s;                  /*file which DOESN'T refer to this user,   */
    FILE *rp,*wp;             /*then temp replaces the size file.        */

    sprintf(fn,"%s/etc/%s.options",getenv("PBMHOME"),GameType());
    sprintf(tmpn,"%s/etc/tmp",getenv("PBMHOME"));
    rp = f_open(fn,"r");
    wp = f_open(tmpn,"w");

    fprintf(wp,"%s:%s:%s\n",player,option,value);

    if (rp) {
        sprintf(cmp,"%s:%s:",player,option);
        while (fgets(line,sizeof(line),rp)) {
            s = strchr(line,'\n');
            if (s)
                *s = '\0';
            if (strncmp(line,cmp,strlen(cmp))) {    /*NOTE: If the comparison*/
                fprintf(wp,"%s\n",line);    /*FAILS, copy this line  */
            }                    /*into twixtmp. Else,    */
        }                        /*skip this line.        */
        f_close(rp);
        remove(fn);
    }

    f_close(wp);
    rename(tmpn,fn);
}

void Game::PrintGameListLine(FILE *wp)
{
  int i;

  fprintf(wp,"%s:",boardno);
  for (i = 0; i < MaxPlayers(); i++)
    fprintf(wp,"%s:",i>=players.Count()?"":players[i]);
  fprintf(wp,"%d:",
    SequentialMoves()?moves.Count():(moves.Count()/players.Count()));
  const char *winner;
  int points;
  if (status.GetWinner(winner,points)) {
    fprintf(wp,"%s won %d points:",winner,points);
  } else if (status.GetTie()) {
    fprintf(wp,"tie:");
  } else if (SequentialMoves()) {
    fprintf(wp,"%s",players[CurrentPlayer()]);
    StringList accepters;
    if (status.GetProposed(accepters)) {
      int i;
      for (i = 0; i < players.Count(); i++)
	if (accepters.Has(players[i]) < 0 && players.Has(players[i]) == i)
	  fprintf(wp," %s",players[i]);
    }
    if (status.GetUndo(accepters)){
      int i;
      for (i = 0; i < players.Count(); i++)
	if (accepters.Has(players[i]) < 0 && players.Has(players[i]) == i)
	  fprintf(wp," %s",players[i]);
    }
    fprintf(wp," turn:");
  } else {
    fprintf(wp,"waiting for");
    int i;
    for (i=0 ; i<players.Count() ; i++) {
      if (hasmoved[i] == 0)
	fprintf(wp," %s",players[i]);
    }
    fprintf(wp,":");
  }
  for (i=0 ; i<parameters.Count() ; i++)
    fprintf(wp," %s",parameters[i]);
  fprintf(wp,"\n");
}

/* Keep a record of what games are present and what their status are     */
void Game::UpdateGameList(int removeit)
{
  static char line[1024];
  char fn[256], tmpn[256];
  char cmp[256];
  char *s;
  FILE *rp,*wp;
  int i;

  sprintf(fn,"%s/etc/%s.list",getenv("PBMHOME"),GameType());
  sprintf(tmpn,"%s/etc/tmp%s",getenv("PBMHOME"),GameType());
  rp = f_open(fn,"r");
  wp = f_open(tmpn,"w");

  fprintf(wp,"boardno:");
  for (i = 0; i < MaxPlayers(); i++) fprintf(wp,"player%i:",i+1);
  fprintf(wp,"moveno:status:parameters\n");

  if (!removeit) {
    PrintGameListLine(wp);
  }
  if (rp) {
    sprintf(cmp,"%s:",boardno);
    fgets(line,sizeof(line),rp); /* not first line */
    while (fgets(line,sizeof(line),rp)) {
      s = strchr(line,'\n');
      if (s)
	*s = '\0';
      if (strncmp(line,cmp,strlen(cmp))) {   /* NOTE: If the comparison*/
	fprintf(wp,"%s\n",line);             /* FAILS, copy this line  */
      }                                      /* else, skip this line.  */
    }
    f_close(rp);
    remove(fn);
  }

  f_close(wp);
  rename(tmpn,fn);
}

/* Rebuild record of what games are present and what their status are     */
int Game::RebuildGameList()
{
  char entry[256];
  char cmd[256];
  char fn[256], tmpn[256];
  FILE *rp,*wp,*dirp;
  int i;

  sprintf(fn,"%s/etc/%s.list",getenv("PBMHOME"),GameType());
  sprintf(tmpn,"%s/etc/tmp%s",getenv("PBMHOME"),GameType());
  rp = f_open(fn,"r");
  wp = f_open(tmpn,"w");

  fprintf(wp,"boardno:");
  for (i = 0; i < MaxPlayers(); i++) fprintf(wp,"player%i:",i+1);
  fprintf(wp,"moveno:status:parameters\n");

  sprintf(cmd, "ls %s/games/%s | grep -v '~$'", getenv("PBMHOME"), GameType());
  dirp = popen(cmd, "r");

  while (fgets(entry, sizeof(entry), dirp)) {
    entry[strlen(entry)-1] = '\0';
    if (Read(entry)) PrintGameListLine(wp);
  }

  f_close(wp);
  remove(fn);
  rename(tmpn,fn);
  return 1;
}

int Game::Player(char piece)
{
    int i;

    for (i=0 ; i<MaxPlayers() ; i++)
        if (piece == PlayerPieces(i))
            return i;
    return i;
}

//  changes:
//   1. commented unused int movect = moves.Count();
//  - akur
int Game::Read(char *b)
{
    int br;
    boardno = b;
    GameFile gamefile;
    if (!gamefile.Read(GameType(),boardno))
        return 0;

    players.Init();
    random.Init();
	totaltimelimit.Init();
	forfeittimelimit.Init();
	nagtimelimit.Init();
    status.SetNormal();
    submitted.Init();
    parameters.Init();
    options.Init();
    subscribers.Init();
	boardnotifiers.Init();
    comment.Init();
    moves.Init();
    hasmoved.Init();
    syncmoves.Init();
    swapped.Init();
    ratingnames.Init();
    ratingold.Init();
    ratingnow.Init();
    int board_read = 0;
    while (!gamefile.IsEof()) {
        if (players.Read(gamefile)) {
            resigned.Init(players.Count());
            pending.Init(players.Count());
            InitClock(gamefile.Modified());
            status.SetNormal();
            Init();
        } else if (!clock.Read(gamefile) &&
            !random.Read(gamefile) &&
            !timer.Read(gamefile) &&
            !totaltimelimit.Read(gamefile) &&
            !forfeittimelimit.Read(gamefile) &&
            !nagtimelimit.Read(gamefile) &&
            !resigned.Read(gamefile) &&
            !pending.Read(gamefile) &&
            !swapped.Read(gamefile) &&
            !status.Read(gamefile) &&
            !ratingnames.Read(gamefile) &&
            !ratingold.Read(gamefile) &&
            !ratingnow.Read(gamefile) &&
            !submitted.Read(gamefile) &&
            !moves.Read(gamefile) &&
            !hasmoved.Read(gamefile) &&
            !syncmoves.Read(gamefile) &&
            !parameters.Read(gamefile) &&
            !options.Read(gamefile) &&
            !subscribers.Read(gamefile) &&
            !boardnotifiers.Read(gamefile) &&
            !comment.Read(gamefile)) {
            if ((br = ReadBoard(gamefile)) != 0) {
                if (br == 1)
                    board_read = 1;
            } else
                gamefile.SkipLine(); // Try to get back in sync
        }
    }

    if (syncmoves.Count() == 0)
        syncmoves.Init(players.Count());

    if (random.Count() == 0) {
        int seed = RandomSeed();
        random.Init().Add(seed).Add(0);
    }

    srand(random[0]);
    for (int i=0 ; i<random[1] ; i++)
        rand();

    gamefile.Close();
    UpdateClock();

//  int movect = moves.Count();
    if (!board_read)
	if (!Regenerate(moves.Count())) return 0;

    previous_moves = moves.Count();
    oldresigned = resigned;

    return 1;
}

int Game::Regenerate(int nmoves) 
{
	is_regenerating = 1;
    // negative nmoves counts back from last move
    if (nmoves<0) nmoves += moves.Count();
    if (nmoves<0) nmoves = 0;		// clip nmoves to valid range
    if (nmoves > moves.Count()) nmoves = moves.Count();
    if (random[1]) {
	srand(random[0]);
	random[1] = 0;
    }
    Init();
	resigned.Init(players.Count());
	pending.Init(players.Count());
    StringList old_moves;
    old_moves.Init();
    for (int m = 0; m < nmoves; m++)
	old_moves.Add(strtolower(trim(moves[m])));
    moves.Init();
    while (moves.Count() < nmoves)
	if (strcasecmp(old_moves[moves.Count()],"resign") &&
		strcasecmp(old_moves[moves.Count()],"forfeit")) {
	    const char *m;
	    if (isSkipMove(old_moves[moves.Count()]))
			m = SkipMove();
	    else {
			player = players[moves.Count()%players.Count()];
			m = MakeMove(old_moves[moves.Count()]);
	    }
	    if (m) moves.Add(m); else { is_regenerating = 0 ; return 0; }
	} else {
		resigned[CurrentPlayer()] = 1;
		old_moves[moves.Count()][0] = toupper(old_moves[moves.Count()][0]);
		moves.Add(old_moves[moves.Count()]);
		int win=0, plrs=0;
		for( int ii=0 ; ii<resigned.Count() ; ii++ ) {
			if (resigned[ii]==0) {
				plrs++;
				win = ii;
			}
		}
		if (plrs==1) {
			status.SetWinner(players[win],GameValue(players[win]));
			PrepareMove();
		}
	}
    const char *winner;
    if (moves.Count() && IsGameOver(winner)) {
		status.SetWinner(winner,GameValue(winner));
		PrepareMove();
    }
	is_regenerating = 0;
    return 1;
}

int Game::Write(void)
{
    GameFile gamefile;
    if (!gamefile.Write(GameType(),boardno))
        return 0;

    players.Write(gamefile);
    if (random[1] != 0) 
        random.Write(gamefile);
    clock.Write(gamefile);
    timer.Write(gamefile);
	totaltimelimit.Write(gamefile);
	forfeittimelimit.Write(gamefile);
	nagtimelimit.Write(gamefile);
    if (resigned.Has(1) >= 0)
        resigned.Write(gamefile);
    if (pending.Has(1) >= 0)
        pending.Write(gamefile);
    if (swapped[0])
        swapped.Write(gamefile);
    if (!status.GetNormal())
        status.Write(gamefile);
    ratingnames.Write(gamefile);
    ratingold.Write(gamefile);
    ratingnow.Write(gamefile);
    submitted.Write(gamefile);
    moves.Write(gamefile);
    hasmoved.Write(gamefile);
    syncmoves.Write(gamefile);
    parameters.Write(gamefile);
    options.Write(gamefile);
    WriteBoard(gamefile);
    subscribers.Write(gamefile);
    boardnotifiers.Write(gamefile);
    comment.Write(gamefile);

    gamefile.Close();

    UpdateActiveCount(status.IsActive());

    return 1;
}

// argv = (boardnumber movenum extra_moves (if any))
int Game::Replay(int argc, char **argv)
{
  char *boardno;

  if (!Read(argv[0]))
      return Error("%s board %s not found",GameType(),argv[0]);
  argc--, argv++;
  int nmoves;
  sscanf(*argv,"%d",&nmoves);
  argc--, argv++;

  // get extra moves if any
  while (argc > 0) {
    if (CanPass() && !Strcmp(strtolower(*argv),"pass"))
      moves.Add(SkipMove());
    else
      moves.Add(strtolower(*argv));
    argc--, argv++;
  }
  previous_moves = moves.Count();

  if (!Regenerate(nmoves)) {
     // Regerate failed: discard changes and apologize.
     Read(boardno);
     comment.Init().Add("PBeM Server writes:\nAttempt to replay was unsuccessful.\nWe apologize for the inconvenience.");
  }

  // Print resulting board
  Mail mail;
  if (Strcmp(getenv("SENDER"),"HTML"))
  {
    mail.Add(getenv("SENDER"));
    Print(mail.fp(), "Summary", 1);
  } else
    PrintForHTML(mail.fp());
  return 0;
}

void Game::UpdateActiveCount(int isactive)
{
	int ii;
	StringList activechange;
	IntList activeincr, refuser;
	activechange.Init();
	refuser.Init();
	activeincr.Init();

	// update active counts if there was a status change
	// must be careful of players playing more than one position in a game
	const char *left;
	if (status.GetHasLeft(left)) {
		if (players.Has(left) < 0) {
			// only if left on first move
			activechange.Add(left);
			activeincr.Add(-1);
			refuser.Add(1);
		}
	}
	for (ii=0 ; ii<players.Count() ; ii++) {
		if (oldresigned[ii]==0 && (resigned[ii]==1 || !isactive)) {
			// handle duplicate players (still active)
			int ok = 1;
			if (isactive) {
				for (int jj=0; jj<players.Count() ; jj++) {
					if (strcmp(players[ii],players[jj])==0 && resigned[jj]==0)
						ok = 0;
				}
			}
			if (ok && activechange.Has(players[ii])<0) {
				activechange.Add(players[ii]);
				activeincr.Add(-1);
				refuser.Add(0);
			}
		}
		if (oldresigned[ii]==1 && resigned[ii]==0) {
			// handle duplicate players (already active)
			int ok = 1;
			for (int jj=0; jj<players.Count() ; jj++) {
				if (strcmp(players[ii],players[jj])==0 && oldresigned[jj]==0)
					ok = 0;
			}
			if (ok && activechange.Has(players[ii])<0) {
				activechange.Add(players[ii]);
				activeincr.Add(1);
				refuser.Add(0);
			}
		}
	}
	if (activechange.Count()) {
		limits.ReadFile(GameType());
		for (ii=0 ; ii<activechange.Count() ; ii++) {
			int act = limits.Active(activechange[ii]);
			act += activeincr[ii];
			limits.UpdateActive(activechange[ii],act);
			if (activeincr[ii]>0) {
				int seek = limits.Seek(activechange[ii]) - 1;
			       	if (seek<0) { seek = 0; }
				limits.UpdateSeek(activechange[ii],seek);
			}
			// if you now have no current games and you refused to play you become
			// inactive.
			if (act == 0 && refuser[ii])
			  limits.UpdateInactivePlayer(activechange[ii],1);
			// if your game just ended and it wasn't because you refused, and you were
			// inactive then you become active again.
			if (activeincr[ii] < 0 && !refuser[ii] && limits.InactivePlayer(activechange[ii]))
			  limits.UpdateInactivePlayer(activechange[ii],0);
		}
		limits.WriteFile(GameType());
	}
}


//  changes:
//   1. recognize both "--" (standard signature) and "__" (HoTMaiL signature)
//      and "--=20" instead of only "--"
//   2. use " **" instead of " ** " (no need for the trailing space, after
//      all)
//   3. a typo in "%s has decided not to play. The game has been re-startet.\n\n"
//      has been corrected (to "re-started")
//   4. now named PrintAs and have more arguments (Print still works for old games)
//   5. calls PrintBoardAs instead of PrintBoard
//   6. displays visible moves only -- useful for some games where opponent's moves
//      should not be displayed. A move is visible if the game has ended, it is the
//      player's own move, or the game uses the default VisibleMoves() = 1. An
//      invisible move is displayed as an ellipsis ("...").
//  - akur
void Game::PrintAs(FILE *fp, const char *type, int what, int which, int all)
{
    int game_over = !status.IsActive();

    if (!Strcmp(type,"New")) {
        fprintf(fp,"Subject: New %s Board %s %s%s\n\n",GameType(),boardno,
            players[CurrentPlayer()],
            (comment.Count()>1 && strncmp(comment[1],"-- ",3) && strncmp(comment[1],"__",2) && strncmp(comment[1],"--=20",5) && strncmp(comment[1],"      ______",12))?" **":"");
		MIME_Header(fp);
        fprintf(fp,"A new %s board (%s) has been started\n\n",GameType(),boardno);
    } else if (Strcmp(type,"Error")) { // Error: Subject has been printed
        if (SequentialMoves())
            fprintf(fp,"Subject: %s Board %s %s%s\n\n",GameType(),boardno,
                game_over?"":players[CurrentPlayer()],
                (comment.Count()>1 && strncmp(comment[1],"-- ",3) && strncmp(comment[1],"__",2) && strncmp(comment[1],"--=20",5) && strncmp(comment[1],"      ______",12))?" **":"");
        else
            fprintf(fp,"Subject: %s Board %s %s%s\n\n",GameType(),boardno,
                game_over?"":hasmoved.Has(1)>=0?"confirmation":"ALL",
                (comment.Count()>1 && strncmp(comment[1],"-- ",3) && strncmp(comment[1],"__",2) && strncmp(comment[1],"--=20",5) && strncmp(comment[1],"      ______",12) && Strcmp(type,"Notification"))?" **":"");
		MIME_Header(fp);
        fprintf(fp,"%s of %s Board %s\n\n",type,GameType(),boardno);
    } else {
        fprintf(fp,"Summary of %s Board %s\n\n",GameType(),boardno);
    }
    

    const char *name;
    int points;
    if (status.GetHasLeft(name))
        fprintf(fp, "%s has decided not to play. The game has been re-started.\n\n",
            name);

    if (!Strcmp(type,"Submit") && submitted.Count() > 0)
        fprintf(fp,"This is a preview of a submitted move.  It must be confirmed to take effect.\n\n");

    if (status.GetSwapped(name))
        fprintf(fp, "%s has swapped sides.\n\n",name);
    StringList accepters;
    if (status.GetProposed(accepters)) {
        fprintf(fp,"%s has proposed a draw.\nThe following players still need to accept or reject this:\n",
            accepters[0]);
        int i;
        for (i = 0; i < players.Count(); i++)
            if (accepters.Has(players[i]) < 0 && players.Has(players[i]) == i)
                fprintf(fp,"\t%s\n",players[i]);
        fprintf(fp,"\n");
    }
    if (status.GetUndo(accepters)) {
        fprintf(fp,"%s has requested to undo a move.\nThe following players still need to accept or reject this:\n",
            accepters[0]);
        int i;
        for (i = 0; i < players.Count(); i++)
            if (accepters.Has(players[i]) < 0 && players.Has(players[i]) == i)
                fprintf(fp,"\t%s\n",players[i]);
        fprintf(fp,"\n");
    }
    
    if (status.GetWinner(name,points)) {
        fprintf(fp, "Game won by %s",name);
        if (points > 1)
            fprintf(fp, " by %d points", points);
        fprintf(fp,".\n\n");
    }
    else if (status.GetTie())
        fprintf(fp, "Game was a tie.\n\n");
    else if (SequentialMoves()) {
        if (moves.Count() == 0)
            fprintf(fp, "Please start the game %s.\n\n",players[0]);
        else
            fprintf(fp, "It is %s\'s turn.\n\n",players[CurrentPlayer()]);

        // display any queued moves here (if AS_PLAYER)
            if (what == AS_PLAYER && syncmoves[which] && Strcmp(syncmoves[which],"")) {
                fprintf(fp,"\nYou have the following move queued:\n\t%s\n\n",syncmoves[which]);
    }

    } else {
        if (hasmoved.Has(0)>=0) {
            fprintf(fp, "Waiting for: ");
            int i;
            for (i=0 ; i<players.Count() ; i++)
                if (hasmoved[i] == 0)
                    fprintf(fp," %s",players[i]);
        }
        fprintf(fp,"\n\n");
    }

    if (ratingnames.Count() > 0) {
        int i;
        for (i = 0; i < ratingnames.Count(); i++) {
            if (ratingold[i] > 0 && ratingnow[i] > 0) {
                if (ratingold[i] != ratingnow[i]) {
                    fprintf(fp,"%s's rating went %sfrom %d to %d.\n",
                            ratingnames[i],
                            ratingold[i]<ratingnow[i]?"up ":"down ",
                            ratingold[i],ratingnow[i]);
                } else {
                    fprintf(fp,"%s's rating is %d.\n",
                            ratingnames[i], ratingnow[i]);
                }
            } else {
                fprintf(fp,"%s's rating is %d (provisional).\n",
                        ratingnames[i], ratingnow[i]);
            }
        }
        fprintf(fp,"\n");
    } 
    else if (game_over) {
	fprintf(fp,"This game was not rated.\n\n");
    }

    if (PrintColoumns()) {
        int i;
        unsigned int colwidth = MoveWidth()+4;
        if (colwidth < 12)
            colwidth = 12;

        for (i = 0; i < players.Count(); i++) {
            char p[20];
            sprintf(p, PlayerNames(i), i+1);
            fprintf(fp,"   %-*s",colwidth-2,p);
        }
        fprintf(fp,"\n");
        for (i = 0; i < players.Count(); i++)
            fprintf(fp,"   %-*s",colwidth-2,players[i]);
        fprintf(fp,"\n");
        for (i=all||game_over?0:MIN(previous_moves,MAX(((moves.Count()+1)/players.Count()-MoveHeight())*players.Count(),0));i<moves.Count();i++) {
	  char mv[500]; // ChuShogi handicap "moves" can be very long...
            strcpy(mv,moves[i]);
            if (status.IsActive() && (what != 1 || which != i % players.Count()) && !VisibleMoves())
                strcpy(mv, "...");
            if (SequentialMoves()) {
                if (i >= previous_moves) strcat(mv," (Auto)");
                fprintf(fp," %3d %-*s",i+1,colwidth-4,mv);
            } else {
                fprintf(fp," %3d %-*s",i/players.Count()+1,colwidth-4,mv);
            }
            if ((i+1)%players.Count() == 0)
                fprintf(fp, "%s\n", MoveComment(i));
            if (i<moves.Count()-1 && (i+1)%players.Count()!=0 && strlen(mv) > colwidth-3)
                // We need to skip to the next line
                fprintf(fp,"\n%*s",(colwidth+1)*((i+1)%players.Count()),"");
        }
        if ((i+1)%players.Count()==0)
            fprintf(fp,"\n");
    } else {
        int i;
        for (i = 0; i < players.Count(); i++) {
            fprintf(fp,PlayerNames(i),i+1);
            fprintf(fp,": %s\n",players[i]);
        }
        fprintf(fp,"\n");
        for (i=all||game_over?0:MIN(previous_moves,MAX(moves.Count()-MoveHeight(),0)); i<moves.Count(); i++) {
            fprintf(fp,PlayerNames(i%players.Count()),i%players.Count()+1);
            if (status.IsActive() && (what != 1 || which != i % players.Count()) && !VisibleMoves())
                fprintf(fp," Move %d: %s%s\n",i+1,"...",i>=previous_moves?" (Auto)":"");
            else
                fprintf(fp," Move %d: %s%s\n",i+1,moves[i],i>=previous_moves?" (Auto)":"");
        }
    }
    if (!SequentialMoves() && status.IsActive() && hasmoved.Has(1)>=0) {
        int colwidth = MoveWidth()+4;
        if (colwidth < 12)
            colwidth = 12;
        fprintf(fp," ");
        int i;
        for (i=0 ; i<players.Count() ; i++)
            fprintf(fp,"%3d %-*s%s",
                    moves.Count()/players.Count()+1,
                    colwidth-4,(((player && !Strcmp(player,players[i]))||hasmoved[i]==2)?syncmoves[i]:(hasmoved[i]==0?"":"???")),
                    (i+1)%players.Count()?" ":"\n");
    }
    fprintf(fp,"\n\n");

    PrintBoardAs(fp, what, which);

    if (Strcmp(type, "Notification")) {
        subscribers.Print(fp);
	boardnotifiers.Print(fp,!subscribers.Count());
        comment.Print(fp);
    }

    if (!status.IsActive() || NagTime() != 7 || ForfeitTime() != 21 || TotalTimeAllowed() || (!isdigit(*boardno) && Strcmp(boardno, "Preview")) ||
        opts.Has("--nagtime")>=0 || opts.Has("--forfeittime")>=0) {
        fprintf(fp,"\n\n************************************************************\n\n");
#if defined(WIN32) && !defined(unix)
#define TIMEFORMAT "%b %d %H:%M"
#else
#define TIMEFORMAT "%b %e %R"
#endif

        char fn[100];
        time_t t;
        struct tm *ts;
        int i;
        for (i = 0; i < 3; i++) {
            ts = localtime(&(t=(time_t)clock[i]));
            strftime(fn,sizeof(fn),TIMEFORMAT,ts);
            switch (i) {
            case 0: fprintf(fp,"Game started...."); break;
            case 1: fprintf(fp,"Last move made.."); break;
            case 2: fprintf(fp,status.IsActive() ? 
                               "It is now......." :
                               "Game ended......");    break;
            }
            fprintf(fp," %s\n",fn);
        }

        if (NagTime() != 7 || ForfeitTime() != 21)
            fprintf(fp,"\n");
        if (NagTime() != 7)
            fprintf(fp,"Nag Timeout set to %d days\n",NagTime());
        if (ForfeitTime() != 21)
            fprintf(fp,"Forfeit Timeout set to %d days\n",ForfeitTime());

        unsigned int longest = 0;
        for (i = 0; i < players.Count(); i++)
            if (strlen(players[i]) > longest)
                longest = strlen(players[i]);
        fprintf(fp,"\nTime used by players:");
	if (TotalTimeAllowed())
		fprintf(fp,"  (Total Time Allowed per player is %d days)",TotalTimeAllowed());
	fprintf(fp,"\n");
        int days,hours,seconds,minutes;
        for (i = 0; i < players.Count(); i++) {
            ConvertTime(timer[i],days,hours,minutes,seconds);
            fprintf(fp,"%-*s (%s): %3d days, %02d:%02d:%02d\n",
                longest,players[i],PlayerNames(i),
                days,hours,minutes,seconds);
        }
        ConvertTime(TotalTime(),days,hours,minutes,seconds);
        fprintf(fp,"%-*s: %3d days, %02d:%02d:%02d\n",
            (int)(longest+3+strlen(PlayerNames(0))), "Total",
                days,hours,minutes,seconds);
    }
    fprintf(fp,"\n");
}

void Game::InitClock(int mtime)
{
    int i;
	// look for time-related options
	for( i=0 ; i<options.Count() ; i++ ) {
		if (strncasecmp(options[i],"--totaltime=",12)==0) {
			totaltimelimit.Init().Add(atoi(options[i]+12));
		}
		if (strncasecmp(options[i],"--forfeittime=",14)==0) {
			forfeittimelimit.Init().Add(atoi(options[i]+14));
		}
		if (strncasecmp(options[i],"--nagtime=",10)==0) {
			nagtimelimit.Init().Add(atoi(options[i]+10));
		}
	}

    clock.Init();
    if (mtime)
        clock.Add(mtime).Add(mtime).Add(mtime).Add(mtime);
    else
        clock.Add(official_time).Add(official_time).Add(official_time).Add(official_time);
    timer.Init(players.Count());
}

/* Timer Logic:
 *	clock[0] = start time
 *	clock[1] = time of last move
 *	clock[2] = now or end time
 *  clock[3] = previous value of clock[2], just in case anyone needs it
 * when reading gamefile:
 *  set clock[2] = now
 *  update timer for player(s) whose turn is pending
 *  (even if a vote in going on)
 *
 * NextPlayer updates clock[1]
 */
void Game::UpdateClock(void)
{
    if (status.IsActive()) {

		// update timers
		int interval = official_time - clock[2];
	    if (SequentialMoves()) {
			timer[CurrentPlayer()] += interval;
	    } else {
			for (int i=0 ; i<players.Count() ; i++)
				if (!hasmoved[i]) 
					timer[i] += interval;
	    }

		clock[3] = clock[2];
        clock[2] = official_time;
    }
}

int Game::TotalTime(void)
{
    /* Return time since game started. If game is no longer active, return the
     * total playing time */
    return clock[2]-clock[0];
}

int Game::BoardAge(void)
{
    /* Return time since last move. */
    return official_time-clock[1];
}

void Game::ConvertTime(int timer, int &days, int &hours, int &minutes, int &seconds)
{
    seconds = timer%60; timer /= 60;
    minutes = timer%60; timer /= 60;
    hours   = timer%24; timer /= 24;
    days    = timer;
}

int Game::NextPlayer(const char *&winner)
{
    if (SequentialMoves())
		clock[1] = clock[2] = official_time;
    int count = 0;
    int i;
    for (i = 0; i < resigned.Count(); i++)
        if (!resigned[i])
            count++;
    if (count == 1) {
        winner = players[resigned.Has(0)];
        if (!SequentialMoves())
            for (i=0 ; i<players.Count() ; i++)
                moves.Add(syncmoves[i]);
        return 0;
    }
    if (SequentialMoves()) {
        const char *move;
        for (;;) {
            if (IsGameOver(winner))
                return 0; /* No next player, the game has ended */
            else if (pending[CurrentPlayer()]) {
                return 1;
            }
            else if (resigned[CurrentPlayer()]) {
                moves.Add(SkipMove());
                PrepareMove();
            }
            else if (syncmoves[CurrentPlayer()] && Strcmp(syncmoves[CurrentPlayer()],"")) {
                char envstr[1024];
                char *tmpmove = Strdup(syncmoves[CurrentPlayer()]);
                syncmoves.Change(CurrentPlayer(), "");
                Write();
                // reset $ENV{SENDER} and $ENV{INPUTFILE} so the correct
                // player gets notified of an error
                player = players[CurrentPlayer()];
                snprintf(envstr,sizeof(envstr),"SENDER=%s",UserAddress(player));
                putenv(envstr);
                putenv("INPUTFILE=/dev/null");
                const char *makemove = MakeMove(tmpmove);
                if (makemove==NULL) {
                    Mail mail(boardno);
                    mail.Add(player);
                    FILE *fp = mail.fp();
                    fprintf(fp,"Subject: Unexpected error (%s Board %s)\n\n",GameType(),boardno);
					MIME_Header(fp);
                    fprintf(fp,"Queued move \"%s\" was found but is illegal\n",tmpmove);
                    free(tmpmove);
                    Print(fp,"Error",1);
                    break;
                }
                free(tmpmove);
                moves.Add(makemove);
                PrepareMove();
            } else if (MustSkip()) {
                moves.Add(SkipMove());
                PrepareMove();
            } else if ((move = ForcedMove()) != NULL) {
                player = players[CurrentPlayer()];
                const char *makemove = MakeMove(move);
                if (makemove==NULL) {
                    Mail mail(boardno);
                    mail.Add("rrognlie@gamerz.net");
                    FILE *fp = mail.fp();
                    fprintf(fp,"Subject: Unexpected error (%s Board %s)\n\n",GameType(),boardno);
					MIME_Header(fp);
                    fprintf(fp,"Forced move \"%s\" was found but is illegal\n",move);
                    Print(fp,"Error",1);
                    break;
                }
                moves.Add(makemove);
                PrepareMove();
            }
            else
                break;
        }
    } else {
        for (;;) {
            if (hasmoved.Has(0)>=0)    // waiting on any other moves?
                break;
			clock[1] = clock[2] = official_time;
            ProcessRound();
            for (i=0 ; i<players.Count() ; i++)
                moves.Add(syncmoves[i]);
            if (IsGameOver(winner))
                return 0;
            for (i=0 ; i<players.Count() ; i++) {
                hasmoved[i] = (resigned[i] ? 2 : 0);
                syncmoves.Change(i, resigned[i] ? SkipMove() : "");
            }
	    PrepareMove();
        }
    }
    return 1; /* Let next player move */
}

//--------
// IsRated
//	Determine if a game should be rated or not
//	(this version does not rate handicapped games)
//--------
int Game::IsRated()
{
	int i;
	for (i=0 ; i<parameters.Count() ; i++) 
		if (abbrev(parameters[i],"-handicap",4))
			return 0;
	return 1;
}

//  changes:
//   1. changed Print to PrintAs(AS_NOTIFIER, i)
//  - akur
void Game::GameOver(const char *winner)
{
    int points = GameValue(winner);
    status.SetWinner(winner,points);
    pending.Init();
    if (isdigit(boardno[strlen(boardno)-1])) {
        // Update Ratings
        Ratings ratings(GameType());
	if (IsRated()) {
	        ratings.GameOver(winner,players,resigned,points,boardno);
	}
        if (!isdigit(*boardno)) {
            // Update Standings
            TournamentStandings standings(GameType(),boardno);
            standings.GameOver(winner,players);
        }
        ratings.GetChange(ratingnames,ratingold,ratingnow);
    }
    // Notify
    notifiers.ReadFile(GameType());
    notifiers.Add(boardnotifiers);
    int i;
    for (i=0 ; i<notifiers.Count() ; i++) {
        Mail mail(boardno);
        mail.AddUser(player=notifiers[i]);
        FILE *fp = mail.fp();
        PrintAs(fp, "Notification", AS_NOTIFIER, i, 1);
    }
}

int Game::Scan(int argc,char **argv,int (Game::*fct)(char *who,FILE *fp),char *who,FILE *fp,StringList &success,StringList &failure,int restrict)
{
    success.Init();
    failure.Init();
    char *ptr = (argc>0) ? strrchr (*argv, '#') : (char *)NULL;
    if (ptr) *ptr++ = '\0';
    int is_board = Read(*argv);
    if (ptr) ptr[-1] = '#';		// if we stripped '#', restore it
    if (argc > 0 && is_board) {
        // All args are board names
        while (argc > 0) {
	    if ( is_board && (!ptr || Regenerate(atoi(ptr))) && (this->*fct)(who,fp) )
                success.Add(*argv);
            else
                failure.Add(*argv);
            argc--, argv++;
            if (argc) {
		if ((ptr = strrchr (*argv, '#')) != NULL) *ptr++ = '\0';
                is_board = Read(*argv);
		if (ptr) ptr[-1] = '#';
	    }
        }

    } else {
        // Args are selectors
        int active = 0, inactive = 0, next = 0, tournament = 0, standard = 0;
        StringList play;
	// perspective is set by ShowAs
        if (perspective)
            play.Add(perspective);
        while (argc > 0) {
            if (!Strcmp(*argv, "active"))
                active = 1;
            else if (!Strcmp(*argv, "inactive"))
                inactive = 1;
            else if (!Strcmp(*argv, "next"))
                next = 1;
            else if (!Strcmp(*argv, "tournament"))
                tournament = 1;
            else if (!Strcmp(*argv, "standard"))
                standard = 1;
            else if (!Strcmp(*argv, "all"))
                ; // Place holder for no arguments. Allows "show all".
			else if (!Strcmp(*argv, "board"))
				; // some users say 'show board 123'
            else
                play.Add(*argv);    /* Ok to show invalid players -- 
                                     * they may have left pbmserv */
            argc--, argv++;
        }
        if (active + inactive + next > 1)
            return Error("Cannot process games that are more than one of active, inactive or next");
        if (tournament + standard > 1)
            return Error("Cannot process games that are more than one of tournament or standard");
        if (next && play.Count() < 1)
            return Error("You must specify which player(s) to be next");
        if (restrict && play.Count()==0 && !tournament)
            return Error("This would generate too much mail -- please refine request");
        char cmd[256];
        sprintf(cmd, "ls %s/games/%s | grep -v '~$'", getenv("PBMHOME"), GameType());
        FILE *dirp = popen(cmd, "r");

        char entry[256];
        while (fgets(entry, sizeof(entry), dirp)) {
            entry[strlen(entry)-1] = '\0';
            int display = 1;
            if (!Read(entry))
                display = 0;
            else if (tournament && isdigit(*boardno))
                display = 0;
            else if (standard && !isdigit(*boardno))
                display = 0;
            else if (active && !status.IsActive())
                display = 0;
            else if (inactive && status.IsActive())
                display = 0;
			else if (next && !status.IsActive())
				display = 0;
            else if (next && SequentialMoves() && play.Has(players[CurrentPlayer()]) < 0)
                display = 0;
            else if (next && !SequentialMoves()) {
				int i;
                display = 0;
				for (i=0 ; i<players.Count() ; i++) {
					if (!hasmoved[i] && !resigned[i] && play.Has(players[i]) >= 0) {
                        display = 1;
						break;
                    }
				}
			} else {
                display = (play.Count() == 0);
                int i;
                for (i = 0; i < play.Count(); i++)
                    if (players.Has(play[i]) >= 0) {
                        display = 1;
                        break;
                    }
            }
            if (display)
                if ((this->*fct)(who,fp))
                    success.Add(entry);
                else
                    failure.Add(entry);
        }

        pclose(dirp);
    }
    return 1;
}

int Game::Main(int argc,char **argv)
{
    int rc=0;
    if (argc==1) return 0;

    MassageArgs(argc,argv);
    int seed = RandomSeed();
    random.Init().Add(seed).Add(0);
    srand(random[0]);

    if (!LockFile()) return 0;

    if (abbrev(argv[1],"challenge",5))
        rc = Challenge(argc-2,argv+2);
    else if (!Strcmp(argv[1],"standings") || !Strcmp(argv[1],"ratings"))
        rc = Standings(argc-2,argv+2);
    else if (!Strcmp(argv[1],"show"))
        rc = Show(argc-2,argv+2);
    else if (!Strcmp(argv[1],"showas"))
        rc = ShowAs(argc-2,argv+2);
    else if (!Strcmp(argv[1],"move") || !Strcmp(argv[1],"board") ||
	     !Strcmp(argv[1],"mvoe"))
        rc = Move("move",argc-2,argv+2);
    else if (!Strcmp(argv[1],"qmove") || !Strcmp(argv[1],"qboard") ||
             !Strcmp(argv[1],"moveq") || !Strcmp(argv[1],"boardq") ||
	         !Strcmp(argv[1],"queue"))
        rc = Move("queue",argc-2,argv+2);
    else if (!Strcmp(argv[1],"chat"))
        rc = Chat(argc-2,argv+2);
    else if (!Strcmp(argv[1],"submit"))
        rc = Submit(argc-2,argv+2);
    else if (!Strcmp(argv[1],"confirm"))
        rc = Confirm(argc-2,argv+2);
    else if (!Strcmp(argv[1],"swap"))
        rc = Swap(argc-2,argv+2);
    else if (!Strcmp(argv[1],"resign"))
        rc = Resign(argc-2,argv+2);
    else if (abbrev(argv[1],"preview",4))
        rc = Preview(argc-2,argv+2);
    else if (!Strcmp(argv[1],"history"))
        rc = History(argc-2,argv+2);
    else if (!Strcmp(argv[1],"games"))
        rc = Games(argc-2,argv+2);
    else if (!Strcmp(argv[1],"help"))
        rc = Help(argc-2,argv+2);
    else if (abbrev(argv[1],"subscribe",3))
        rc = Subscribe(argc-2,argv+2);
    else if (abbrev(argv[1],"unsubscribe",5))
        rc = Unsubscribe(argc-2,argv+2);
    else if (!Strcmp(argv[1],"accept"))
        rc = Accept(argc-2,argv+2);
    else if (!Strcmp(argv[1],"reject"))
        rc = Reject(argc-2,argv+2);
    else if (!Strcmp(argv[1],"limit"))
        rc = Limit(argc-2,argv+2);
    else if (!Strcmp(argv[1],"seek"))
        rc = Seek(argc-2,argv+2);
    else if (!Strcmp(argv[1],"notify"))
        rc = Notify(argc-2,argv+2);
    else if (!Strcmp(argv[1],"Cleanup"))
        rc = Cleanup(argc-2,argv+2);
    else if (!Strcmp(argv[1],"rebuildgamelist"))
        rc = RebuildGameList();
    else if (!Strcmp(argv[1],"rebuildactivecount"))
        rc = RebuildActiveCount();
    else if (!Strcmp(argv[1],"echo")) {
        // for debugging
        Mail mail;
        if (Strcmp(getenv("SENDER"),"HTML"))
        {
            mail.Add(getenv("SENDER"));
        }
        FILE *fp = mail.fp();
        fprintf(fp, "Subject: %s echo\n\n",GameType());
        fprintf(fp, "argc=%d\n", argc);
        int i;
        for (i=0; i<argc; ++i)
            fprintf(fp, "argv[%d]=\"%s\"\n", i, argv[i]);
		//fprintf(fp,"\n");

		//FILE *envp = popen("env", "r");
		//char envl[256];
        //while (fgets(envl,sizeof(envl),envp)) {
			//char *cr = strchr(envl,'\n');
			//if (cr)    *cr = '\0';
			//fprintf(fp,"%s\n",envl);
		//}
		//fclose(envp);
		//fprintf(fp,"\n");
		rc = 0;
    }
    else if (!Strcmp(argv[1],"replay"))
        rc = Replay(argc-2,argv+2);
    else
        rc = UnknownCommand(argc,argv);

    UnlockFile();

    return rc;
}

//  changes:
//   1. changed Print to PrintAs(AS_PLAYER, i)
//   2. changed Print to PrintAs(AS_SUBSCRIBER, i)
//  - akur
int Game::Challenge(int argc, char **argv)
{
    char fn[256];
    sprintf(fn,"%s/games/%s/.special",getenv("PBMHOME"),GameType());
    FILE *fp = f_open(fn,"r");
    int IsSpecial=(fp != NULL);
    // Check for correct number of users
    if (argc < MinPlayers())
        return Error("At least %d players must play",MinPlayers());
    if (argc > MaxPlayers())
        return Error("At most %d players can play",MaxPlayers());
    // Validate users
    StringList invalid, dontwant, overlimit;
    limits.ReadFile(GameType());
    players.Init();
    resigned.Init();
    while (argc > 0) {
        if (CheckUserid(player=*argv,"") == BAD_USER)
            invalid.Add(*argv);
        else if (limits.Limit(*argv)==REJECTALL) 
            dontwant.Add(*argv);
        else if (limits.Active(*argv) >= limits.Limit(*argv) && limits.Limit(*argv) != ACCEPTALL)
            overlimit.Add(*argv);
	// always add to players; if one is bad, they are not used anyway
        players.Add(*argv);
        resigned.Add(0);
        oldresigned.Add(1);  // for UpdateActiveCount
        argc--, argv++;
    }
    if (!IsSpecial && (invalid.Count()||dontwant.Count()||overlimit.Count())) {
        Mail mail;
        if (Strcmp(getenv("SENDER"),"HTML"))
        {
            mail.Add(getenv("SENDER"));
        }
        FILE *fp = mail.fp();
        fprintf(fp,"Subject: Invalid challenge\n\n");
		MIME_Header(fp);
        if (invalid.Count()) {
            fprintf(fp,"The following userid\'s are not recognized:\n");
            int i;
            for (i=0; i<invalid.Count(); i++)
                fprintf(fp, "\t%s\n",invalid[i]);
            fprintf(fp,"\n");
        }
        if (dontwant.Count()) {
            fprintf(fp,"The following users are not accepting %s challenges:\n",GameType());
            int saveQ = 0;
            for (int i=0; i<dontwant.Count(); i++){
                fprintf(fp,"\t%s\n",dontwant[i]);
		if (limits.Active(dontwant[i]) == 0){
                    // not currently playing, make player inactive
		    limits.UpdateInactivePlayer(dontwant[i],1);
		    saveQ = 1;
		}
	    }
            fprintf(fp,"\n");
	    if (saveQ) limits.WriteFile(GameType());
        }
        if (overlimit.Count()) {
            fprintf(fp,"The following users have reached their limit of active %s games:\n",GameType());
            int i;
            for (i=0; i<overlimit.Count(); i++)
                fprintf(fp,"\t%s (%d)\n",overlimit[i],limits.Limit(overlimit[i]));
            fprintf(fp,"\n");
        }
        return 0;
    }
    // Get new board number
    if (fp) {
        char boardstr[16];
        fgets(boardstr,sizeof(boardstr),fp);
        if (boardstr[strlen(boardstr)-1] == '\n')
            boardstr[strlen(boardstr)-1] = '\0'; 
        f_close(fp);
        unlink(fn);
        boardno = Strdup(boardstr);
    }
    else {
        sprintf(fn,"%s/games/%s/.board",getenv("PBMHOME"),GameType());
        fp = f_open(fn,"r");
        int boardnumber;
        if (fp) {
            fscanf(fp,"%d",&boardnumber);
            f_close(fp);
        } else {
            boardnumber = 100;
        }
        fp=f_open(fn,"w");
        fprintf(fp,"%d\n",++boardnumber);
        f_close(fp);
        char boardstr[16];
        sprintf(boardstr,"%d",boardnumber);
        boardno = Strdup(boardstr);
    }

    int seed = RandomSeed();
    random.Init().Add(seed).Add(0);
    srand(random[0]);

    // Initialize board
    parameters.Init(parms);
    parms.Init();
    options.Init(opts);
    opts.Init();
    if (!Init())
        return 0;
    status.SetNormal();
    // Write board
    comment.GetNew("Challenger");
    if (!SequentialMoves()) {
        hasmoved.Init(players.Count());
    }
    syncmoves.Init(players.Count());
    InitClock(0);
    PrepareMove();
    const char *winner;
    if (!NextPlayer(winner))
        GameOver(winner);
    // Check for "subscribe all"'s
    sprintf(fn,"%s/games/%s/.subscribeall",getenv("PBMHOME"),GameType());
    fp = f_open(fn,"r");
    if (fp!=NULL) {
      char who[10];    
      fscanf(fp,"%s",who);
      while(!feof(fp)) {
        if (subscribers.Has(who) < 0 && players.Has(who) < 0 && status.IsActive())
          subscribers.Add(who);
        fscanf(fp,"%s",who);
      }
      f_close(fp);
    }
    Write();
    UpdateGameList(0);

    // Send mail to all players
    int i;
    for (i=0 ; i<players.Count() ; i++) {
        Mail mail(boardno);
        mail.AddUser(player=players[i]);
        PrintAs(mail.fp(), "New", AS_PLAYER, i, 1);
    }
    for (i=0 ; i<subscribers.Count() ; i++) {
        Mail mail(boardno);
        mail.AddUser(player=subscribers[i]);
        PrintAs(mail.fp(), "New", AS_SUBSCRIBER, i, 1);
    }
    if (!Strcmp(getenv("SENDER"),"HTML"))
    {
        Mail mail(boardno);
        PrintAs(mail.fp(), "New", AS_SUBSCRIBER, i, 1);
    }
    return 1;
}

int Game::RebuildActiveCount()
{
	Limits oldlimits;
	oldlimits.ReadFile(GameType());
	limits.ReadFile(GameType());
	limits.ResetActive();

	char *argv = "active";
	StringList success, failure;
	Scan(1,&argv,&Game::doActiveScan,NULL,NULL,success,failure);

	limits.WriteFile(GameType());

	if (parms.Has("-nomail")>=0) return 1;

	int errs=0, ii;
	Mail mail;
    if (Strcmp(getenv("SENDER"),"HTML"))
    {
        mail.Add(getenv("SENDER"));
    }
	FILE *fp = mail.fp();
	fprintf(fp,"Subject: %s Rebuild Active Count Results\n\n",GameType());
	MIME_Header(fp);
	for ( ii=0; ii<oldlimits.Count(); ii++ ) {
		if (oldlimits.Active(ii) != limits.Active(oldlimits.Name(ii))) {
			if (!errs++)
				fprintf(fp,"Player Name          Old Count New Count\n-------------------- --------- ---------\n");
			fprintf(fp,"%-20s %9d %9d\n",oldlimits.Name(ii),oldlimits.Active(ii),limits.Active(oldlimits.Name(ii)));
		}
	}
	for ( ii=0; ii<limits.Count(); ii++ ) {
		if (oldlimits.Has(limits.Name(ii))<0) {
			if (!errs++)
				fprintf(fp,"Player Name          Old Count New Count\n-------------------- --------- ---------\n");
			fprintf(fp,"%-20s %9d %9d\n",limits.Name(ii),oldlimits.Active(limits.Name(ii)),limits.Active(ii));
		}
	}
	fprintf(fp,"\nTotal Discrepancies: %d\n",errs);

        return 1;
}


int Game::doActiveScan(char *, FILE*)
{
        if (!status.IsActive())
                return 0;

        int ii;
        StringList active;
        active.Init();
        for (ii=0 ; ii<players.Count() ; ii++) {
                if (!resigned[ii] && active.Has(players[ii])<0)
                        active.Add(players[ii]);
        }
        for (ii=0 ; ii<active.Count() ; ii++) {
		limits.UpdateActive(active[ii],limits.Active(active[ii])+1);
        }
        return active.Count();
}


int Game::Standings(int argc, char **argv)
{
    //(void)argc;
    //(void)argv;
        
    Mail mail;
    if (Strcmp(getenv("SENDER"),"HTML"))
    {
        mail.Add(getenv("SENDER"));
    }

    Ratings ratings(GameType());
    ratings.Display(mail.fp());

    TournamentStandings standings(GameType());
    standings.Display(mail.fp());

    return 0;
}

int Game::doShow(char *who, FILE *)
{
    Mail mail(boardno);
    if (Strcmp(getenv("SENDER"),"HTML"))
    {
        mail.Add(who);
    }
	if (SequentialMoves()) {
		player = players[CurrentPlayer()];
	}
    if (!perspective || players.Has(perspective)<0) {
        Print(mail.fp(), "Summary", 1);
    } else {
		player = perspective;
        for (int i = 0; i < players.Count(); i++)
            if (!strcmp(players[i], perspective))
                PrintAs(mail.fp(), "Summary", AS_PLAYER, i, 1);
    }
    return 1;
}

int Game::Show(int argc, char **argv)
{
    if (argc < 1)
        return Error("Syntax is \"%s show <board>...<board>\"",GameType());

    StringList success, failure;
    if (!Scan(argc,argv,&Game::doShow,getenv("SENDER"),NULL,success,failure,1))
        return 0;

    if (failure.Count()) {
        int i;
        Mail mail;
        if (Strcmp(getenv("SENDER"),"HTML"))
        {
            mail.Add(getenv("SENDER"));
        }
        FILE *fp = mail.fp();
        fprintf(fp,"Subject: Show summary\n\n");
		MIME_Header(fp);
        if (success.Count()) {
            fprintf(fp,"The following board%s successfully shown:\n  ", success.Count()!=1?"s were":" was");
            for (i = 0; i < success.Count(); i++)
                fprintf(fp," %s",success[i]);
        }
        fprintf(fp,"\n\nThe following board%s not shown:\n  ", failure.Count()!=1?"s were":" was");
        for (i = 0; i < failure.Count(); i++)
            fprintf(fp," %s",failure[i]);
        fprintf(fp,"\n");
    }
    return 1;
}

int Game::ShowAs(int argc, char **argv)
{
    if (argc < 3)
        return Error("Syntax is \"%s showas <userid> <password> <board>...<board>\"",GameType());

    switch (CheckUserid(player=argv[0],argv[1])) {
        case BAD_PASSWORD: return Error("Invalid password entered");
        case BAD_USER: return Error("Invalid userid entered");
        case GOOD_USER: ;
    }
    perspective = argv[0];
    return Show(argc-2,argv+2);
}


int Game::doResign(const char *w, int forfeit)
{
    char who[50];
    strcpy(who,w); // We may need to delete player[i] and thereby corrupt w.
    current_player = CurrentPlayer();
    if (Strcmp(players[current_player],who))
        current_player = players.Has(who);

    int first_move = (moves.Count() - players.Count() < FirstMove());

//printf("DEBUG: isdigit() && (moves.Count(%d) - players.Count(%d) < first_move(%d))\n",moves.Count(),players.Count(),FirstMove());

    /* If first move is resign, simply remove that player from the game */
    if (isdigit(*boardno) && first_move) {
        players.DeleteIndex(current_player);
        status.SetHasLeft(who);
        resigned.Init(players.Count());
        pending.Init(players.Count());
        if (players.Count() == 1) {
            /* Only one player left: remove game */
            char fn[256];
            sprintf(fn,"%s/games/%s/%s",getenv("PBMHOME"),GameType(),boardno);
            unlink(fn);
            UpdateActiveCount(0);  // normally called by Game::Write()
	    UpdateGameList(1);

            Mail mail(boardno);
            int i;
            for (i = 0; i < players.Count(); i++)
                if (!resigned[i])
                    mail.AddUser(players[i]);
            for (i = 0; i < subscribers.Count(); i++)
                mail.AddUser(subscribers[i]);
            mail.AddUser(who);
            FILE *fp = mail.fp();
            fprintf(fp,"Subject: %s board %s has been removed\n\n",GameType(),boardno);
			MIME_Header(fp);
            fprintf(fp,"%s has disagreed to play.\n\n", who);
            fprintf(fp,"There is only one player left and the game has therefore been removed.\n");
            comment.Print(fp);
        } else {
            /* One has left. Start game all over because board may have different setup*/
            InitClock(0);
            moves.Init();
            int reinit = Init();
            if (!SequentialMoves()) {
                hasmoved.Init(players.Count());
            }
            syncmoves.Init(players.Count());
			if (reinit && MinPlayers()<=players.Count()) {
                int i;
                for (i=0 ; i<players.Count() ; i++) {
                    if (resigned[i])
                        continue;
                    Mail mail(boardno);
                    mail.AddUser(player=players[i]);
                    PrintAs(mail.fp(), "Summary", AS_PLAYER, i);
                }
                for (i=0 ; i<subscribers.Count() ; i++) {
                    Mail mail(boardno);
                    mail.AddUser(player=subscribers[i]);
                    PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
                }
                Mail mail(boardno);
                mail.AddUser(player=who);
                Print(mail.fp(), "Summary"); // not deleted player's perspective
                if (Strcmp(getenv("SENDER"),"HTML"))
                {
                    Mail mail(boardno);
                    Print(mail.fp(), "Summary"); // not deleted player's perspective
                }
                Write();
		UpdateGameList(0);
            } else {
                /* attempt to start over failed; remove the board */
                char fn[256];
                sprintf(fn,"%s/games/%s/%s",getenv("PBMHOME"),GameType(),boardno);
                unlink(fn);
                UpdateActiveCount(0);  // normally called by Game::Write()
		UpdateGameList(1);

                Mail mail(boardno);
                int i;
                for (i = 0; i < players.Count(); i++)
                    if (!resigned[i])
                        mail.AddUser(players[i]);
                for (i = 0; i < subscribers.Count(); i++)
                    mail.AddUser(subscribers[i]);
                mail.AddUser(who);
                FILE *fp = mail.fp();
                fprintf(fp,"Subject: %s board %s has been removed\n\n",GameType(),boardno);
    			MIME_Header(fp);
                fprintf(fp,"%s has disagreed to play.\n\n", who);
                fprintf(fp,"The game has been removed because it could not be restarted with %d players.\n",players.Count());
                comment.Print(fp);
			}
		}
        return 1;
    }

    /* out-of-turn resignation goes pending */
    if (SequentialMoves() && Strcmp(who,players[CurrentPlayer()])) {
//printf("DEBUG: SequentialMoves() && who != current_player\n");
		current_player = players.Has(who);
        int change = 0;
        int resign = 0;
        int i;
        for (i = 0; i < players.Count(); i++)
            if (!Strcmp(players[i],who))
                if (!resigned[i]) {
                    change = 1, resign = (pending[i] = !pending[i]);
                    if (resign)
                        syncmoves.Change(i, "Resign");
                    else 
                        syncmoves.Change(i, "");
                }
        Mail mail(boardno);
        if (Strcmp(getenv("SENDER"),"HTML"))
        {
            mail.AddUser(player=who);
        }
        FILE *fp = mail.fp();
        fprintf(fp,"Subject: %s Board %s Resigned\n\n",GameType(),boardno);
        MIME_Header(fp);
        if (change && resign) {
            fprintf(fp,"Your resign request has been successfully queued.\n");
            fprintf(fp,"You will automatically resign on your next move.\n");
                        fprintf(fp,"If you send a new resign request, this resign will be cancelled.\n");
        }
        if (change && !resign) {
            fprintf(fp,"Your pending resign request has been cancelled.\n");
        }
        Write();
        return 1;
    }

    if (forfeit == 0)
        swapped.Init(1);

    resigned[current_player] = 1;
    if (SequentialMoves())
        pending[current_player] = 0;
    else
        hasmoved[current_player] = 1;

    if (forfeit)
        if (SequentialMoves())
            moves.Add("Forfeit");
        else
            syncmoves.Change(current_player, "Forfeit");
    else
        if (SequentialMoves())
            moves.Add("Resign");
        else
            syncmoves.Change(current_player, "Resign");

    previous_moves = moves.Count();

    const char *winner;
    if (!NextPlayer(winner))
        GameOver(winner);
    if (SequentialMoves() && pending[CurrentPlayer()])
        return doResign(players[CurrentPlayer()]);
    
    if (subscribers.Has(who) < 0)
        subscribers.Add(who);
//printf("DEBUG:  doResign - pre Write()\n");
    Write();
    UpdateGameList(0);
//printf("DEBUG:  doResign - post Write()\n");

    if (SequentialMoves() || !status.IsActive() || hasmoved.Has(1)<0) {
        int i;
        for (i=0 ; i<players.Count() ; i++) {
            if (resigned[i])
                continue;
            Mail mail(boardno);
            mail.AddUser(player=players[i]);
            PrintAs(mail.fp(), "Summary", AS_PLAYER, i);
        }
        for (i=0 ; i<subscribers.Count() ; i++) {
            Mail mail(boardno);
            mail.AddUser(player=subscribers[i]);
            PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
        }

        if (!Strcmp(getenv("SENDER"),"HTML"))
        {
            Mail mail(boardno);
            PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
        }
    }
    else {
	// only show everyone's comments when the round is complete
        comment.GetNew(who);
        Mail mail(boardno);
        mail.AddUser(player=who);
        Print(mail.fp(), "Summary");
        if (!Strcmp(getenv("SENDER"),"HTML"))
        {
            Mail mail(boardno);
            Print(mail.fp(), "Summary");
        }
    }
    return 1;
}

int Game::Resign(int argc, char **argv)
{
    if (argc != 3)
        return Error("Syntax is \"%s resign <board> <userid> <password>\"",GameType());

    if (!Read(argv[0]))
        return Error("%s board %s not found",GameType(),argv[0]);

    switch (CheckUserid(player=argv[1],argv[2])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

    print_board_on_error = 1;

    if (players.Has(argv[1]) < 0)
        return Error("You are NOT playing this game");

    if (!status.IsActive())
        return Error("The game has ended");

    if (!SequentialMoves() || !Strcmp(argv[1],players[CurrentPlayer()]))
        comment.GetNew(argv[1]);

    return doResign(argv[1]);
}

//  changes:
//   1. added check to allow # as the last character of move (as in chess)
//   2. changed Print to PrintAs(AS_PLAYER, i)
//   3. changed Print to PrintAs(AS_SUBSCRIBER, i)
//   4. added a comment to another Print
//  - akur
int Game::doMove(const char *move)
{
    char who[50];
    strcpy(who,players[current_player]);

    char *ptr = (char *) strrchr(move,'#');
    if (ptr != NULL && *(ptr + 1) != '\0') {
        int moveno;
        if (sscanf(ptr+1,"%d",&moveno) != 1)
            return Error("Invalid move number %s specified",ptr+1);
        if (SequentialMoves() && moveno != moves.Count()+1)
            return Error("Move %s received as move #%d",move,moves.Count()+1);
        if (!SequentialMoves() && moveno != moves.Count()/players.Count()+1)
            return Error("Move %s received as move #%d",move,moves.Count()/players.Count()+1);
        *ptr = '\0';
    }
    else
        ptr = NULL;

    const char *formatted_move;
    if (!Strcmp(move,"pass") || !Strcmp(move,SkipMove()))
        if (CanPass())
            formatted_move = SkipMove();
        else
            return Error("%s does not allow passing",GameType());
    else {
        if (player == NULL)
            player = players[current_player];
        formatted_move = MakeMove(move);
    }
    if (!formatted_move) return 0;

    if (SequentialMoves()) {
        moves.Add(formatted_move);
        comment.GetNew(who);
    }
    else {
		comment.GetNew(who,(hasmoved.Has(1)>=0));
        hasmoved[current_player] = 1;
        syncmoves.Change(current_player, formatted_move);
    }
    submitted.Init();
    swapped.Init(1);

    previous_moves = moves.Count();
    PrepareMove();

    const char *winner;
    if (!NextPlayer(winner))	// forced moves happen inside NextPlayer()
        GameOver(winner);
    else
        status.SetNormal();
    if (SequentialMoves() && pending[CurrentPlayer()])
        return doResign(players[CurrentPlayer()]);

    Write();
    UpdateGameList(0);

    if (SequentialMoves() || !status.IsActive() || hasmoved.Has(1)<0) {
        int i;
        for (i=0 ; i<players.Count() ; i++) {
            if (resigned[i])
                continue;
            Mail mail(boardno);
            mail.AddUser(player=players[i]);
            PrintAs(mail.fp(), "Summary", AS_PLAYER, i);
        }
        for (i=0 ; i<subscribers.Count() ; i++) {
            Mail mail(boardno);
            mail.AddUser(player=subscribers[i]);
            PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
        }
        if (!Strcmp(getenv("SENDER"),"HTML"))
        {
            Mail mail(boardno);
            PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
        }
    }
    else {
	// only show everyone's comments when the round is complete
        comment.GetNew(who);
        Mail mail(boardno);
        mail.AddUser(player=who);
        PrintAs(mail.fp(), "Summary", AS_PLAYER, players.Has(player));
        if (!Strcmp(getenv("SENDER"),"HTML"))
        {
            Mail mail(boardno);
            Print(mail.fp(), "Summary");
        }
    }
    return 1;
}

int Game::Move(char *movetype, int argc, char **argv)
{
    if (argc != 4)
        return Error("Syntax is \"%s %s <board> <userid> <password> <move>\"",GameType(),movetype);

    if (!Read(argv[0]))
        return Error("%s board %s not found",GameType(),argv[0]);

    switch (CheckUserid(player=argv[1],argv[2])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

    if (!status.IsActive())
        return Error("The game has ended");

    print_board_on_error = 1;

    // First check propose, accept, reject. Anybody can propose. Game then enters
    // "Propose" mode until one person rejects (or a move is made).

    if (players.Has(argv[1]) < 0)
        return Error("You are NOT playing this game");

    const char *move = argv[3];

    if (Strcmp(move,"propose")==0)
        return doProposeDraw(argv[1]);
    if (Strcmp(move,"undo")==0)
	return doProposeUndo(argv[1]);
    if (Strcmp(move,"accept")==0)
        return doAccept(argv[1]);
    if (Strcmp(move,"reject")==0)
        return doReject(argv[1]);
    if (isSpecialMove(move)) {
        move = doSpecialMove(move,argv[1]);
        if (!move)
            return 0;
    }

    current_player = 0;
    if (SequentialMoves())
        current_player = CurrentPlayer();
    else {
		int i;
		current_player = -1;
		for (i=0 ; i<players.Count() ; i++)
			if (!Strcmp(argv[1],players[i]))
				if (current_player == -1 || hasmoved[current_player]) 
					current_player = i;
    }

    if (Strcmp(move,"resign")==0) {
        if (SequentialMoves() && !Strcmp(argv[1],players[current_player])) {
			comment.GetNew(argv[1]);
		}
        else if (!SequentialMoves()) {
			comment.GetNew(argv[1],(hasmoved.Has(1)>=0));
        }
		return doResign(argv[1]);
    }

	int days,hr,min,sec;
	ConvertTime(timer[current_player],days,hr,min,sec);
    if (TotalTimeAllowed() && days >= TotalTimeAllowed()) {
        if (SequentialMoves() && !Strcmp(argv[1],players[current_player])) {
			comment.Init();
			comment.Add("You used too much time");
			comment.GetNew(argv[1],1);
		}
        else if (!SequentialMoves()) {
			// check whether anyone else's time is longer
			int death;
			comment.GetNew(argv[1],(hasmoved.Has(1)>=0));
			do {
				death = current_player;
				for (int i = 0 ; i<players.Count() ; i++ ) {
					if (!resigned[i] && timer[i]>timer[death]) {
						death = i;
					}
				}
				if (death != current_player) {
					Kill(2,death);
					if (!status.IsActive())
						return 1;
				}
			} while (death != current_player);

        }
		return doResign(argv[1],1);
    }

    if (!Strcmp(movetype,"move")) { // non queued move
        if (SequentialMoves() && Strcmp(argv[1],players[current_player]))
            return Error("It is not your move");
    }
    else if (SequentialMoves() && Strcmp(argv[1],players[current_player])) {
        int pl;
        for (pl=0 ; pl<players.Count() ; pl++) {
            if (!Strcmp(argv[1],players[pl]))
                break;
        }
        syncmoves.Change(pl,move);
        Write();
        if (!Strcmp(getenv("SENDER"),"HTML"))
        {
            Mail mail(boardno);
            Print(mail.fp(), "Summary ");
        }
        else
        {
            // notify the player that the move has been queued here...
            Mail mail(boardno);
            mail.AddUser(player=argv[1]);
            PrintAs(mail.fp(), "Summary", AS_PLAYER, players.Has(player));
        }

        return 1;
    }

    if (Strcmp(move,"swap")==0)
        return doSwap(argv[1]);
    return doMove(move);
}

int Game::Chat(int argc, char **argv)
{
    if (argc != 3)
        return Error("Syntax is \"%s chat <board> <userid> <password>\"",GameType());

    if (!Read(argv[0]))
        return Error("%s board %s not found",GameType(),argv[0]);

    switch (CheckUserid(player=argv[1],argv[2])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

    comment.GetNew(argv[1]);

    int i;
    for (i=0 ; i<players.Count() ; i++) {
        if (resigned[i])
            continue;
        Mail mail(boardno);
        mail.AddUser(player=players[i]);
        FILE *fp = mail.fp();
        fprintf(fp,"Subject: %s chat %s %s\n\n",GameType(),boardno,argv[1]);
		MIME_Header(fp);
        fprintf(fp,"Chat at %s Board %s\n\n",GameType(),boardno);
        comment.Print(fp);
    }
    for (i=0 ; i<subscribers.Count() ; i++) {
        Mail mail(boardno);
        mail.AddUser(player=subscribers[i]);
        FILE *fp = mail.fp();
        fprintf(fp,"Subject: %s chat %s %s\n\n",GameType(),boardno,argv[1]);
		MIME_Header(fp);
        fprintf(fp,"Chat at %s Board %s\n\n",GameType(),boardno);
        comment.Print(fp);
    }
    if (players.Has(argv[1]) < 0 && subscribers.Has(argv[1]) < 0) {
        Mail mail(boardno);
        mail.AddUser(player=argv[1]);
        FILE *fp = mail.fp();
        fprintf(fp,"Subject: %s chat %s %s\n\n",GameType(),boardno,argv[1]);
		MIME_Header(fp);
        fprintf(fp,"Chat at %s Board %s\n\n",GameType(),boardno);
        comment.Print(fp);
    }
    return 1;
}

//  changes:
//   1. added check to allow # as the last character of move (as in chess)
//   2. changed Print to PrintAs(AS_PLAYER, CurrentPlayer(-1))
//  - akur
int Game::Submit(int argc, char **argv)
{
    is_submitting = 1;
    if (argc != 4)
        return Error("Syntax is \"%s submit <board> <userid> <password> <move>\"",GameType());

    if (!CanPreview())
        return Error("%s does not support submission of moves",GameType());

    if (!Read(argv[0]))
        return Error("%s board %s not found",GameType(),argv[0]);

    switch (CheckUserid(player=argv[1],argv[2])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

    if (!status.IsActive())
        return Error("The game has ended");

    if (Strcmp(argv[1],players[CurrentPlayer()]))
        return Error("It is not your move");

    char *ptr = strrchr(argv[3],'#');
    if (ptr != NULL && *(ptr + 1) != '\0') {
        int moveno;
        if (sscanf(ptr+1,"%d",&moveno) != 1 || moveno != moves.Count()+1)
            return Error("Move %s received as move #%d",argv[3],moves.Count()+1);
        *ptr = '\0';
    }
    else
        ptr = NULL;

    submitted.Init().Add(argv[3]);

    /* It is important to write the board back before setting the game move e.t.c. */
    Write();

    const char *move;
    if (!Strcmp(argv[3],"pass") || !Strcmp(argv[3],SkipMove()))
        if (CanPass())
            move = SkipMove();
        else
            return Error("%s does not allow passing",GameType());
    else {
        player = players[CurrentPlayer()];
        move = MakeMove(argv[3]);
    }
    if (!move) return 0;

    /* Now make the board changes that are temporary: Adding move, setting winner */
    moves.Add(move);
    previous_moves = moves.Count();
    PrepareMove();
    
    const char *winner;
    if (!NextPlayer(winner))
        status.SetWinner(winner,GameValue(winner));
    else
        status.SetNormal();

    Mail mail(boardno);
    if (Strcmp(getenv("SENDER"),"HTML"))
    {
        mail.Add(getenv("SENDER"));
    }
    player=argv[1];
    PrintAs(mail.fp(), "Submit", AS_PLAYER, CurrentPlayer(-1));

    return 1;
}

int Game::Confirm(int argc, char **argv)
{
    if (argc != 3)
        return Error("Syntax is \"%s confirm <board> <userid> <password>\"",GameType());

    if (!CanPreview())
        return Error("%s does not support submission/confirmation of moves",GameType());

    if (!Read(argv[0]))
        return Error("%s board %s not found",GameType(),argv[0]);

    switch (CheckUserid(player=argv[1],argv[2])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

    if (submitted.Count() != 1)
        return Error("No submission to confirm");

    if (!status.IsActive())
        return Error("The game has ended");

    if (Strcmp(argv[1],players[CurrentPlayer()]))
        return Error("It is not your move");

    char move[1024];
    strcpy(move, submitted[0]);
    current_player = CurrentPlayer();
    return doMove(move);
}

//  changes:
//   1. changed Print to PrintAs(AS_PLAYER, i)
//   2. changed Print to PrintAs(AS_SUBSCRIBER, i)
//  - akur
int Game::doSwap(const char *who)
{
    if (!SwapAfterMove())
        return Error("%s does not support swapping",GameType());

    if (moves.Count() != SwapAfterMove())
        return Error("You can only swap after move number %d",SwapAfterMove());

    if (swapped[0])
        return Error("The board has just been swapped. Please make a move");

    status.SetSwapped(players[CurrentPlayer()]);
    swapped.Init().Add(1);

    players.Add(players[0]).DeleteIndex(0);
    resigned.Add(resigned[0]).DeleteIndex(0);
    timer.Add(timer[0]).DeleteIndex(0);

    comment.GetNew(who);

    Write();
    UpdateGameList(0);

    int i;
    for (i=0 ; i<players.Count() ; i++) {
        if (resigned[i])
            continue;
        Mail mail(boardno);
        mail.AddUser(player=players[i]);
        PrintAs(mail.fp(), "Summary", AS_PLAYER, i);
    }
    for (i=0 ; i<subscribers.Count() ; i++) {
        Mail mail(boardno);
        mail.AddUser(player=subscribers[i]);
        PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
    }
    if (!Strcmp(getenv("SENDER"),"HTML"))
    {
        Mail mail(boardno);
        PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
    }

    return 1;
}

int Game::doReject(const char *who)
{
	char voting;
	StringList accepters;
	if (status.GetProposed(accepters)) 
		voting = 'P';
	else if (status.GetUndo(accepters))
		voting = 'U';
	else
        	return Error("Nothing has been proposed. Please move or propose a draw or undo.");
	
    status.SetNormal();

    comment.GetNew(who);

    Write();
    UpdateGameList(0);

    int i;
    for (i=0 ; i<players.Count() ; i++) {
        if (resigned[i])
            continue;
        Mail mail(boardno);
        mail.AddUser(player=players[i]);
        PrintAs(mail.fp(), "Summary", AS_PLAYER, i);
    }
    for (i=0 ; i<subscribers.Count() ; i++) {
        Mail mail(boardno);
        mail.AddUser(player=subscribers[i]);
        PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
    }
    if (!Strcmp(getenv("SENDER"),"HTML"))
    {
        Mail mail(boardno);
        PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
    }

    return 1;
}

int Game::doAccept(const char *who)
{
	char voting;
	StringList accepters;
	if (status.GetProposed(accepters)) {
		voting = 'P';
		status.SetProposed(who);
		status.GetProposed(accepters);
	} else if (status.GetUndo(accepters)) {
		voting = 'U';
		status.SetUndo(who);
		status.GetUndo(accepters);
	} else
        	return Error("Nothing has been proposed. Please move or propose a draw or undo.");

	comment.GetNew(who);

	int i;
	int undecided = 0;
	for (i = 0; i < players.Count(); i++)
        	if (accepters.Has(players[i]) < 0)
			undecided++;
	// Check if all players have accepted
	if (undecided==0) {
		switch(voting) {
		case 'P':
			GameOver(NULL);
			break;
		case 'U':
			doUndo();
			status.SetNormal();
			break;
		}
	}

	Write();
	UpdateGameList(0);

	for (i=0 ; i<players.Count() ; i++) {
		if (resigned[i])
        		continue;
		Mail mail(boardno);
		mail.AddUser(player=players[i]);
		PrintAs(mail.fp(), "Summary", AS_PLAYER, i);
	}
	for (i=0 ; i<subscribers.Count() ; i++) {
		Mail mail(boardno);
		mail.AddUser(player=subscribers[i]);
		PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
	}
    if (!Strcmp(getenv("SENDER"),"HTML"))
    {
        Mail mail(boardno);
        PrintAs(mail.fp(), "Summary", AS_SUBSCRIBER, i);
    }

	return 1;
}

int Game::doProposeDraw(const char *who)
{
    if (!CanDraw())
        return Error("%s does not support proposed Draws",GameType());

    StringList accepters;
    if (status.GetProposed(accepters))
        return Error("A draw has already been proposed.\nPlease \'accept\' or \'reject\' it using\n\t\"move <board> <userid> <password> accept\"\nor\t\"move <board> <userid> <password> reject\"");

	status.SetProposed(who);
	for (int i=0 ; i<players.Count() ; i++)
		if (resigned[i])
			status.SetProposed(players[i]);
	doAccept(who);

    return 1;
}

int Game::doProposeUndo(const char *who)
{
	if (!CanUndo() || !SequentialMoves())
		return Error("%s does not support undoing moves",GameType());

	StringList undoers;
	if (status.GetUndo(undoers))
		return Error("An undo has already been requested.\nPlease \'accept\' or \'reject\' it using\n\t\"move <board> <userid> <password> accept\"\nor\t\"move <board> <userid> <password> reject\"");

	status.SetUndo(who);
	for (int i=0 ; i<players.Count() ; i++)
		if (resigned[i])
			status.SetUndo(players[i]);
	doAccept(who);
	return 1;
}

int Game::doUndo(int numdo)
{
	if (numdo == 0) {
		numdo = -1;
		if (!SequentialMoves())
			numdo = -(players.Count());
	}
	if (Regenerate(numdo)) {
		clock[1] = clock[2] = official_time;	// as if a move just happened
		comment.Add("\nPBeM Server writes:\nUNDO Successful!");
		return 1;
	} else {
		// Regerate failed: discard changes and apologize.
		Read(boardno);
		comment.Init().Add("PBeM Server writes:\nAttempt to undo move was unsuccessful.\nWe apologize for the inconvenience.");
		return 0;
	}
}

int Game::Swap(int argc, char **argv)
{
    if (argc != 3)
        return Error("Syntax is \"%s swap <board> <userid> <password>\"",GameType());

    if (!Read(argv[0]))
        return Error("%s board %s not found",GameType(),argv[0]);

    switch (CheckUserid(player=argv[1],argv[2])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

    if (!status.IsActive())
        return Error("The game has ended");

    if (Strcmp(argv[1],players[CurrentPlayer()]))
        return Error("It is not your move");

    return doSwap(argv[1]);
}

//  changes:
//   1. changed "int moveno;;" to "int moveno;"
//   2. added check to allow # as the last character of move (as in chess)
//   3. changed Print to PrintAs player (1, CurrentPlayer(-1))
//  - akur
int Game::Preview(int argc, char **argv)
{
    is_previewing = 1;

    if (argc < 0)
        return Error("Syntax is \"%s preview <board>|(new <numplayers>) <move> ... <move>\"",GameType());

    if (!CanPreview())
        return Error("%s does not support previewing of moves",GameType());

    if (Strcmp(*argv,"new") == 0) {
        argc--,argv++;
        if (argc < 0)
            return Error("Syntax is \"%s preview <board>|(new <numplayers>) <move> ... <move>\"",GameType());
        boardno = Strdup("Preview");
        int numplayers = atoi(*argv);
        if (numplayers < MinPlayers())
            return Error("At least %d players can play",MinPlayers());
        if (numplayers > MaxPlayers())
            return Error("At most %d players can play",MaxPlayers());
        argc--,argv++;
        int i;
	players.Init(numplayers);
        for (i = 0; i < numplayers; i++) {
            players.Change(i,PlayerNames(i));
            resigned.Add(0);
        }
        parameters.Init(parms);
        parms.Init();
        options.Init(opts);
        opts.Init();
        if (!Init())
            return 0;
        status.SetNormal();
        InitClock(0);
        const char *winner;
        if (!NextPlayer(winner))
            GameOver(winner);
    } else {
		char *ptr = strrchr(*argv,'#');
        int moveno;
        if (ptr != NULL && *(ptr + 1) != '\0') {
			if (sscanf(ptr+1,"%d",&moveno) != 1)
			return Error("Invalid move number %s specified",ptr+1);
			*ptr = '\0';
		}
        else
            ptr = NULL;

        if (!Read(*argv))
            return Error("Preview board not found");
		if (ptr) {
			// this moveno assumes a SequentialMoves() game
			if (!Regenerate(moveno)) 
			return Error("Couldn't regenerate board %s to move %d", *argv, moveno);
		}
        argc--,argv++;
    }

    while (argc > 0) {
        if (!status.IsActive())
            return Error("The game has ended");

        const char *move = *(argv++);  argc--;
        const char *formatted_move;
        if (!Strcmp(move,"pass") || !Strcmp(move,SkipMove()))
            if (CanPass())
                formatted_move = SkipMove();
            else
                return Error("%s does not allow passing",GameType());
        else {
            player = players[moves.Count()%players.Count()];
            formatted_move = MakeMove(move);
        }
        if (!formatted_move) return 0;
        moves.Add(formatted_move);

        PrepareMove();

        const char *winner;
        if (!NextPlayer(winner))
            status.SetWinner(winner,GameValue(winner));
        else
            status.SetNormal();
    }

    previous_moves = moves.Count();
    Mail mail(boardno);
    if (Strcmp(getenv("SENDER"),"HTML"))
    {
        mail.Add(getenv("SENDER"));
    }
    Print(mail.fp(), "Preview", 1);

    return 1;
}

int Game::doGames(char *who, FILE *fp)
{
    static int line = 0;
    if (who == NULL) {
        int temp = line;
        line = 0;
        return temp;
    }
    int i;
    line++;
    if (line==1) {
        fprintf(fp,"Board Move ");
        for (i = 0; i < MaxPlayers(); i++) {
            char p[100];
            sprintf(p,PlayerNames(i),i+1);
            if (MaxPlayers() > 4)
                fprintf(fp,"%-8s ",p);
            else
                fprintf(fp,"%-10s ",p);
        }
        fprintf(fp,"Board Status\n");
        fprintf(fp,"----- ---- ");
        for (i = 0; i < MaxPlayers(); i++)
            if (MaxPlayers() > 4)
                fprintf(fp,"-------- ");
            else
                fprintf(fp,"---------- ");
        fprintf(fp,"-------------------\n");
    }
    fprintf(fp,"%4s  %3d  ",boardno,SequentialMoves()?moves.Count():(moves.Count()/players.Count()));
    for (i = 0; i < MaxPlayers(); i++)
        if (MaxPlayers() > 4)
            fprintf(fp,"%-8s ",i>=players.Count()?"":players[i]);
        else
            fprintf(fp,"%-10s ",i>=players.Count()?"":players[i]);
    const char *winner;
    int points;
    if (status.GetWinner(winner,points)) {
        fprintf(fp,"%s Won",winner);
        if (points != 1)
            fprintf(fp," %d points", points);
        fprintf(fp,"\n");
    } else if (status.GetTie()) {
        fprintf(fp,"game was a tie\n");
    } else if (SequentialMoves()) {
        if (moves.Count() == 0)
            fprintf(fp,"%s to start\n",players[0]);
        else
            fprintf(fp,"%s's turn\n",players[CurrentPlayer()]);
    } else {
        fprintf(fp,"waiting for");
        int i;
        for (i=0 ; i<players.Count() ; i++) {
            if (hasmoved[i] == 0)
                fprintf(fp," %s",players[i]);
        }
        fprintf(fp,"\n");
    }
    return 1;
}

int Game::History(int argc, char **argv)
{
    char fn[1024];
    Mail mail;
    if (Strcmp(getenv("SENDER"),"HTML"))
    {
        mail.Add(getenv("SENDER"));
    }
    FILE *fp = mail.fp();
    FILE *hp ;

    sprintf(fn,"%s/history/%s",getenv("PBMHOME"),GameType());
    hp = fopen(fn,"r");

    fprintf(fp,"Subject: Historical List of %s games\n\n", GameType());
    fprintf(fp,"Historical List of %s games\n\n",GameType());

    while (hp && fgets(fn,sizeof(fn),hp)) {
        fputs(fn,fp);
    }
    fclose(hp);
    return 1;
}

int Game::Games(int argc, char **argv)
{
    StringList success, failure;

    doGames(NULL,NULL); // Initialize

    Mail mail;
    if (Strcmp(getenv("SENDER"),"HTML"))
    {
        mail.Add(getenv("SENDER"));
    }
    FILE *fp = mail.fp();
    fprintf(fp,"Subject: List of %s games\n\n", GameType());
	MIME_Header(fp);
    fprintf(fp,"List of %s games\n\n",GameType());
    int res = Scan(argc,argv,&Game::doGames,getenv("SENDER"),fp,success,failure);
    
    if (!doGames(NULL,NULL))
        fprintf(fp,"No %s boards found\n",GameType());

    if (failure.Count()) {
        int i;
        fprintf(fp,"\n\nThe following board%s not found:\n  ", failure.Count()!=1?"s were":" was");
        for (i = 0; i < failure.Count(); i++)
            fprintf(fp," %s",success[i]);
        if (!failure.Count())
            fprintf(fp," None");
        fprintf(fp,"\n");
    }
    return res;
}

int Game::Help(int argc, char **argv)
{
        (void)argc;
        (void)argv;
    
    char cmd[256];
    sprintf(cmd,"$PBMHOME/bin/help %s",GameType());
    for (char *c = cmd + 15 ; *c ; c++)
        *c = tolower(*c);
    system(cmd);
    return 1;
}

//  changes:
//   1. changed Print to PrintAs(AS_SUBSCRIBER, subscriber.Count() - 1)
//  - akur
int Game::doSubscribe(char *who, FILE*)
{
    int ix = players.Has(who);
    if (subscribers.Has(who) >= 0 || (ix >= 0 && !resigned[ix]) || !status.IsActive())
        return 0;
    subscribers.Add(who);
    Mail mail;
    mail.AddUser(player=who);
    PrintAs(mail.fp(), "Subscription Summary", AS_SUBSCRIBER, subscribers.Has(who), 1);
    Write();
    return 1;
}

int Game::Subscribe(int argc, char **argv)
{
    if (argc < 3)
        return Error("Syntax is \"%s subscribe <board> <userid> <password>\"", GameType());

    char *who;
    int all;
    StringList success, failure;
    if (!Read(argv[0]) && Strcmp(*argv, "all")) {
        // Perhaps alternate syntax
        if (CheckUserid(player=argv[0],argv[1]) != GOOD_USER)
            return Error("%s board %s not found",GameType(),argv[0]);
        if (!Scan(argc-2,argv+2,&Game::doSubscribe,argv[0],NULL,success,failure))
            return 0;
        who = argv[0];
        all = !Strcmp(argv[2],"all");
    } else {
        switch (CheckUserid(player=argv[1],argv[2])) {
	case BAD_PASSWORD: return Error("Invalid password entered");
        case BAD_USER: return Error("Invalid userid entered");
        }
        if (!Scan(1,argv,&Game::doSubscribe,argv[1],NULL,success,failure))
            return 0;
        who = argv[1];
        all = !Strcmp(argv[0],"all");
    }
    if(all){
      char cmd[256];
      sprintf(cmd,"grep -qs '^%s$' $PBMHOME/games/%s/.subscribeall",who,GameType());
      if(system(cmd)){
	char filename[256];
	FILE *fp;
	sprintf(filename,"%s/games/%s/.subscribeall",getenv("PBMHOME"),GameType());
	fp=fopen(filename,"a");
	fprintf(fp,"%s\n",who);
	fclose(fp);
      }
    }
    Mail mail;
    mail.AddUser(player=who);
    FILE *fp = mail.fp();
    fprintf(fp,"Subject: Subscription summary\n\n");
	MIME_Header(fp);
    fprintf(fp,"The following %s board%s successfully subscribed:\n  ", GameType(), success.Count()!=1?"s were":" was");
    int i;
    for (i = 0; i < success.Count(); i++)
        fprintf(fp," %s",success[i]);
    if (!success.Count())
        fprintf(fp," None");
    fprintf(fp,"\n\nThe following %s board%s not subscribed:\n  ", GameType(), failure.Count()!=1?"s were":" was");
    for (i = 0; i < failure.Count(); i++)
        fprintf(fp," %s",failure[i]);
    if (!failure.Count())
        fprintf(fp," None");
    fprintf(fp,"\n");
    return 1;
}

int Game::doUnsubscribe(char *who, FILE*)
{
    if (subscribers.Has(who) < 0)
        return 0;
    subscribers.Delete(who);
    Write();
    return 1;
}


int Game::Unsubscribe(int argc, char **argv)
{
    if (argc < 3)
        return Error("Syntax is \"%s unsubscribe <board> <userid> <password>\"", GameType());

    char *who;
    int all;
    StringList success, failure;
    if (!Read(argv[0]) && Strcmp(*argv, "all")) {
        // Perhaps alternate syntax
        if (CheckUserid(player=argv[0],argv[1]) != GOOD_USER)
            return Error("%s board %s not found",GameType(),argv[0]);
        if (!Scan(argc-2,argv+2,&Game::doUnsubscribe,argv[0],NULL,success,failure))
            return 0;
        who = argv[0];
        all = !Strcmp(argv[2],"all");
    } else {
        switch (CheckUserid(player=argv[1],argv[2])) {
        case BAD_PASSWORD: return Error("Invalid password entered");
        case BAD_USER: return Error("Invalid userid entered");
        }
        if (!Scan(1,argv,&Game::doUnsubscribe,argv[1],NULL,success,failure))
            return 0;
        who = argv[1];
        all = !Strcmp(argv[0],"all");
    }
    if(all){
      char cmd[256];
      char fn[256];
      sprintf(fn,"$PBMHOME/games/%s/.subscribeall",GameType());
      sprintf(cmd,"mv %s %s.%d 2> /dev/null",fn,fn,getpid());
      system(cmd);
      sprintf(cmd,"grep -vs '^%s$' %s.%d >%s",who,fn,getpid(),fn);
      system(cmd);
      sprintf(cmd,"rm -f %s.%d",fn,getpid());
      system(cmd);
    }
    Mail mail;
    mail.AddUser(player=who);
    FILE *fp = mail.fp();
    fprintf(fp,"Subject: Subscription summary\n\n");
	MIME_Header(fp);
    fprintf(fp,"The following %s board%s successfully unsubscribed:\n  ", GameType(), success.Count()!=1?"s were":" was");
    int i;
    for (i = 0; i < success.Count(); i++)
        fprintf(fp," %s",success[i]);
    if (!success.Count())
        fprintf(fp," None");
    fprintf(fp,"\n\nThe following %s board%s not unsubscribed:\n  ", GameType(), failure.Count()!=1?"s were":" was");
    for (i = 0; i < failure.Count(); i++)
        fprintf(fp," %s",failure[i]);
    if (!failure.Count())
        fprintf(fp," None");
    fprintf(fp,"\n");
    return 1;
}

int Game::Accept(int argc, char **argv)
{
    if (argc != 2)
        return Error("Syntax is \"%s accept <userid> <password>\"",GameType());

    switch (CheckUserid(player=argv[0],argv[1])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

    limits.ReadFile(GameType());

    if (limits.Limit(argv[0])==ACCEPTALL) {
        return Error("You are already accepting all %s challenges",GameType());
    } else {
        /* Accept */
	limits.UpdateLimit(argv[0],ACCEPTALL);
        Mail mail;
        mail.AddUser(player=argv[0]);
        FILE *fp = mail.fp();
        fprintf(fp, "Subject: %s challenges now accepted\n\n",GameType());
		MIME_Header(fp);
        fprintf(fp, "You are now accepting all %s challenges\n",GameType());
    }

    limits.WriteFile(GameType());

    return 1;
}

int Game::Reject(int argc, char **argv)
{
    if (argc != 2)
        return Error("Syntax is \"%s reject <userid> <password>\"",GameType());

    switch (CheckUserid(player=argv[0],argv[1])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

    limits.ReadFile(GameType());

    if (limits.Limit(argv[0])==REJECTALL) {
        return Error("You are already rejecting %s challenges",GameType());
    } else {
        /* Reject */
        limits.UpdateLimit(argv[0],REJECTALL);
        Mail mail;
        mail.AddUser(player=argv[0]);
        FILE *fp = mail.fp();
        fprintf(fp, "Subject: %s challenges now rejected\n\n",GameType());
		MIME_Header(fp);
        fprintf(fp, "You are now rejecting all %s challenges\n",GameType());
    }

    limits.WriteFile(GameType());

    return 1;
}

int Game::Limit(int argc, char **argv)
{
        switch (CheckUserid(player=argv[0],argv[1])) {
        case BAD_PASSWORD: return Error("Invalid password entered");
        case BAD_USER: return Error("Invalid userid entered");
        }

        limits.ReadFile(GameType());

        int oldlimit = limits.Limit(argv[0]);

        if (argc != 3) {
                Mail mail;
                mail.AddUser(player=argv[0]);
                FILE *fp = mail.fp();
                fprintf(fp, "Subject: %s limit status\n\n",GameType());
                MIME_Header(fp);
                fprintf(fp,"Syntax is \"%s limit <userid> <password> <limit>\"\n",GameType());
                fprintf(fp,"where <limit> is the maximum number of games to allow,\n");
                fprintf(fp,"or 'accept' to accept all games.\n\n\n");
                if (oldlimit == ACCEPTALL)
                        fprintf(fp, "Currently you are accepting all %s challenges\n\n",GameType());
                else if (oldlimit == REJECTALL)
                        fprintf(fp, "Currently you are rejecting all %s challenges\n\n",GameType());
                else
                        fprintf(fp, "Currently you are limited to %d active %s games\n\n",oldlimit,GameType());
                return 1;
        }

        int newlimit;
        newlimit = atoi(argv[2]);
        if (newlimit<=0) {
		if (!strcmp(argv[2],"0"))
			newlimit = REJECTALL;
                else if (!strncasecmp(argv[2],"reject",6))
                        newlimit = REJECTALL;
                else if (!strncasecmp(argv[2],"accept",6))
                        newlimit = ACCEPTALL;
                else if (!strncasecmp(argv[2],"all",3))
                        newlimit = ACCEPTALL;
                else if (!strncasecmp(argv[2],"inf",3))
                        newlimit = ACCEPTALL;
                else
                        return Error("Invalid Limit: \"%s\"",argv[2]);
        }

	limits.UpdateLimit(argv[0],newlimit);

        Mail mail;
        mail.AddUser(player=argv[0]);
        FILE *fp = mail.fp();
        fprintf(fp, "Subject: %s challenges now limited\n\n",GameType());
        MIME_Header(fp);
        if (newlimit == ACCEPTALL)
                fprintf(fp, "You are now accepting all %s challenges\n\n",GameType());
        else if (newlimit == REJECTALL)
                fprintf(fp, "You are now rejecting all %s challenges\n\n",GameType());
        else
                fprintf(fp, "You are now limited to %d active %s games\n\n",newlimit,GameType());
        if (oldlimit == ACCEPTALL)
                fprintf(fp, "Previously you were accepting all %s challenges\n\n",GameType());
        else if (oldlimit == REJECTALL)
                fprintf(fp, "Previously you were rejecting all %s challenges\n\n",GameType());
        else
                fprintf(fp, "Previously you were limited to %d active %s games\n\n",oldlimit,GameType());

        limits.WriteFile(GameType());

        return 1;
}

int Game::Seek(int argc, char **argv)
{
        switch (CheckUserid(player=argv[0],argv[1])) {
        case BAD_PASSWORD: return Error("Invalid password entered");
        case BAD_USER: return Error("Invalid userid entered");
        }

        limits.ReadFile(GameType());

        int oldseek = limits.Seek(argv[0]);

        if (argc != 3) {
                Mail mail;
                mail.AddUser(player=argv[0]);
                FILE *fp = mail.fp();
                fprintf(fp, "Subject: %s seek status\n\n",GameType());
                MIME_Header(fp);
                fprintf(fp,"Syntax is \"%s seek <userid> <password> <number>\"\n",GameType());
                fprintf(fp,"where <number> is the number of challenges you are seeking.\n");
                fprintf(fp,"The number will be shown on the 'standings' list.\n\n\n");
                fprintf(fp, "Currently you are seeking %d %s challenge%s.\n\n",oldseek,GameType(),(oldseek==1?"":"s"));
                return 1;
        }

        int newseek;
        newseek = atoi(argv[2]);
        if (newseek<0) {
		return Error("Invalid Number: \"%s\"",argv[2]);
        }

	limits.UpdateSeek(argv[0],newseek);

        Mail mail;
        mail.AddUser(player=argv[0]);
        FILE *fp = mail.fp();
        fprintf(fp, "Subject: Now seeking %d %s challenge%s\n\n",newseek,GameType(),(newseek==1?"":"s"));
        MIME_Header(fp);
        fprintf(fp, "You are now seeking %d %s challenge%s\n\n",newseek,GameType(),(newseek==1?"":"s"));

        limits.WriteFile(GameType());

        return 1;
}


int Game::doNotify(char *who, FILE*)
{
    int ix = players.Has(who);
    if (boardnotifiers.Has(who) >= 0 || subscribers.Has(who) >= 0 || (ix >= 0 && !resigned[ix]) || !status.IsActive())
        return 0;
    boardnotifiers.Add(who);
    Write();
    return 1;
}


int Game::Notify(int argc, char **argv)
{
	if (argc<2)
        return Error("Syntax is \"%s notify <userid> <password> [<board>...]\"",GameType());

    switch (CheckUserid(player=argv[0],argv[1])) {
    case BAD_PASSWORD: return Error("Invalid password entered");
    case BAD_USER: return Error("Invalid userid entered");
    }

	// Global Notify
	if (argc==2) {
        notifiers.ReadFile(GameType());

	    if (notifiers.Has(argv[0])>=0) {
	        /* Un-notify */
	        notifiers.Delete(argv[0]);
	        Mail mail;
	        mail.AddUser(player=argv[0]);
	        FILE *fp = mail.fp();
	        fprintf(fp, "Subject: %s notification cancelled\n\n",GameType());
			MIME_Header(fp);
	        fprintf(fp, "Notification of all %s games has been cancelled",GameType());
	    } else {
	        /* Notify */
	        notifiers.Add(argv[0]);
	        Mail mail;
	        mail.AddUser(player=argv[0]);
	        FILE *fp = mail.fp();
	        fprintf(fp, "Subject: %s notification started\n\n",GameType());
			MIME_Header(fp);
	        fprintf(fp, "Notification of all %s games has been started",GameType());
	    }

		notifiers.WriteFile(GameType());
		return 1;
	}

	// Specific Game(s) Notify
    StringList success, failure;
    if (!Scan(argc-2,argv+2,&Game::doNotify,argv[0],NULL,success,failure))
        return 0;
    Mail mail;
    mail.AddUser(player=argv[0]);
    FILE *fp = mail.fp();
    fprintf(fp,"Subject: %s Notify summary\n\n",GameType());
	MIME_Header(fp);
	fprintf(fp,"Notification of the following board%s was successful:\n  ",success.Count()!=1?"s":"");
    int i;
    for (i = 0; i < success.Count(); i++)
        fprintf(fp," %s",success[i]);
    if (!success.Count())
        fprintf(fp," None");
    fprintf(fp,"\n\nNotification of the following board%s failed:\n  ", failure.Count()!=1?"s":"");
    for (i = 0; i < failure.Count(); i++)
        fprintf(fp," %s",failure[i]);
    if (!failure.Count())
        fprintf(fp," None");
    fprintf(fp,"\n");
    return 1;
}


#if defined(WIN32) && !defined(unix)
int Game::Cleanup(int argc, char **argv)
{
    return 0;
}
#else

int Game::Kill(int forfeit,int who)
{
    char cmt[256];

    int age = BoardAge();
    int days,hr,min,sec;

    ConvertTime(age,days,hr,min,sec);

    comment.Init();

    switch (forfeit) {
    case 0: // player left the server
        comment.Add("Player deleted from server.");
        sprintf(cmt,"%s forfeits on %s board %s.", players[who], GameType(), boardno);
        comment.Add(cmt);
        break;
    case 1: // player did not move for 3 weeks (or more)
        sprintf(cmt, "Player has not moved in %d days.", days);
        comment.Add(cmt);
        sprintf(cmt,"%s forfeits on %s board %s.", players[who], GameType(), boardno);
        comment.Add(cmt);
        break;
    case 2: // player used too much time
        sprintf(cmt, "Player exceeded total time limit of %d days.", TotalTimeAllowed());
        comment.Add(cmt);
        sprintf(cmt,"%s forfeits on %s board %s.", players[who], GameType(), boardno);
        comment.Add(cmt);
        break;
	case 3: // player did not accept/reject draw within ForfeitTime()
		sprintf(cmt, "Player has not accepted or rejected the proposed draw in %d days.", days);
		comment.Add(cmt);
		sprintf(cmt, "%s auto-accepts on %s board %s.", players[who], GameType(), boardno);
		comment.Add(cmt);
		break;
    }

	int rc;
	if (forfeit == 3)
		rc = doAccept(players[who]);
	else
    	rc = doResign(players[who],1);

	return rc;
}

//  changes:
//   1. changed Print to PrintAs(AS_PLAYER, who)
//  - akur
int Game::Nag(int who)
{
	StringList accepters;
    const char *addr = UserAddress(players[who]);

    int age = BoardAge();
    int days,hr,min,sec;

    ConvertTime(age,days,hr,min,sec);

    printf("%s: %s is stale.  (%d vs %d)  %s's turn.\n",GameType(),boardno,days,ForfeitTime(),players[who]);
    if (!addr || !Strcmp(addr,"nobody")) {
		if (status.GetProposed(accepters) && accepters.Has(players[who]) < 0) {
        	Kill(3,who);
		} else {
        	Kill(0,who);
		}
    } else {
        Mail mail(boardno);
		player = players[who];
        mail.Add(addr);

        FILE *fp = mail.fp();
        fprintf(fp,"Subject: Nag: %s Board %s %s\n\n",GameType(),boardno,player);
		MIME_Header(fp);
		if (status.GetProposed(accepters) && accepters.Has(players[who]) < 0) {
			fprintf(fp,"A draw has been proposed on %s board %s.\n",GameType(),boardno);
			fprintf(fp,"You have not accepted or rejected the draw in %d days.\n",days);
			fprintf(fp,"If you do not accept or reject the draw before the board\n");
        	fprintf(fp,"ages to %d days, you will automatically accept the draw.\n",ForfeitTime());
		} else {
			fprintf(fp,"It is your move on %s board %s.  You have not moved in\n",GameType(),boardno);
			fprintf(fp,"%d days.  If no move is made before the board\n",days);
			fprintf(fp,"ages to %d days, you will forfeit the game.\n",ForfeitTime());
		}
        fprintf(fp,"\n");
        fprintf(fp,"You will find the board attached at the end of this letter.\n");
        fprintf(fp,"\n");
        fprintf(fp,"Richard\n");
        fprintf(fp,"\n");
        PrintAs(fp,"Nag", AS_PLAYER, who);
    }

    return 0;
}

int Game::TotalTimeAllowed(void)
{
	if (totaltimelimit.Count() > 0 && totaltimelimit[0] > 0)
		return totaltimelimit[0];
	return 0;
}

int Game::NagTime(void)
{
	if (nagtimelimit.Count() > 0 && nagtimelimit[0] > 0)
		return nagtimelimit[0];
	if (!isdigit(boardno[0]) && isdigit(boardno[strlen(boardno)-1]))
		return 1;
	return 7;
}

int Game::ForfeitTime(void)
{
	if (forfeittimelimit.Count() > 0 && forfeittimelimit[0] > 0)
		return forfeittimelimit[0];
	if (!isdigit(boardno[0]) && isdigit(boardno[strlen(boardno)-1]))
		return 7;
	return 21;
}

int Game::Cleanup(int argc, char **argv)
{
// Suppressing a compiler warning...
    //(void)argc;
    //(void)argv;
        
    int daily=0;
    int nagonly=0;

    if (!Strcmp(argv[0],"daily"))
        daily=1;
    if (!Strcmp(argv[0],"nag") ||
        !Strcmp(argv[0],"justnag") ||
        !Strcmp(argv[0],"nagonly"))
        nagonly=1;

    char cmd[1024];

    /* cleanup .* entries */
    printf("%s: %scleanup\n",GameType(),(daily?"daily ":""));
    sprintf(cmd, "find %s/games/%s \\( -name '.[1-9][0-9]*' -o -name '.[a-z][1-9][0-9]*' -o -name '.[1-9][0-9]*[a-z]' -o -name '.[a-z][1-9][0-9]*[a-z]' \\) -exec rm -f {} \\;", getenv("PBMHOME"), GameType());
    system(cmd);

    /* first Check for "Dead" boards... Active.  BoardAge > ForfeitTime() */
    /* now Check for "Old" boards... Not Active.  BoardAge > 14 days */
    /* now Check for "Stale" boards... Active.  BoardAge > NagTime() */
    sprintf(cmd, "ls %s/games/%s | grep -v '~$'", getenv("PBMHOME"), GameType());
    FILE *dirp = popen(cmd, "r");
    if (!dirp) {
            fprintf(stderr,"Unable to: %s\n",cmd);
            exit (0);
    }
    char dp[256];
    while (fgets(dp,sizeof(dp),dirp)) {
        char *cr = strchr(dp,'\n');
        if (cr)    *cr = '\0';


        if (daily && (isdigit(dp[0]) || !isdigit(dp[strlen(dp)-1])) && (ForfeitTime() != 21 || NagTime() != 7))
            continue;

        if (!Read(dp))    continue;

        printf("DEBUG: scanning %s\n",dp);

        int age = BoardAge();
        int days,hr,min,sec;

		// handle TotalTimeAllowed infractions
		// continue checking if no penalties
        ConvertTime(timer[CurrentPlayer()],days,hr,min,sec);
		if (status.IsActive() && TotalTimeAllowed()) {
			if (SequentialMoves() && days >= TotalTimeAllowed()) {
				char *whosturn = players[CurrentPlayer()];
				printf("DEBUG: kill(2,%d) %s: %s: %s used too much time (%d vs %d).\n",CurrentPlayer(),GameType(),dp,whosturn,days,TotalTimeAllowed());
				Kill(2,CurrentPlayer());
				continue;
			}
			if (!SequentialMoves()) {	// check each player...
				int death;
				do {
					death=-1;
					for(int i=0; i<players.Count() ; i++) {
						if (resigned[i]) continue;
						ConvertTime(timer[i],days,hr,min,sec);
						if (days >= TotalTimeAllowed()) {
							// make sure we kill the longest time first
							if (death<0 || timer[i]>timer[death]) {
								death = i;
							}
						}
					}
					if (death>=0) {
						printf("%s: %s: %s used too much time (%d vs %d).\n",GameType(),dp,players[death],days,TotalTimeAllowed());
						Kill(2,death);
					}
				} while (death>=0 && status.IsActive());
			}
        }

		StringList accepters;
		ConvertTime(age,days,hr,min,sec);
	
		// check for stalling during draw proposal
		// undos will be nagged normally
		if (status.GetProposed(accepters)) {
// premise: if a draw has been proposed, it should auto-accept at forfeit-time.
// conclusion: we should nag all stallers, instead of CurrentPlayer()
			for (int i=0 ; i<players.Count() ; i++) {
				// check each player that has not voted...
				char cmd[256];
				char *whosturn = players[i];
				int onvacation=0, backfromvacation=0;
	
				if (accepters.Has(players[i]) >= 0) continue;
	
				sprintf(cmd,"grep -q '^%s ' $PBMHOME/etc/vacation",whosturn);
				onvacation = (system(cmd) == 0);
                if (!onvacation) {
                    sprintf(cmd,"grep -q '^#%s ' $PBMHOME/etc/vacation",whosturn);
                    backfromvacation = (system(cmd) == 0);
                }
				if (!onvacation && !backfromvacation && days >= ForfeitTime()) {
					printf("DEBUG: Kill(3,%d) %s: %s is dead.  (%d vs %d)  %s's turn.  \n",i,GameType(),dp,days,ForfeitTime(),whosturn);
                    //scanf("%s",yn)
					if (daily == 0 && !nagonly) {
                        printf("DEBUG: Kill it\n");
						Kill(3,i);	// 3 == auto-accept draw
                    }
					else {
                        printf("DEBUG: Nag em\n");
						Nag(i);
                    }
				}
				else if (!onvacation && days >= NagTime()) {
                    printf("DEBUG: Nag em\n");
					Nag(i);
				}
			}
			continue;
		}
		
		// cleanup old inactive games
		if (!status.IsActive()) {
			if (days > 14) {
				char fn[1024];
				sprintf(fn,"%s/games/%s/%s", getenv("PBMHOME"), GameType(), dp);
				printf("%s: %s: Game Over",GameType(),dp);
				if (dp[0] >= 'a' && dp[0] <= 'z') {
					char nfn[1024];
					sprintf(nfn,"%s/games/%s-tourney/%s", getenv("PBMHOME"), GameType(), dp);
					rename(fn,nfn);
					printf("  Saved to %s",nfn);
				}
				else {
					char nfn[1024];
					sprintf(nfn,"%s/games/%s-archive/%s", getenv("PBMHOME"), GameType(), dp);
					rename(fn,nfn);
					printf("  archived");
				}
				printf("\n");
				UpdateGameList(1);
			}
			continue;
		}

		// sequential move nags
		if (SequentialMoves()) {
			char cmd[256];
			char *whosturn = players[CurrentPlayer()];
			int onvacation = 0;
			int backfromvacation = 0;

			sprintf(cmd,"grep -q '^%s ' %s/etc/vacation",whosturn,getenv("PBMHOME"));
			onvacation = (system(cmd) == 0);
			if (!onvacation) {
                sprintf(cmd,"grep -q '^#%s ' %s/etc/vacation",whosturn,getenv("PBMHOME"));
                backfromvacation = (system(cmd) == 0);
            }
            //printf("onvacation %d  backfrom %d  days %d\n",onvacation,backfromvacation,days);
            //fflush(stdout);
			//scanf("%s",yn);
			if (!onvacation && !backfromvacation && days >= ForfeitTime()) {
				printf("DEBUG: X Kill(1,%d) %s: %s is dead.  (%d vs %d)  %s's turn.  \n",CurrentPlayer(),GameType(),dp,days,ForfeitTime(),whosturn);
				//scanf("%s",yn);
				if (daily == 0 && !nagonly) {
                    printf("DEBUG: Kill it\n");
					Kill(1,CurrentPlayer());
                }
				else {
                    printf("DEBUG: Nag em\n");
					Nag(CurrentPlayer());
                }
			}
			else if (!onvacation && days >= NagTime()) {
                printf("DEBUG: Nag em\n");
				Nag(CurrentPlayer());
			}
			continue;
		}

		// simultaneous move nags
		if (!SequentialMoves()) {
			for (int i=0 ; i<players.Count() ; i++) {	// check each player that has not moved...
				char cmd[256];
				char *whosturn = players[i];
				int onvacation;
	
				if (hasmoved[i] || resigned[i])	continue;
	
				fprintf(stderr,"%s: grep '^%s ' %s/etc/vacation >/dev/null 2>&1\n",dp,whosturn,getenv("PBMHOME"));
				sprintf(cmd,"grep '^%s ' %s/etc/vacation >/dev/null 2>&1\n",whosturn,getenv("PBMHOME"));
				onvacation = (system(cmd) == 0);
				if (!onvacation && days >= ForfeitTime()) {
					printf("DEBUG: Y Kill(1,%d) %s: %s is dead.  (%d vs %d)  %s's turn.  \n",i,GameType(),dp,days,ForfeitTime(),whosturn);
					//scanf("%s",yn);
					if (daily == 0)
						Kill(1,i);
					else
						Nag(i);
				}
				else if (!onvacation && days >= NagTime()) {
					Nag(i);
				}
			}
			continue;
		}

    }
    pclose(dirp);
        return 0;
}
#endif

//  changes:
//   1. changed Print to PrintAs(AS_PLAYER, players.Has(player))
//  - akur
int Game::Error(const char *msg, ...)
{
    va_list marker;
    va_start(marker,msg);

    Mail mail(boardno);
	//if (is_previewing || player == NULL)
        if (Strcmp(getenv("SENDER"),"HTML"))
        {
            mail.Add(getenv("SENDER"));
        }
	//else if (player != NULL) {
        //mail.AddUser(player);
    //}
    FILE *fp = mail.fp();
    if (print_board_on_error) {
        Read(boardno);
        fprintf(fp,"Subject: %s Board %s Error\n\n",GameType(),boardno);
		MIME_Header(fp);
        vfprintf(fp,msg,marker);
        fprintf(fp,"\n\n");
        if (player == NULL || players.Has(player) < 0)
            Print(fp,"Error",1);
        else
            PrintAs(fp,"Error",AS_PLAYER, players.Has(player), 1);
    } else {
		fprintf(fp,"Subject: %s error\n\n",GameType());
		MIME_Header(fp);
        vfprintf(fp,msg,marker);
    }
    fprintf(fp,"\n\nOriginal message follows:\n");

    FILE *oldmailfp = f_open(getenv("INPUTFILE"), "r");
    if (oldmailfp) {
        char line[256];
        while (fgets(line,sizeof(line),oldmailfp))
            fprintf(fp,">%s",line);
        f_close(oldmailfp);
    }

    va_end(marker);
    return 0;
}


int Game::DecodeDigits(const char *s, int &value)
{
    value = 0;
    int ix = 0;
    while (isdigit(s[ix])) {
        value *= 10;
        value += s[ix++]-'0';
    }
    return ix;
}

const char *Game::EncodeDigits(int value)
{
    static char s[20];
    sprintf(s,"%d",value);
    return s;
}

int Game::DecodeLetters(const char *s, int &value)
{
    value = -1;
    int ix = 0;
	if (*s == '@') {
		value=0;
		ix++;
		return ix;
	}
    while (isalpha(s[ix])) {
        value = (value+1) * ('z'-'a'+1);
        value += tolower(s[ix])-'a';
        ix++;
    }
    value++;
    return ix;
}

const char *Game::EncodeLetters(int value, int uppercase)
{
    static char s[20];
    int ix = 19;
    s[ix--] = '\0';
	if (value ==  0) {
        s[ix--] = '@';
	}
    while (value>0) {
        s[ix--] = (value-1) % ('z'-'a'+1) + (uppercase ? 'A' : 'a');
        value = (value-1) / ('z'-'a'+1);
    }
    return s+ix+1;
}

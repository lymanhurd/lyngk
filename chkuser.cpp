#include <stdio.h>
#include "file.h"
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#define strncasecmp strnicmp
#define strcasecmp stricmp
#else
#include <unistd.h>
#endif

#include "chkuser.h"

static FILE *users=NULL;

UserStatus CheckUserid(const char *userid, const char *password)
{
	static char line[1024];
	char goodpw[256];
	char badpw[256];
	char fn[256];
	char *s;

#ifdef _WIN32
    return GOOD_USER;
#endif

	sprintf(goodpw,"%s:%s:",userid,password);
	sprintf(badpw,"%s:",userid);

	sprintf(fn,"%s/etc/users",getenv("PBMHOME"));
	if (!users)
		users = f_open(fn,"r");
	else
		rewind(users);

	while (fgets(line,sizeof(line),users)) {
		s = strchr(line,'\n');                
		if (s)
			*s = '\0';
		if (!strncasecmp(line,goodpw,strlen(goodpw))) {
			return GOOD_USER;
		}
		if (!strncasecmp(line,badpw,strlen(badpw))) {
#ifdef PASSWORD
			if (!strcasecmp(password,PASSWORD))
				return GOOD_USER;
			else
#endif
				return BAD_PASSWORD;
		}
			
	}
	return BAD_USER;
}

int DigestStatus(const char *userid)
{
	static char line[1024];
	char fn[256];
	char cmp[256];
	char *s;

	sprintf(fn,"%s/etc/users",getenv("PBMHOME"));
	if (!users)
		users = f_open(fn,"r");
	else
		rewind(users);

	sprintf(cmp,"%s:",userid);
	while (fgets(line,sizeof(line),users)) {
		s = strchr(line,'\n');                
		if (s)
			*s = '\0';
		if (!strncasecmp(line,cmp,strlen(cmp))) {
			s = strrchr(line,':');
			return (*--s)-'0';
		}
	}

	return 2;
}

const char *UserAddress(const char *userid)
{
	static char line[1024];
	char fn[256];
	char cmp[256];
	char *s;

	sprintf(fn,"%s/etc/users",getenv("PBMHOME"));
	if (!users)
		users = f_open(fn,"r");
	else
		rewind(users);

	sprintf(cmp,"%s:",userid);
	while (fgets(line,sizeof(line),users)) {
		s = strchr(line,'\n');                
		if (s)
			*s = '\0';
		if (!strncasecmp(line,cmp,strlen(cmp))) {
			s = strrchr(line,':');
			return s+1;
		}
	}

	sprintf(line,"nobody");
	return line;
}

const char * GetUserOptions(const char *game,const char *userid)  /* This routine reads a file which     */
{                                    /* contains the size preference info   */
	static char line[1024];      /* for each user on a separate line, in*/
	char fn[256];                /* the form userid:size with no spaces.*/
	char cmp[256];               /* size is either "small", "medium",   */
	char *s;                     /* or "big".                           */

	sprintf(fn,"%s/etc/%s.options",getenv("PBMHOME"),game);
	if (!users) {
		users = f_open(fn,"r");
		if (!users) return NULL;
	}
        else
		rewind(users);

	sprintf(cmp,"%s:",userid);
	while (fgets(line,sizeof(line),users)) {
		s = strchr(line,'\n');
		if (s)
			*s = '\0';
		if (!strncasecmp(line,cmp,strlen(cmp))) {
			s = strrchr(line,':');
                        s++;
			return s;
		}
	}

	return NULL;
}

void SetUserOptions(const char *game,const char *userid, const char *option)
{
    static char line[1024];   /*This routine creates a temp file, the    */
    char fn[256], tmpn[256];  /*first line of which tells the user's new */
    char cmp[256];            /*size, followed by every line in the size */
    char *s;                  /*file which DOESN'T refer to this user,   */
    FILE *rp,*wp;             /*then temp replaces the size file.        */

    sprintf(fn,"%s/etc/%s.options",getenv("PBMHOME"),game);
    sprintf(tmpn,"%s/etc/tmp",getenv("PBMHOME"));
    rp = f_open(fn,"r");
    wp = f_open(tmpn,"w");

    fprintf(wp,"%s:%s\n",userid,option);

    if (rp) {
        sprintf(cmp,"%s:",userid);
        while (fgets(line,sizeof(line),rp)) {
            s = strchr(line,'\n');
            if (s)
                *s = '\0';
            if (strncasecmp(line,cmp,strlen(cmp))) {	/*NOTE: If the comparison*/
                fprintf(wp,"%s\n",line);	/*FAILS, copy this line  */
            }
        }					/*into twixtmp. Else,    */
    }						/*skip this line.        */

    f_close(rp);
    f_close(wp);
    remove(fn);
    rename(tmpn,fn);
}

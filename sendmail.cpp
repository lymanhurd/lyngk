#if defined(WIN32) && !defined(unix)
#define popen(str,mode) (printf("%s\n",str),*(mode)=='w'?stdout:stdin)
#define pclose(fp)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(WIN32) && !defined(unix)
#define popen(str,mode) (printf("%s\n",str),*(mode)=='w'?stdout:stdin)
#define pclose(fp)
#define lockf(fp,mode,x)
#include <process.h>
#define strcasecmp(x,y) stricmp(x,y)
#define strncasecmp(x,y,z) strnicmp(x,y,z)
#define sleep(x)
#else
#include <unistd.h>
#endif
#include <time.h>

#include "file.h"
#include "sendmail.h"


static time_t localtimer = time(NULL);

Mail::Mail(char *bn, const int ad)
{
	_fp = NULL;
    append_ad = ad;
    boardno = bn;
}

Mail::~Mail()
{
	if (_fp) {
		MIME_Footer(_fp,append_ad);
		pclose(_fp);
	}
}

FILE *Mail::fp(char *from)
{
	if (_fp)
		return _fp;

    int i;
	int count=0;

	char mailcmd[255] = "";
	char *sendmail = getenv("PBMSENDMAIL");
	if (sendmail && strlen(sendmail) > 0) {
		strcpy(mailcmd,sendmail);
	} else {
		strcpy(mailcmd,"/home/PBM/pbmserv/bin/sendmail -t");
		//strcpy(mailcmd,"/usr/sbin/sendmail -t");
	}
	
    if (Count() != 0) 
    {
            while (count < 60 && (_fp = popen(mailcmd,"w")) == NULL) {
            count++;
            sleep(5);
        }
        if (count >= 30) {
            return NULL;
        }
    }
    else {
        _fp = stdout;
        return _fp;
    }

	if (!from)
		fprintf(_fp,"From: %s\n",SERVERADDR);
	else
		fprintf(_fp,"From: %s\n",from);

	fprintf(_fp,"To:");
	for (i=0; i<Count(); i++) {
		fprintf(_fp," %s",(*this)[i]);
		if (i < Count()-1)
			fprintf(_fp,",");
	}
	fprintf(_fp,"\n");

	char *type   = getenv("CONTENTTYPE");
	char *boundary = getenv("MIME_BOUNDARY");
	//char *mime   = getenv("MIMEVERSION");
	//char *how    = getenv("CONTENTDISPOSITION");
	//char *encode = getenv("CONTENTTRANSFERENCODING");

	if (type)	{
		fprintf(_fp,"MIME-Version: 1.0\n");
		if (!strcasecmp(type,"text/plain") || 
		    !strncasecmp(type,"text/plain;",11))
			fprintf(_fp,"Content-Type: %s\n",type);
		else
			fprintf(_fp,"Content-Type: multipart/mixed; boundary=\"%s\"\n",boundary);
	}
	return _fp;
}

void MIME_Header(FILE *fp)
{
	char *type   = getenv("CONTENTTYPE");

	if (!type)	return;
	if (!strcasecmp(type,"text/plain") || 
		!strncasecmp(type,"text/plain;",11))
		return;

	char *boundary = getenv("MIME_BOUNDARY");

	if (!boundary)	return;
	
	fprintf(fp,"--%s\n",boundary);
	fprintf(fp,"Content-Type: text/plain\n");
	fprintf(fp,"Content-Disposition: inline\n");
	fprintf(fp,"\n");
}

void MIME_Middle(FILE *fp)
{
	char *type   = getenv("CONTENTTYPE");

	if (!type)	return;
	if (!strcasecmp(type,"text/plain") || 
		!strncasecmp(type,"text/plain;",11))
		return;

	char *boundary = getenv("MIME_BOUNDARY");

	if (!boundary)	return;
	
	fprintf(fp,"--%s\n",boundary);

	fprintf(fp,"Content-Type: %s\n",type);
	fprintf(fp,"Content-Disposition: inline\n\n");
}

void MIME_Footer(FILE *fp,int append_ad)
{
	char *type   = getenv("CONTENTTYPE");
	char *boundary = getenv("MIME_BOUNDARY");
    char advfile[256];
    char line[256];
    FILE *advfp;

	if (!type || 
        !strcasecmp(type,"text/plain") || 
		!strncasecmp(type,"text/plain;",11) ||
        !boundary) {
        sprintf(advfile,"%s/advertisement",getenv("PBMHOME"));
        advfp = f_open(advfile,"r");
        while (append_ad && advfp && fgets(line,sizeof(line),advfp))
            fputs(line,fp);
	if (advfp) f_close(advfp);
	return;
    }

    sprintf(advfile,"%s/advertisement",getenv("PBMHOME"));
    advfp = f_open(advfile,"r");
    if (append_ad && advfp) {
        fprintf(fp,"--%s\n",boundary);
        fprintf(fp,"Content-Type: text/plain\n");
        fprintf(fp,"Content-Disposition: inline\n\n");
    }
    while (append_ad && advfp && fgets(line,sizeof(line),advfp))
        fputs(line,fp);
    if (advfp) f_close(advfp);
    fprintf(fp,"--%s--\n",boundary);
}

Mail &Mail::Add(const char *address)
{
	if (Has(address) < 0)
		StringList::Add(address);
	return *this;
}

Mail &Mail::AddList(StringList &sl)
{
	int i;
	for (i=0; i < sl.Count(); i++)
		Add(sl[i]);
	return *this;
}

Mail &Mail::AddUser(const char *userid)
{
	Add(UserAddress(userid));
	return *this;
}

Mail &Mail::AddUserList(StringList &sl)
{
	int i;
	for (i=0; i < sl.Count(); i++)
		AddUser(sl[i]);
	return *this;
}

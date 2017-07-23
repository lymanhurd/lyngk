#ifndef SENDMAIL_H
#define SENDMAIL_H

#include "lists.h"
#include "chkuser.h"

#define SERVERADDR "Richard's PBeM Server <pbmserv@gamerz.net>"

void MIME_Header(FILE *fp);
void MIME_Middle(FILE *fp);
void MIME_Footer(FILE *fp,int append_ad);

class Mail:public StringList {
protected:
	FILE *_fp;
    int append_ad;
    char *boardno;
public:
	Mail(char *boardno=NULL,const int ad=1);
	~Mail();
	FILE *fp(char *from=NULL);
	Mail &Add(const char *address);
	Mail &AddList(StringList &sl);
	Mail &AddUser(const char *name);
	Mail &AddUserList(StringList &sl);
};

#endif


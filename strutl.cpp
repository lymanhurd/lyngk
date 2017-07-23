#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "strutl.h"

int Strcmp(const char *s1, const char *s2)
{
	if (!s1 && !s2)
		return 0;
	if (!s1)
		return -1;
	if (!s2)
		return 1;
	return strcmp(s1,s2);
}

char *strapp(char *s1, const char *s2)
{
	if (!s2 || !*s2)
		return s1;
	if (!s1)
		return strdup(s2);
	s1 = (char *) realloc(s1,strlen(s1)+strlen(s2)+1);
	strcat(s1,s2);
	return s1;
}

int abbrev(const char *s1, const char *s2, unsigned int len)
{
    const char *eq = strchr(s1,'=');
    int cl=len;
	if (!s1)	s1="";
	if (!s2)	s2="";
    if (eq)
    {
        if ((unsigned int)(eq-s1) > len)
            cl = eq-s1;
    }
    else
    {
        if (strlen(s1)>len)
            cl=strlen(s1);
    }
	return (!strncmp(s1,s2,cl));
}

int contains(const char *str, const char *ch)
{
	if (!str || !ch)
		return 0;

	while (*ch) {
		if (strchr(str,*ch))
			return 1;
		ch++;
	}

	return 0;
}

char *Strdup(const char *s)
{
	if (!s)
		return NULL;
	return strdup(s);
}

char *trim(char *s)
{
	if (s) {
		char *t=s+strlen(s)-1;

		while (t >= s && strchr(" \n\t\r",*t))
			*t-- = '\0';
	}
	return s;
}

char *strtolower(char *s)
{
	for (char *t = s ; t && *t ; t++)
		*t = tolower(*t);
	return s;
}

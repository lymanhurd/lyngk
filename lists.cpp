#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lists.h"
#include "gamefile.h"
#include "strutl.h"

StringList::StringList(void)
{
    count = 0;
    str = NULL;
}

StringList::~StringList(void)
{
    Init();
}

StringList &StringList::Init(int n)
{
    int i;

	for (i=0 ; i<count ; i++)
		if (str[i])	free(str[i]);
    delete [] str;

    count = 0;
    str = NULL;

    for (i=0 ; i<n ; i++)
    Add("");
    return *this;
}

char * StringList::operator[](int ix)
{
    static char * zero;
    zero = NULL;
    return ix >= 0 && ix < count ? str[ix] : zero;
}

StringList &StringList::Prepend(const char * s)
{
    String *newstr = new String[count+1];

    if (newstr) {
        if (count) memcpy(newstr+1,str,count*sizeof(String));
		delete [] str;
        str = newstr;
        str[0] = Strdup(s);
        count++;
    }
    return *this;
}

StringList &StringList::Append(const char * s)
{
	if (s) {
		String *newstr = new String[count+1];

		if (newstr) {
			if (count) memcpy(newstr,str,count*sizeof(String));
			delete [] str;
			str = newstr;
			str[count++] = Strdup(s);
		}
	}
    return *this;
}

int StringList::Has(const char * s, int start)
{
    int ix;
    for (ix = start; ix < count; ix++)
        if (!strcmp(str[ix], s))
            return ix;
    return -1;
}

void StringList::Delete(const char * s)
{
    DeleteIndex(Has(s));
}

void StringList::DeleteIndex(int ix)
{
    if (ix < 0 || ix >= count)
        return;
    free(str[ix]);
	str[ix] = NULL;
    int i;
    for (i = ix+1; i < count; i++)
        str[i-1] = str[i];
    if (--count == 0) {
        delete [] str;
        str = NULL;
    }
}

void StringList::Change(int ix, const char * s)
{
    if (ix < 0 || ix >= count)
        return;
	if (str[ix]) free(str[ix]);
    str[ix] = Strdup(s);
}



int ReadWriteStringList::Read(GameFile &game)
{
    if (!game.GetEntry(Name()))
        return 0;
    Init();
    const char * token;
    while ((token = game.GetToken()))
        Add(token);
    return 1;	
}

int ReadWriteStringList::Write(GameFile &game)
{
    if (!count)
        return 1;
    game.PutEntry(Name(),OneLiner());
    int i;
    for (i=0; i < count; i++)
        game.PutToken(str[i]);
    return 1;
}


int SingleFileStringList::ReadFile(const char * gameType)
{
    GameFile game;
    game.Read(gameType,FileName());
    return Read(game);	// Will automatically do game.Close()
}

int SingleFileStringList::WriteFile(const char * gameType)
{
    GameFile game;
    game.Write(gameType,FileName());
    return Write(game);	// Will automatically do game.Close()
}



IntList::IntList(void)
{
    count = 0;
    ints = NULL;
}

IntList::IntList(IntList &l)
{
    count = 0;
    ints = NULL;
    (*this) = l;
}

IntList::~IntList(void)
{
    Init();
}

IntList &IntList::Init(int n)
{
    if (ints) delete [] ints;
    count = 0;
    ints = NULL;

    int i;
    for (i=0 ; i<n ; i++)
        Add(0);
    return *this;
}

int &IntList::operator[](int ix)
{
    static int zero;
    zero = 0;
    return ix >= 0 && ix < count ? ints[ix] : zero;
}

void IntList::operator=(IntList &l)
{
    int i;
    (*this).Init();
    for (i = 0; i < l.Count(); i++)
        Append(l[i]);
}

int IntList::Max(void)
{
    int i,max=ints[0];

    for (i=1 ; i<count ; i++)
        if (ints[i]>max)	max=ints[i];
    return max;
}

int IntList::Min(void)
{
    int i,min=ints[0];

    for (i=1 ; i<count ; i++)
        if (ints[i]<min)	min=ints[i];
    return min;
}

IntList &IntList::Prepend(const int i)
{
    int *newints = new int[count+1];

    if (newints) {
        if (count) memcpy(newints+1, ints, count*sizeof(int));
        delete [] ints;
        ints = newints;
        ints[0] = i;
       count++;
    }
    return *this;
}

IntList &IntList::Append(const int i)
{
    int *newints = new int[count+1];

    if (newints) {
        if (count) memcpy(newints, ints, count*sizeof(int));
        delete [] ints;
        ints = newints;
        ints[count++] = i;
    }
    return *this;
}

int IntList::Has(const int i)
{
    int ix;
    for (ix = 0; ix < count; ix++)
        if (ints[ix] == i)
            return ix;
    return -1;
}

void IntList::Delete(int i)
{
    DeleteIndex(Has(i));
}

void IntList::DeleteIndex(int ix)
{
    if (ix < 0 || ix >= count)
        return;
    int i;
    for (i = ix+1; i < count; i++)
        ints[i-1] = ints[i];
    if (--count == 0) {
        delete [] ints;
        ints = NULL;
    }
}


int ReadWriteIntList::Read(GameFile &game)
{
    if (!game.GetEntry(Name()))
        return 0;
    Init();
    const char * token;
    while ((token = game.GetToken())) {
        int i;
        sscanf(token,"%d",&i);
        Add(i);
    }
    return 1;	
}

int ReadWriteIntList::Write(GameFile &game)
{
    if (!count)
        return 1;
    game.PutEntry(Name(), OneLiner());
    int i;
    for (i=0; i < count; i++)
        game.PutToken((*this)[i]);
    return 1;
}


SingleFileIntList::SingleFileIntList(const char * gametype)
{
    gameType = Strdup(gametype);
}

SingleFileIntList::~SingleFileIntList(void)
{
    if (gameType)	free(gameType);
    gameType = NULL;
}

int SingleFileIntList::ReadFile(void)
{
    GameFile game;
    if (!game.Read(gameType,FileName()))
        return 0;
    return Read(game);
}

int SingleFileIntList::WriteFile(void)
{
    GameFile game;
    if (!game.Write(gameType,FileName()))
        return 0;
    return Write(game);
}


DoubleList::DoubleList(void)
{
    count = 0;
    doubles = NULL;
}

DoubleList::DoubleList(DoubleList &l)
{
    count = 0;
    doubles = NULL;
    (*this) = l;
}

DoubleList::~DoubleList(void)
{
    Init();
}

DoubleList &DoubleList::Init(int n)
{
    if (doubles) delete [] doubles;
    count = 0;
    doubles = NULL;

    int i;
    for (i=0 ; i<n ; i++)
        Add(0);
    return *this;
}

double &DoubleList::operator[](int ix)
{
    static double zero;
    zero = 0;
    return ix >= 0 && ix < count ? doubles[ix] : zero;
}

void DoubleList::operator=(DoubleList &l)
{
    int i;
    (*this).Init();
    for (i = 0; i < l.Count(); i++)
        Append(l[i]);
}

double DoubleList::Max(void)
{
    int i;
    double max=doubles[0];

    for (i=1 ; i<count ; i++)
        if (doubles[i]>max)	max=doubles[i];
    return max;
}

double DoubleList::Min(void)
{
    int i;
    double min=doubles[0];

    for (i=1 ; i<count ; i++)
        if (doubles[i]<min)	min=doubles[i];
    return min;
}

DoubleList &DoubleList::Prepend(const double i)
{
    double *newdoubles = new double[count+1];

    if (newdoubles) {
        if (count) memcpy(newdoubles+1, doubles, count*sizeof(double));
        delete [] doubles;
        doubles = newdoubles;
        doubles[0] = i;
       count++;
    }
    return *this;
}

DoubleList &DoubleList::Append(const double i)
{
    double *newdoubles = new double[count+1];

    if (newdoubles) {
        if (count) memcpy(newdoubles, doubles, count*sizeof(double));
        delete [] doubles;
        doubles = newdoubles;
        doubles[count++] = i;
    }
    return *this;
}

int DoubleList::Has(const double i)
{
    int ix;
    for (ix = 0; ix < count; ix++)
        if (doubles[ix] == i)
            return ix;
    return -1;
}

void DoubleList::Delete(double i)
{
    DeleteIndex(Has(i));
}

void DoubleList::DeleteIndex(int ix)
{
    if (ix < 0 || ix >= count)
        return;
    int i;
    for (i = ix+1; i < count; i++)
        doubles[i-1] = doubles[i];
    if (--count == 0) {
        delete [] doubles;
        doubles = NULL;
    }
}



#ifndef LISTS_H
#define LISTS_H

#include <stdio.h>

#include "gamefile.h"

typedef char*	String;

class StringList {
protected:
    int count;
    String *str;
public:
    StringList(void);
    virtual ~StringList(void);
    StringList &Init(int n=0);
    StringList &Init(const StringList &sl)
        { Init(); Add(sl); return *this; }
    char * operator[](int n);
    StringList &Prepend(const char * s);
    StringList &Append(const char * s);
    StringList &Add(const char * s)
        { return Append(s); }
    StringList &Add(const StringList &sl)
        { int i; for (i = 0; i < sl.count; i++) Add(sl.str[i]); return *this; }
	int Has(const char * s, int start=0);
	void Delete(const char * s);
	void DeleteIndex(int n);
	void Change(int ix, const char * s);
	int Count(void) { return count; }
	void Swap(int i, int j)
        { char * t = str[i]; str[i] = str[j]; str[j] = t; }
};


class ReadWriteStringList:public StringList {
public:
	int Read(GameFile &game);
	int Write(GameFile &game);
protected:
	virtual const char * Name(void) = 0;
	virtual int OneLiner(void) { return 1; }
};

class SingleFileStringList:public ReadWriteStringList {
public:
	int ReadFile(const char * gameType);
	int WriteFile(const char * gameType);
protected:
	virtual const char * FileName(void) = 0;
	virtual int OneLiner(void) { return 0; }
};



class IntList {
protected:
	int count;
	int *ints;
public:
	IntList(void);
	IntList(IntList &l);
	virtual ~IntList(void);
	IntList &Init(int n=0);
	int &operator[](int n);
	void operator=(IntList &l);
	IntList &Prepend(const int i);
	IntList &Append(const int i);
	IntList &Add(const int i) { return Append(i); }
	int Has(const int i);
	int Max(void);
	int Min(void);
	void Delete(int i);
	void DeleteIndex(int n);
	int Count(void) { return count; }
	void Swap(int i, int j) { int t = ints[i]; ints[i] = ints[j]; ints[j] = t; }
};

class ReadWriteIntList:public IntList {
public:
	int Read(GameFile &game);
	int Write(GameFile &game);
protected:
	virtual const char * Name(void) = 0;
	virtual int OneLiner(void) { return 1; }
};

class SingleFileIntList:public ReadWriteIntList {
public:
	int ReadFile(void);
	int WriteFile(void);
protected:
	char * gameType;
	virtual const char * FileName(void) = 0;
	virtual int OneLiner(void) { return 0; }
	SingleFileIntList(const char * gametype);
//	SingleFileIntList(void);
	virtual ~SingleFileIntList(void);
};

class DoubleList {
protected:
	int count;
	double *doubles;
public:
	DoubleList(void);
	DoubleList(DoubleList &l);
	virtual ~DoubleList(void);
	DoubleList &Init(int n=0);
	double &operator[](int n);
	void operator=(DoubleList &l);
	DoubleList &Prepend(const double i);
	DoubleList &Append(const double i);
	DoubleList &Add(const double i) { return Append(i); }
	int Has(const double i);
	double Max(void);
	double Min(void);
	void Delete(double i);
	void DeleteIndex(int n);
	int Count(void) { return count; }
	void Swap(int i, int j) { double t = doubles[i]; doubles[i] = doubles[j]; doubles[j] = t; }
};

#endif

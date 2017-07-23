#include <stdio.h>
#ifndef FILE_H
#define FILE_H


#undef FILEDEBUG

#ifndef FILEDEBUG
#define f_open fopen
#ifdef WIN32
#define f_close fclose
#else
#define f_close(file) { fsync(fileno((file))); fclose((file)); }
#endif
#else
FILE *f_open(const char *name, const char *mode);
int f_close(FILE *file);
#endif

#endif


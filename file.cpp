#include <stdio.h>
#include <unistd.h>
#include "file.h"

#ifdef FILEDEBUG
FILE *f_open(const char *name, const char *mode)
{
  FILE *file = fopen(name,mode);
  FILE *logf = fopen("/home/PBM/pbmserv/tmp/log","a");
  if (logf) {
    if(file){
      fprintf(logf,"\"%s\" opened in mode \"%s\" as %08X, fileno = %i\n",name,mode,file,fileno(file));
    } else {
      fprintf(logf,"\"%s\" not opened in mode \"%s\"\n",name,mode);
    }
    fsync(fileno(logf));
    fclose(logf);
  }
  return file;
}

int f_close(FILE *file)
{
  FILE *logf = fopen("/home/PBM/pbmserv/tmp/log","a");
  if (logf) {
    fprintf(logf,"%08X closing (fileno %i)\n",file,fileno(file));
    fsync(fileno(logf));
    fclose(logf);
  }
  fsync(fileno(file));
  int result = fclose(file);
  return result;  
}
#endif

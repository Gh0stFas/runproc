#ifndef _POPEN2_H
#define _POPEN2_H

FILE *popen2(const char *, const char *, int *);
int pclose2(FILE *);

#endif

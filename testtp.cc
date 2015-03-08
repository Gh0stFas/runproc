////////////////////////////////////////////////////
//
// Quick test to check the throughput of runproc
//
// Takes a single argument defining the number of
// lines to write.
//
// ex. runproc -c "./testtp 10000" -l testtp
//     Writes 10000 lines to a log file prefixed with
//     testtp.
//
////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
  unsigned int nlines=10000;
  if(argc > 1) nlines = atol(argv[1]);

  for(unsigned int i=0;i<nlines;i++){
    printf("The quick brown frox jumped over the lazy dog.\n");
    //printf("The quick brown frox jumped over the lazy dog.");
  }

  return 0;
}

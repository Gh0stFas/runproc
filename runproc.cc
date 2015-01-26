/***************************************************
 * Written to be a quick way to run a process and
 * log its output. When running, you can kill it
 * and the child processes with either CTRL+C or
 * send the negative process id of the runproc
 * process to kill the entire process group.
 *
 * ie kill -INT -14054
***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "popen2.h"

#define MAX_CMD_LEN 1024
#define DEFAULT_NBYTES 1000000000

//{{{ command line parsing
typedef struct {
  int verbose;
  char cmd[MAX_CMD_LEN];
  char log_prefix[PATH_MAX];
  char log_dir[PATH_MAX];
  char log_arch_dir[PATH_MAX];
  int instance;
  unsigned long num_bytes;
} OPTIONS_T;

void print_usage(){
  printf("runproc [OPTION] ...\n");
  printf("\n\033[1mDESCRIPTION\033[0m\n");
  printf("\tRuns a given process and writes standard out/error to log files.\n");
  printf("\n\033[1mOPTION\033[0m\n");
  printf("\n\t-c, --command=STRING\n");
  printf("\t\tQuoted command string. (Required)\n");
  printf("\n\t-v, --verbose=LEVEL\n");
  printf("\t\tVerbosity level.\n");
  printf("\n\t-l, --log-prefix=STRING\n");
  printf("\t\tLog prefix. (Required)\n");
  printf("\n\t-d, --dir=STRING\n");
  printf("\t\tLog directory. (Default: CWD)\n");
  printf("\n\t-a, --arch-dir=STRING\n");
  printf("\t\tLog archive directory. (Default: CWD/arch)\n");
  printf("\n\t-b, --bytes=NBYTES\n");
  printf("\t\tNumber of bytes per log file. (Default: %d)\n",DEFAULT_NBYTES);
  printf("\n\t-i, --instance=NUMBER\n");
  printf("\t\tProcess number to distinguish like processes. (Default: 1)\n");
  return;
}

int parse_opts(int argc, char **argv, OPTIONS_T *op){
  int opt= 0;
  int rtn=0;
  long tc;

  //Specifying the expected options
  //The two options l and b expect numbers as argument
  static struct option long_options[] = {
      {"verbose", optional_argument, 0,'v'},
      {"command", required_argument, 0,'c'},
      {"log-prefix", required_argument, 0,'l'},
      {"instance", required_argument, 0,'i'},
      {"bytes", required_argument, 0,'b'},
      {"dir", required_argument, 0,'d'},
      {"arch-dir", required_argument, 0,'a'},
      {0,0,0,0}
  };

  // Default values
  op->verbose=0;
  op->instance=1;
  op->num_bytes=DEFAULT_NBYTES;
  op->cmd[0]='\0';
  op->log_prefix[0]='\0';
  op->log_dir[0]='\0';
  op->log_arch_dir[0]='\0';

  int long_index =0;
  while ((opt = getopt_long(argc, argv,"hv::c:l:i:b:d:a:", long_options, &long_index )) != -1) {
    if(opt == 'v'){
      op->verbose = 1;
    }
    else if(opt == 'c'){
      strncpy(op->cmd,optarg,MAX_CMD_LEN);
    }
    else if(opt == 'l'){
      strncpy(op->log_prefix,optarg,PATH_MAX);
    }
    else if(opt == 'a'){
      strncpy(op->log_arch_dir,optarg,PATH_MAX);
    }
    else if(opt == 'd'){
      strncpy(op->log_dir,optarg,PATH_MAX);
    }
    else if(opt == 'h'){
      print_usage();
      rtn=-1;
      break;
    }
    else if(opt == 'i'){
      char *eptr=NULL;
      tc = strtol(optarg, &eptr, 10);
      
      if(tc == 0 && eptr != NULL){
        printf("Invalid value '%s' passed to --instance\n",optarg);
        rtn=-1;
        break;
      } else {
        if(tc <= 0){
          printf("Invalid instance %d given\n",tc);
          rtn=-1;
          break;
        }
        else op->instance=tc;
      }

    }
    else if(opt == 'b'){
      char *eptr=NULL;
      tc = strtol(optarg, &eptr, 10);
      
      if(tc == 0 && eptr != NULL){
        printf("Invalid value '%s' passed to --bytes\n",optarg);
        rtn=-1;
        break;
      } else {
        if(tc <= 0){
          printf("Invalid bytes %d given\n",tc);
          rtn=-1;
          break;
        }
        else op->num_bytes=tc;
      }

    }

    else if(opt == '?'){
      // An unrecognized option was on the command line. Most applications
      // fall off here and drop execution while printing the usage statement.
      // We're going to print a warning and continue execution.
      printf("Unrecognized option found\n");
      
      print_usage();
      rtn=-1;
      break;
    }
  }

  // If no arguments were given then print the help. If no files were given then print an error. In
  // either case flag an exit
  if(argc <= 1){
    print_usage();
    rtn=-1;
  }
  
  return rtn;
}
//}}}

//{{{ run
int run(char *cmd, char *log_dir, char *log_prefix, char *log_arch_dir,  int instance, unsigned long rollover_size){
  //char stdval[129];
  char stdval[MAX_INPUT];
  char *stdval_r;
  char log_fname[PATH_MAX];
  char log_base[PATH_MAX];
  char log_fname_arch[PATH_MAX];
  char cmd_r[MAX_CMD_LEN];
  char line_tstamp[30];
  char log_date[9];
  char chk_date[9];
  char log_time[7];
  char log_pre[PATH_MAX];
  time_t tval = time(NULL);
  int nread,nwrite,nwrit,nwritten;
  char log_marker[100];
  int roll_log=0;
  int archive_number=0;
  unsigned long nbytes_written=0;
  int status;
  int child_pid;

  // Check the inputs
  if(strlen(log_dir) == 0){
    printf("No log directory provided.\n");
    printf("...exiting\n");
    return 1;
  }
  if(strlen(log_arch_dir) == 0){
    printf("No log archive directory provided.\n");
    printf("...exiting\n");
    return 1;
  }
  if(strlen(cmd) == 0){
    printf("No command given to execute.\n");
    printf("...exiting\n");
    return 1;
  }
  if(strlen(log_prefix) == 0){
    printf("No log prefix provided.\n");
    printf("...exiting\n");
    return 1;
  }
  // Redirect stderr to stdout
  sprintf(cmd_r,"%s 2>&1",cmd);

  // If an underscore was already provided by the log_prefix then
  // don't add one.
  strcpy(log_pre,log_prefix);
  if(log_pre[strlen(log_pre)-1] == '_') log_pre[strlen(log_pre)-1] = '\0';

  strftime(log_date,9,"%Y%m%d",localtime(&tval));
  strftime(log_time,7,"%H%M%S",localtime(&tval));
  sprintf(log_marker,"\n\n######################### Start of log (time=%s) #########################\n\n",log_time);
  sprintf(log_base,"%s_%s_%d.log",log_pre,log_date,instance);
  sprintf(log_fname,"%s/%s",log_dir,log_base);

  printf("Log name: %s\n",log_fname);

  FILE *logf = fopen(log_fname,"a");

  if(logf != NULL){
    // Write a log start marker
    fwrite(log_marker,sizeof(char),strlen(log_marker),logf);

    FILE *stdoe = popen2(cmd_r,"r",&child_pid);
    if(stdoe != NULL){
      printf("Child PID= %d\n",child_pid);
      while(!feof(stdoe)){
        ///////////////// Read ///////////////////////////
        stdval_r = fgets(stdval,MAX_INPUT,stdoe);
        if(stdval_r != NULL){
          nread=strlen(stdval_r);
        }
        else {
          // If the NULL is due to end-of-file then go ahead and push us to
          // the top of the loop to get us out of here. Otherwise print an
          // error and break.
          if(!feof(stdoe)) perror("fgets: ");
          break;
        }
        ///////////////// Read ///////////////////////////

        //////////////// Write ////////////////////
        // Write out a timestamp first
        tval = time(NULL);
        strftime(line_tstamp,30,"%Y-%m-%d::%H:%M:%S  ||  ",localtime(&tval));
        fwrite(line_tstamp,sizeof(char),strlen(line_tstamp),logf);

        // Loop to write the data
        nwritten=0;
        nwrite=nread;
        while(nwritten != nread){
          nwrit = fwrite(stdval,sizeof(char),nwrite,logf);
          if(nwrit <= 0){
            perror("fwrite: ");
            break;
          }
          else {
            nwritten+=nwrit;
            nwrite-=nwrit;
          }
        }
        fflush(logf);
        nbytes_written += (unsigned long) nwritten;
        //////////////// Write ////////////////////

        // Check to see if we've hit a rollover point
        // Number of bytes
        if(nbytes_written >= rollover_size){
          printf("Max log file size reached. Rolling over log...\n");
          roll_log=1;
        }
        // Check the date
        tval = time(NULL);
        strftime(chk_date,9,"%Y%m%d",localtime(&tval));
        if(strcmp(chk_date,log_date) != 0){
          printf("A new day has begun. Rolling over log...\n");
          roll_log=1;
        }

        if(roll_log){
          // Close the current log file
          fclose(logf);

          // Archive the old file
          archive_number++;
          sprintf(log_fname_arch,"%s/%s.arch%d",log_arch_dir,log_base,archive_number);
          printf("Archive name: %s\n",log_fname_arch);
          status = rename(log_fname,log_fname_arch);
          if(status != 0){
            printf("Failed to move log file to the archive.\n");
            perror("rename: ");
          }

          // Setup everything for the new log file
          strftime(log_date,9,"%Y%m%d",localtime(&tval));
          strftime(log_time,7,"%H%M%S",localtime(&tval));
          sprintf(log_marker,"\n\n######################### Start of log (time=%s) #########################\n\n",log_time);
          sprintf(log_base,"%s_%s_%d.log",log_pre,log_date,instance);
          sprintf(log_fname,"%s/%s",log_dir,log_base);

          printf("Log name: %s\n",log_fname);
         
          // Open a new log file
          logf = fopen(log_fname,"w");
          if(logf == NULL){
            printf("Failed to open new log file.\n");
            perror("fopen: ");
            printf("...exiting\n");

            // TODO: This should get us out of the loop but does not guarantee
            // that the process we spawned is actually dead. Need to work out
            // the details of killing the spawned process.
            break;
          }
          else fwrite(log_marker,sizeof(char),strlen(log_marker),logf);

          // Reset counters and flags
          nbytes_written=0;
          roll_log=0;
        }
      }
    }
    else perror("popen: ");

    if(stdoe != NULL) pclose2(stdoe);
  }
  else perror("fopen: ");

  if(logf != NULL) fclose(logf);

  return 0;
}
//}}}

void sigint_ignore(int signum){
  printf("Caught CTRL-C\n");
}

int main(int argc, char **argv){
  OPTIONS_T opts;
  int ostat = parse_opts(argc,argv,&opts);
  if(ostat != 0) exit(1);

  if(strlen(opts.log_dir) == 0){
    // Set it to the current working directory
    char cwd[PATH_MAX];
    char *nptr;
    if((nptr = getcwd(cwd,sizeof(cwd))) == NULL){
      perror("getcwd: ");
      return 1;
    }
    strcpy(opts.log_dir,cwd);

  }

  // Make sure the directory exists and if not attempt to create it
  int status = mkdir(opts.log_dir,S_IRWXU|S_IRGRP|S_IWGRP|S_IROTH);
  // Couldn't create the directory
  if(status == -1){
    // Why?
    if(errno != EEXIST){
      perror("mkdir: ");
      return 1;
    }
  }

  if(strlen(opts.log_arch_dir) == 0){
    sprintf(opts.log_arch_dir,"%s/arch",opts.log_dir);
  }

  // Make sure the directory exists and if not attempt to create it
  status = mkdir(opts.log_arch_dir,S_IRWXU|S_IRGRP|S_IWGRP|S_IROTH);
  // Couldn't create the directory
  if(status == -1){
    // Why?
    if(errno != EEXIST){
      perror("mkdir: ");
      return 1;
    }
  }

  printf("Running: %s\n",opts.cmd);
  printf("Log dir: %s\n",opts.log_dir);
  printf("Log archive dir: %s\n",opts.log_arch_dir);
  printf("Log prefix: %s\n",opts.log_prefix);
  printf("Log bytes: %lu\n",opts.num_bytes);

  // Setup a signal to ignore SIGINT. the reason for this
  // is that if you allow this process to die on SIGINT then
  // you don't get the child processes time to react to SIGINT
  // as well and clean up if possible. Doing it this way causes
  // the SIGINT to be passed on through to the child processes,
  // or process group, allowing them to return as intended and
  // allows this process to pick up any last messages.
  //
  // NOTE: In order to replicate a CTRL+c with the kill command
  // you must send the SIGINT to the entire process group. You
  // can do this by turning the process ID of runproc to a
  // negative. For instance, if the process ID of runproc was
  // 20405 then you could send SIGINT properly to the process
  // group with kill like this:  kill -INT -20405.
  signal(SIGINT,sigint_ignore);

  run(opts.cmd,
      opts.log_dir,
      opts.log_prefix,
      opts.log_arch_dir,
      opts.instance,
      opts.num_bytes);

  return 0;
}

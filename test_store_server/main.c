#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include <getopt.h>
#include <mycache.h>
#include <mystore_srv.h>

#define OPTIONS_SET "vft:"
#define ADDITIONAL_ARGS 0

static int debug_level = DEBUG_INIT;
static int end = 0;
static int printStats = 0;
static int flushTimeInSeconds = 15;
static FILE *logFile;
static unsigned long int totalRequests = 0;
static unsigned long int totalReadRequests = 0;
static unsigned long int totalWriteRequests = 0;

static void exit_handler(int sig_num) {
  switch (sig_num) {
  case SIGINT:
    signal(SIGINT, exit_handler);
    end = 1;
    break;
  case SIGTERM:
    signal(SIGTERM, exit_handler);
    end = 1;
    break;
  case SIGQUIT:
    signal(SIGQUIT, exit_handler);
    end = 1;
    break;
  }
}

static void alarm_handler(int sig_num) {
  if (!end) {
    debug_debug("Alarm is going to flush");
    MYC_flushAll();
    debug_debug("New alarm in %d seconds", flushTimeInSeconds);
    alarm(flushTimeInSeconds);
    signal(SIGALRM, alarm_handler);
  }
}

static void redirectingMessagesToALogFile(char *filename) {
  char *home_dir = getenv("HOME");
  char *filepath = malloc(strlen(home_dir) + strlen(filename) + 1);
  strncpy(filepath, home_dir, strlen(home_dir) + 1);
  strncat(filepath, filename, strlen(filename) + 1);
  printf("\033[0;35mLOG FILES -> %s\033[0m\n", filepath);
  logFile = freopen(filepath, "w", stderr);
  fclose(stdout);
  fclose(stdin);
  free(filepath);
}

static void daemon_debuglevel_rotate() {
  debuglevel_rotate();
  MYC_debuglevel_rotate();
  STORS_debuglevel_rotate();
  debug_info("\033[0;33mRotating debug level in all my libraries. Current "
             "level=%d.\033[0m",
             debug_level);
}

static void printStadistics() { printStats = 1; }

static void daemonServer() {
  if (MYC_initCache() != 0) {
    debug_error("Error initializing cache.");
    exit(1);
  }

  if (STORS_init() != 0) {
    debug_error("Error initializing server side API.");
    MYC_closeCache();
    exit(1);
  }

  debug_info("Test store server started OK.");

  debug_debug("New alarm in %d seconds", flushTimeInSeconds);
  alarm(flushTimeInSeconds);
  while (!end) {
    request_message_t req;
    answer_message_t answer;

    int status = STORS_readrequest(&req);
    if (status == -1) {
      debug_info("No request received.");
    } else {
      totalRequests++;
      answer.mtype = req.return_to;

      switch (req.requested_op) {
      case MYSCOP_READ:
        totalReadRequests++;
        answer.status = MYC_readEntry(req.index, &(answer.data));
        debug_debug("Read operation (client=%ld, idx=%d) ret %d.",
                    req.return_to, req.index, status);
        debug_verbose("id: %u, age: %d, gender: %d, name: %s",
                      answer.data.registerid, answer.data.age,
                      answer.data.gender, answer.data.name);
        break;

      case MYSCOP_WRITE:
        totalWriteRequests++;
        answer.status = MYC_writeEntry(req.index, &(req.data));
        debug_debug("Write operation (client=%ld, idx=%d) ret %d.",
                    req.return_to, req.index, status);
        break;

      default:
        debug_error("Unknown operation received from client.");
        answer.status = -1;
        break;
      }

      status = STORS_sendanswer(&answer);
      if (status != 0) {
        debug_error("Problems sending back an answer.");
        break;
      }
    }

    if (printStats) {
      debug_info("\033[0;32mprocessed:%lu reads:%lu writes:%lu\033[0m",
                 totalRequests, totalReadRequests, totalWriteRequests);
      printStats = 0;
    }
  }

  alarm(0);
  signal(SIGALRM, SIG_IGN);

  if (STORS_close() != 0) {
    debug_error("Error closing server API.");
  }
  if (MYC_closeCache() != 0) {
    debug_error("Error closing cache.");
    exit(1);
  }

  debug_info("Test store server ended OK.");

  if (logFile != NULL) {
    fclose(logFile);
    logFile = NULL;
  }
}

int main(int argc, char **argv) {

  int c;
  int errorWithOptions = 0;
  opterr = 0;
  int detach = 1;

  while (1) {
    c = getopt(argc, argv, OPTIONS_SET);
    if (c == EOF) {
      break;
    }
    switch (c) {
    case 'v':
      daemon_debuglevel_rotate();
      break;
    case 'f':
      detach = 0;
      break;
    case 't':
      flushTimeInSeconds = atoi(optarg);
      if (flushTimeInSeconds <= 0) {
        errorWithOptions = 1;
      }
      break;
    case '?':
      errorWithOptions = 1;
      break;
    case ':':
      errorWithOptions = 1;
      break;
    }
  }

  if ((argc - optind) != ADDITIONAL_ARGS) {
    errorWithOptions = 1;
  }
  if (errorWithOptions) {
    debug_error(
        "Incorrect parameters please provide any or none of these:\n>\t-v: "
        "Increase logging level\n>\t-t [time]: Set flush timer (time must be "
        "greater than 0)\n>\t-f: Run the server in the foreground (no daemon)");
    exit(1);
  }
  signal(SIGTERM, exit_handler);
  signal(SIGINT, exit_handler);
  signal(SIGQUIT, exit_handler);
  signal(SIGALRM, alarm_handler);
  signal(SIGHUP, SIG_IGN);
  signal(SIGUSR1, printStadistics);
  signal(SIGUSR2, daemon_debuglevel_rotate);

  if (detach) {
    if (fork() == 0) {
      setsid();
      redirectingMessagesToALogFile("/store_server.log");
      daemonServer();
      exit(EXIT_SUCCESS);
    } else {
      exit(EXIT_SUCCESS);
    }
  } else {
    daemonServer();
    exit(EXIT_SUCCESS);
  }

  return (EXIT_SUCCESS);
}

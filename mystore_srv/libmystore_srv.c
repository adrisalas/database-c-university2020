#include "debug.h"
#include "messages.h"
#include "mystore_srv.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEBUG_LEVEL 0
#define SEND_TO_SERVER 1

static int message_queue = -1;

static int debug_level = DEBUG_INIT;

/**
 * Initialize the server library: open message queue, etc.
 * @return -1 in case of error during initialization. 0 means OK.
 */
int STORS_init() {

  key_t key = IPC_PRIVATE;
  key = (key_t)getuid();

  debug_verbose("Opening message queue in server API... (key=0x%08x)", key);
  message_queue =
      msgget(key, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  if (-1 == message_queue) {
    switch (errno) {
    case EEXIST:
      debug_perror(
          "There is another server running using the same IPC queue. ");
      break;
    case ENOMEM:
      debug_perror("System has not enough memory to create a queue. ");
      break;
    case ENOSPC:
      debug_perror("Cannot create a queue, maximum of queues reached. ");
      break;
    }
    return -1;
  }

  debug_info("Message queue opened in server API. (key=0x%08x)", key);
  return 0;
}

/**
 * This function finishes the cache. It flushes all the information inside the
 * cache that is not written to the file yet and closes the file.
 * @return
 */
int STORS_close() {
  if (0 != msgctl(message_queue, IPC_RMID, NULL)) {
    debug_perror("Error removing message queue");
    return -1;
  }
  debug_info("Message queue removed in server API. (key=0x%08x)",
             MYSTORE_API_KEY);

  message_queue = -1;

  return 0;
}

/**
 * This function reads a request from the message queue.
 * This function will wait blocked until it receives a request.
 * When returning, the request passed by reference as pameter will contain the
 * data of the request received from the message queue.
 * The request will be processes outside this library.
 * @param request Is a pointer to a request structure to return a request
 * received from the client.
 * @return Return 0 if OK. -1 in case of some error receiving.
 */
int STORS_readrequest(request_message_t *request) {

  debug_verbose("Receiving request from client (type=%d).", MYSAPMT_REQUEST);

  int status;

  do {
    status = msgrcv(message_queue, request, sizeof(request_message_t),
                    SEND_TO_SERVER, 0);
    if (-1 != status) {
      break;
    }

    if (errno == EIDRM) {
      debug_perror("Message queue is removed. ");
      return -1;
    } else if (errno == EINTR) {
      debug_debug("Signal received, aborting reading message");
      return -1;
    }

    debug_perror("Unexpected error receiving message, retrying. ");

  } while (1);

  debug_debug("Request received from client (cliend id=%ld, op=%d, idx=%d).",
              request->return_to, request->requested_op, request->index);

  return 0;
}

/**
 * This function send an answer structure to a client through a message queue.
 * @param answer This structure is already initialized and ready to be sent.
 * @return Return 0 if OK. -1 in case of some error sending.
 */
int STORS_sendanswer(answer_message_t *answer) {

  debug_verbose("Sending answer to client (client id=%ld, status=%d).",
                answer->mtype, answer->status);
  int status;

  do {
    status = msgsnd(message_queue, answer, sizeof(answer_message_t), 0);

    if (-1 != status) {
      break;
    }

    if (errno == EIDRM) {
      debug_perror("Message queue is removed");
      return -1;
    }

    debug_perror("Error sending message, retrying");

  } while (1);

  debug_debug("Answer sent to client (client id=%ld, status=%d).",
              answer->mtype, answer->status);

  return 0;
}

/** Increases current debug level or reset to 0 if maximum is reached. */
void STORS_debuglevel_rotate() { debuglevel_rotate(); }
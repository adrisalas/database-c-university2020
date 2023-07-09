#include "debug.h"
#include "messages.h"
#include "mystore_cli.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SEND_TO_SERVER 1

static int message_queue = -1;

static int debug_level = DEBUG_INIT;

/**
 * Initialize the client API: open message queue, etc.
 * @return -1 in case of error during initialization. 0 means OK.
 */
int STORC_init() {

  key_t key = IPC_PRIVATE;
  key = (key_t)getuid();

  debug_verbose("Opening message queue in client API. (key=0x%08x)", key);
  message_queue = msgget(key, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  if (-1 == message_queue) {
    switch (errno) {
    case EACCES:
      debug_error("Client has no permission to access the IPC queue");
      break;
    case ENOENT:
      debug_error("Server is not running");
      break;
    }
    return -1;
  }

  debug_info("Message queue opened in client API. (key=0x%08x)", key);
  return 0;
}

/**
 * This function finishes the client API. You should not remove the queue in
 * the client as thre may be more clients.
 * @return -1 in case of error during cleaning. 0 means OK.
 */
int STORC_close() {

  debug_info("Message queue closed in client API.");
  /*
   * IN POSIX YOU CAN CLOSE THE QUEUE WITHOUT REMOVING IT TO OTHER CLIENTS, WITH
   * SYSTEM V MESSAGE QUEUE IF YOU "CLOSE" IT, IT WOULD BE REMOVED FROM THE
   * SYSTEM.
   */
  message_queue = -1;
  return 0;
}

/**
 * This function reads a record from the store server.
 * @param fileIndex This is the index of the record to read.
 * @param record This is a pointer to a record allocated by the user.
 * @return Return the status from the server. 0 is OK.
 */
int STORC_read(int fileIndex, MYRECORD_RECORD_t *record) {

  answer_message_t answer;
  request_message_t request;

  request.mtype = SEND_TO_SERVER;
  request.requested_op = MYSCOP_READ;
  request.return_to = getpid();
  memcpy(&(request.data), record, sizeof(MYRECORD_RECORD_t));
  request.index = fileIndex;

  int status;

  debug_verbose("Sending request to server (idx=%d).", fileIndex);
  do {
    status = msgsnd(message_queue, &request, sizeof(request_message_t), 0);
    if (-1 == status) {
      if (errno == EIDRM) {
        perror("Message queue is removed");
        return -1;
      } else {
        perror("Error sending message, retrying");
        continue;
      }
    } else {
      break;
    }
  } while (1);

  debug_verbose("Receiving answer from server (client id=%ld).",
                request.return_to);
  do {
    status =
        msgrcv(message_queue, &answer, sizeof(answer_message_t), getpid(), 0);
    if (-1 == status) {
      if (errno == EIDRM) {
        perror("Message queue is removed");
        return -1;
      } else {
        perror("Error receiving message, retrying");
        continue;
      }
    } else {
      break;
    }

  } while (1);

  debug_debug("Answer received from server (status=%d).", answer.status);
  if (-1 != answer.status) {
    memcpy(record, &(answer.data), sizeof(MYRECORD_RECORD_t));
  }

  return answer.status;
}

/**
 * This function writes a record to the store server.
 * @param fileIndex This is the index of the record to write.
 * @param record This is a pointer to a record allocated by the user.
 * @return Return the status from the server. 0 is OK.
 */
int STORC_write(int fileIndex, MYRECORD_RECORD_t *record) {

  answer_message_t answer;
  request_message_t request;

  request.mtype = SEND_TO_SERVER;
  request.requested_op = MYSCOP_WRITE;
  request.return_to = getpid();
  memcpy(&(request.data), record, sizeof(MYRECORD_RECORD_t));
  request.index = fileIndex;

  debug_verbose("Sending request to server (idx=%d).", fileIndex);

  int status;

  do {
    status = msgsnd(message_queue, &request, sizeof(request_message_t), 0);
    if (-1 == status) {
      if (errno == EIDRM) {
        perror("Message queue is removed");
        return -1;
      } else {
        perror("Error sending message, retrying");
        continue;
      }
    } else {
      break;
    }
  } while (1);

  debug_verbose("Receiving answer from server (client id=%ld).",
                request.return_to);

  do {
    status =
        msgrcv(message_queue, &answer, sizeof(answer_message_t), getpid(), 0);
    if (-1 == status) {
      if (errno == EIDRM) {
        perror("Message queue is removed");
        return -1;
      } else {
        perror("Error receiving message, retrying");
        continue;
      }
    } else {
      break;
    }
  } while (1);

  debug_debug("Answer received from server (status=%d).", answer.status);
  if (-1 != answer.status) {
    memcpy(record, &(answer.data), sizeof(MYRECORD_RECORD_t));
  }

  return answer.status;
}

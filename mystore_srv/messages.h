#ifndef MESSAGES_H
#define MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#define MYSTORE_API_KEY ((key_t)getuid())
#define MYSTORE_API_CLIENT ((long)getpid())

typedef enum {
  MYSAPMT_REQUEST = 1,
  MYSPMT_GETCLID = 2,
  MYSAPMT_ANYCLIENT = 3
} MYSTORE_API_MTYPES;

typedef enum { MYSCOP_READ = 0, MYSCOP_WRITE = 1 } MYSTORE_CLI_OP;

typedef struct {
  long mtype;
  MYSTORE_CLI_OP requested_op;
  long return_to;
  MYRECORD_RECORD_t data;
} request_message_t;

typedef struct {
  long mtype;
  int status;
  MYRECORD_RECORD_t data;
} answer_message_t;

#ifdef __cplusplus
}
#endif

#endif

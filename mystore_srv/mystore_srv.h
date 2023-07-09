#ifndef MYSTORE_CLI_H
#define MYSTORE_CLI_H

#include <stdint.h>
#include <sys/types.h>

#include <messages.h>
#include <myrecord.h>

#ifdef __cplusplus
extern "C" {
#endif

int STORS_init();
int STORS_close();

int STORS_readrequest(request_message_t *request);
int STORS_sendanswer(answer_message_t *answer);

void STORS_debuglevel_rotate();

#ifdef __cplusplus
}
#endif

#endif /* MYSTORE_CLI_H */

#ifndef MYSTORE_CLI_H
#define MYSTORE_CLI_H

#include <stdint.h>
#include <sys/types.h>

#include <myrecord.h>

#ifdef __cplusplus
extern "C" {
#endif

int STORC_init();
int STORC_close();

int STORC_read(int fileIndex, MYRECORD_RECORD_t *record);
int STORC_write(int fileIndex, MYRECORD_RECORD_t *record);
int STORC_flush(int fileIndex);
int STORC_flushAll();

#ifdef __cplusplus
}
#endif

#endif /* MYSTORE_CLI_H */

#ifndef MYCACHE_H
#define MYCACHE_H

#include <stdint.h>
#include <sys/types.h>

#include "mybucket.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MYC_NUMENTRIES 64
#define MYC_FILENAME "myDBtable.dat"

int MYC_initCache();
int MYC_closeCache();

int MYC_readEntry(int fileIndex, MYRECORD_RECORD_t *record);
int MYC_writeEntry(int fileIndex, MYRECORD_RECORD_t *record);
int MYC_flushEntry(int fileIndex);
int MYC_flushAll();

void MYC_debuglevel_rotate();

#ifdef __cplusplus
}
#endif

#endif

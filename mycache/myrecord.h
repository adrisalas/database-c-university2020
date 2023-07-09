#ifndef MYRECORD_H
#define MYRECORD_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MYRECORD_NAMELENGTH 16

typedef struct {
  unsigned int registerid;
  int age;
  int gender;
  char name[MYRECORD_NAMELENGTH];
} MYRECORD_RECORD_t;

#define myb_newrecord()                                                        \
  ((MYRECORD_RECORD_t *)calloc(1, sizeof(MYRECORD_RECORD_t)))
#define myb_freerecord(r) free(r)

#ifdef __cplusplus
}
#endif

#endif
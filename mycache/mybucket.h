#ifndef MYBUCKET_H
#define MYBUCKET_H

#include "myrecord.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MYBUCKET_RECORDSIZE (sizeof(MYRECORD_RECORD_t))
typedef struct {
  unsigned char record[MYBUCKET_RECORDSIZE];
  unsigned int id;
} MYBUCKET_BUCKET_t;

#define myb_record2bucket(r, b)                                                \
  memcpy(&((b)->record), (r), MYBUCKET_RECORDSIZE);
#define myb_bucket2record(b, r)                                                \
  memcpy((r), &((b)->record), MYBUCKET_RECORDSIZE);

#ifdef __cplusplus
}
#endif

#endif

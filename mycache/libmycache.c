#include "debug.h"
#include "mycache.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int dbFile = -1;

static MYBUCKET_BUCKET_t *CacheEntries = NULL;

static int *CacheDirty = NULL;

static int debug_level = DEBUG_INIT;

/**
 * Allocate memory for the cache.
 * @param n Number of buckets of the cache.
 * @return A pointer to a table with the required number of buckets.
 * NULL means a problem allocating memory.
 */
static MYBUCKET_BUCKET_t *allocateCache(int n) {
  return (MYBUCKET_BUCKET_t *)calloc(n, sizeof(MYBUCKET_BUCKET_t));
}

/**
 * Allocate memory for the array of booleans for the dirty flag.
 * @param n Number of buckets of the cache.
 * @return A pointer to a table with the required number of integers.
 * NULL means a problem allocating memory.
 */
static int *allocateDirty(int n) { return (int *)calloc(n, sizeof(int)); }

/**
 * Search for an unused entry in the table.
 * If there's no unused one, just one clean entry.
 * If there's no clean one, return -1.
 * @return The index of the selected entry. -1 means that no entry was unused or
 * clean.
 */
static int searchUnusedOrClean() {
  for (int i = 0; i < MYC_NUMENTRIES; i++) {
    if (0 == CacheEntries[i].id) {
      debug_verbose("returns %d.", i);
      return i;
    }
  }

  for (int i = 0; i < MYC_NUMENTRIES; i++) {
    if (0 == CacheDirty[i]) {
      debug_verbose("returns %d.", i);
      return i;
    }
  }
  debug_verbose("returns %d.", -1);
  return -1;
}

/**
 * Search for an entry already associated with an offset of the file.
 * If there's no such entry, return -1.
 * @return The index of the entry already containing fileIndex. -1 means that no
 * entry was found.
 */
static int searchRecord(int fileIndex) {
  for (int i = 0; i < MYC_NUMENTRIES; i++) {
    if (fileIndex == CacheEntries[i].id) {
      debug_verbose("returns %d.", i);
      return i;
    }
  }
  debug_verbose("returns %d.", -1);
  return -1;
}

/**
 * This function reads one entry from the file into the cache.
 * The entry CachesEntries[cacheIndex] of the cache is read from the position
 * number "CacheEntries[cacheIndex].id" of the file.
 * @param cacheIndex The index of the entry in the cache.
 * @return -1 indicates an error reading the entry. 0 success.
 */
static int readEntry(int cacheIndex) {
  int offset = CacheEntries[cacheIndex].id *
               MYBUCKET_RECORDSIZE; /* Replace 0 with calculation */

  do {
    if (lseek(dbFile, offset, SEEK_SET) == offset) {
      break;
    }

    debug_error("Error seeking into DB file. %s", strerror(errno));

    if (errno != EINTR) {
      return -1;
    }
  } while (1);

  do {
    if (read(dbFile, CacheEntries[cacheIndex].record, MYBUCKET_RECORDSIZE) ==
        MYBUCKET_RECORDSIZE) {
      break;
    }

    debug_error("Error reading from DB file. %s", strerror(errno));

    if (errno != EINTR) {
      return -1;
    }
  } while (1);

  CacheDirty[cacheIndex] = 0;

  return 0;
}

/**
 * This function writes one entry of the cache to the file.
 * The entry CachesEntries[cacheIndex] of the cache is written on the position
 * number "CachesEntries[cacheIndex].id" of the file.
 * @param cacheIndex The index of the entry in the cache.
 * @return -1 indicates an error writing the entry. 0 success.
 */
static int writeEntry(int cacheIndex) {

  int offset = CacheEntries[cacheIndex].id *
               MYBUCKET_RECORDSIZE; /* Replace 0 with calculation */

  do {
    if (lseek(dbFile, offset, SEEK_SET) == offset) {
      break;
    }

    debug_error("Error seeking into DB file. %s", strerror(errno));

    if (errno != EINTR) {
      return -1;
    }
  } while (1);

  do {
    if (write(dbFile, CacheEntries[cacheIndex].record, MYBUCKET_RECORDSIZE) ==
        MYBUCKET_RECORDSIZE) {
      break;
    }

    debug_error("Error writing to DB file. %s", strerror(errno));

    if (errno != EINTR) {
      return -1;
    }
  } while (1);

  CacheDirty[cacheIndex] = 0;
  return 0;
}

/**
 * Initialize the cache: allocate RAM, open file, etc.
 * @return -1 in case of error during initialization. 0 means OK.
 */
int MYC_initCache() {
  CacheEntries = allocateCache(MYC_NUMENTRIES);

  if (CacheEntries == NULL) {
    debug_error("Not enough memory for the entry table.");
    return -1;
  }

  CacheDirty = allocateDirty(MYC_NUMENTRIES);

  if (CacheDirty == NULL) {
    debug_error("Not enough memory for the flags table.");
    return -1;
  }

  dbFile = open(MYC_FILENAME, O_SYNC | O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  if (dbFile < 0) {
    debug_error("Error opening DB file. %s ", strerror(errno));
    return -1;
  }

  debug_info("DB file opened. (%s)", MYC_FILENAME);

  return 0;
}

/**
 * This function finishes the cache. It flushes all the information inside the
 * cache that is not written to the file yet and closes the file.
 * @return
 */
int MYC_closeCache() {
  MYC_flushAll();

  free(CacheEntries);
  CacheEntries = NULL;
  free(CacheDirty);
  CacheDirty = NULL;

  if (close(dbFile) < 0) {
    debug_error("Error closing DB file. %s", strerror(errno));
    dbFile = -1;
    return -1;
  }

  debug_info("DB file closed. (%s)", MYC_FILENAME);
  dbFile = -1;

  return 0;
}

/**
 * This function copies into a record passed as argument from the cache.
 * The cache will be read from the given index of the file if not on the cache.
 * The record structure is property of the user, so we have to copy the content
 * of the cache entry onto it.
 *
 * @param fileIndex This is the index of the record in the file.
 * @param record This is a pointer to a record allocated by the user.
 * @return -1 in case of any error like I/O error when reading. 0 is OK.
 */
int MYC_readEntry(int fileIndex, MYRECORD_RECORD_t *record) {

  int cacheIndex = searchRecord(fileIndex);

  if (cacheIndex < 0) {
    cacheIndex = searchUnusedOrClean();

    if (cacheIndex < 0) {
      cacheIndex = fileIndex % MYC_NUMENTRIES;
      if (-1 == writeEntry(cacheIndex)) {
        debug_error("Error flushing entry to cache.");
        return -1;
      }
    }

    CacheEntries[cacheIndex].id = fileIndex;
    CacheDirty[cacheIndex] = 1;
  }

  if (-1 == readEntry(cacheIndex)) {
    debug_error("Error reading entry from cache.");
    return -1;
  }

  myb_bucket2record(&CacheEntries[cacheIndex], record);
  debug_debug("Entry %d read from cache.", fileIndex);

  return 0;
}

/*
 * This function copies a record passed as argument into the cache.
 * The record will be written at the given index of the file LATER.
 * This funtions does not write the cache entry to the file inmediately.
 * The record structure is property of the user, so we have to copy its
 * content to the entry as the record can be deallocated by the user.
 *
 * @param fileIndex This is the index of the record in the file.
 * @param record This is a pointer to a record allocated by the user.
 * @return -1 in case of any error like I/O error when writing. 0 is OK.
 */
int MYC_writeEntry(int fileIndex, MYRECORD_RECORD_t *record) {

  int cacheIndex = searchRecord(fileIndex);

  if (0 > cacheIndex) {
    cacheIndex = searchUnusedOrClean();
    if (0 > cacheIndex) {
      cacheIndex = record->registerid % MYC_NUMENTRIES;
    }
    if (1 == CacheDirty[cacheIndex]) {
      if (-1 == writeEntry(cacheIndex)) {
        debug_error("Error flushing entry to cache.");
        return -1;
      }
    }
  }

  CacheEntries[cacheIndex].id = fileIndex;
  CacheDirty[cacheIndex] = 1;

  myb_record2bucket(record, &CacheEntries[cacheIndex]);
  debug_debug("Entry %d written to cache.", fileIndex);

  return 0;
}

/**
 * Forces the cache to write the contents of the entry containing the record at
 * "fileIndex" in the file.
 * @param fileIndex This is the index of the entry of the file to be flushed.
 * @return -1 in case of I/O error. 0 is OK.
 */
int MYC_flushEntry(int fileIndex) {

  int i = searchRecord(fileIndex);
  if (0 <= i && 1 == CacheDirty[i]) {
    if (-1 == writeEntry(i)) {
      debug_error("Error flushing entry to cache.");
      return -1;
    }
  }
  debug_debug("Entry %d flushed to disk.", fileIndex);
  return 0;
}

/**
 * Flush any dirty entry in the cache inmediately.
 * @return
 */
int MYC_flushAll() {
  for (int i = 0; i < MYC_NUMENTRIES; i++) {
    if (CacheDirty[i] == 1) {
      if (-1 == writeEntry(i)) {
        debug_error("Error flushing entry to cache.");
        return -1;
      }
    }
  }
  debug_debug("All entries flushed to disk.");

  return 0;
}

/** Increases current debug level or reset to 0 if maximum is reached. */
void MYC_debuglevel_rotate() { debuglevel_rotate(); }

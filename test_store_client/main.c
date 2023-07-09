#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include <mycache.h>
#include <mystore_cli.h>


static int debug_level = DEBUG_INIT;

#define TEST_LENGTH 67
#define NUMBER_CACHE_ENTRIES 64

int main(int argc, char **argv) {
  if (STORC_init() != 0) {
    debug_error("Error initializing client API.");
    exit(1);
  }

  MYRECORD_RECORD_t record;

  for (int k = 0; k < 100; k++)
    for (int j = 1; j < TEST_LENGTH - NUMBER_CACHE_ENTRIES; j++)
      for (int i = j; i < j + NUMBER_CACHE_ENTRIES; i++) {
        record.registerid = i;
        record.age = i;
        record.gender = -1;

        snprintf(record.name, sizeof(record.name), "reg #%d", i);

        if (STORC_write(i, &record) != 0) {
          debug_error("Error writing to the storage.");
          exit(1);
        }
        debug_debug("idx: %d write OK", i);
      }

  if (STORC_close() != 0) {
    debug_error("Error closing API.");
    exit(1);
  }

  debug_info("Write test ended OK.");

  debug_info("Read test started...");
  if (STORC_init() != 0) {
    debug_error("Error initializing client API.");
    exit(1);
  }

  for (int k = 0; k < 100; k++)
    for (int j = 1; j < TEST_LENGTH - NUMBER_CACHE_ENTRIES; j++)
      for (int i = j; i < j + NUMBER_CACHE_ENTRIES; i++) {
        if (STORC_read(i, &record) != 0) {
          debug_error("Error reading from server.");
          exit(1);
        }

        if (record.registerid != i) {
          debug_error("Register at %d contains id %d.", i, record.registerid);
        } else {
          debug_debug("idx: %d read OK", i);
          debug_verbose("id: %u, age: %d, gender: %d, name: %s",
                        record.registerid, record.age, record.gender,
                        record.name);
        }
      }

  if (STORC_close() != 0) {
    debug_error("Error closing client.");
    exit(1);
  }

  debug_info("Read test ended OK.");

  debug_info("Test store client ended OK.");
  return (EXIT_SUCCESS);
}

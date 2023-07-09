#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <string.h>

#define DEBUG_ERROR 0
#define DEBUG_INFO 1
#define DEBUG_DEBUG 2
#define DEBUG_VERBOSE 3
#define DEBUG_INIT DEBUG_INFO

#define debuglevel_increase() {
if (debug_level < DEBUG_VERBOSE)
  debug_level++;
}
#define debuglevel_decrease() {
if (debug_level > DEBUG_ERROR)
  debug_level--;
}
#define debuglevel_rotate() {
if (debug_level < DEBUG_VERBOSE)
  debug_level++;
else
  debug_level = DEBUG_ERROR;
}

#define debug_error(...) {
if (debug_level >= DEBUG_ERROR) {
  fprintf(stderr, "%s:%s()::\033[0;31mERROR\033[0m ", __FILE__, __func__);
  fprintf(stderr, __VA_ARGS__);
  fputc('\n', stderr);
}
}
#define debug_perror(...) {
if (debug_level >= DEBUG_ERROR) {
  fprintf(stderr, "%s:%s()::\033[0;31mERROR\033[0m ", __FILE__, __func__);
  fprintf(stderr, __VA_ARGS__);
  fputs(strerror(errno), stderr);
  fputc('\n', stderr);
}
}
#define debug_info(...) {
if (debug_level >= DEBUG_INFO) {
  fprintf(stderr, "%s:%s()::\033[0;36mINFO\033[0m ", __FILE__, __func__);
  fprintf(stderr, __VA_ARGS__);
  fputc('\n', stderr);
}
}

#ifdef DEBUG_LIB
#define debug_debug(...) {
if (debug_level >= DEBUG_DEBUG) {
  fprintf(stderr, "%s:%s()::\033[0;32mDEBUG\033[0m ", __FILE__, __func__);
  fprintf(stderr, __VA_ARGS__);
  fputc('\n', stderr);
}
}
#define debug_verbose(...) {
if (debug_level >= DEBUG_VERBOSE) {
  fprintf(stderr, "%s:%s()::\033[0;33mVERBOSE\033[0m\033[0m ", __FILE__,
          __func__);
  fprintf(stderr, __VA_ARGS__);
  fputc('\n', stderr);
}
}
#else
#define debug_debug(...) {
}
#define debug_verbose(...) {
}
#endif

#ifdef __cplusplus
}
#endif

#endif

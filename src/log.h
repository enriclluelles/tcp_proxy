#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <errno.h>

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stdout, "[DBG]: " M "\n", ##__VA_ARGS__)
#endif

#define die(M, ...) do {\
	fprintf(stderr, "[FATAL]: " M "\nerrno=%d msg=\"%s\"\n", ##__VA_ARGS__, errno, strerror(errno)); \
	exit(1); \
} while(0)

#define log_errno(M, ...) fprintf(stderr, \
	M " errno=%d msg=\"%s\"\n", ##__VA_ARGS__, errno, strerror(errno));

#endif

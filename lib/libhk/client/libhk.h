#ifndef LIBHK_H_
#define LIBHK_H_

#include <stdint.h>

#define LIBHK_PATH_LENGTH 50

typedef struct __attribute__((__packed__)) libhk_service_request_s {
	uint8_t type;
	uint32_t t0;
	uint32_t interval;
	uint32_t count;
	char path[LIBHK_PATH_LENGTH];
} libhk_service_request_t;

#endif /* LIBHK_H_ */

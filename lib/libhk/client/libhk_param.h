#ifndef LIBHK_PARAM_H_
#define LIBHK_PARAM_H_

#include <stddef.h>
#include <stdint.h>
#include <param/param_types.h>

/**
 * Define memory space
 */
#define LIBHK_COL_EN					0x00
#define LIBHK_STORE_INTERVAL			0x04
#define LIBHK_STORE_EN					0x08
#define LIBHK_STORE_MAXCNT				0x0C
#define LIBHK_SIZE						0x10

/**
 * Setup info about parameters
 */
static const param_table_t libhk_param[] = {

		{.name = "col_en", 			.addr = LIBHK_COL_EN,				.type = PARAM_UINT8,	.size = sizeof(uint8_t)},
		{.name = "store_en", 		.addr = LIBHK_STORE_EN,				.type = PARAM_UINT8,	.size = sizeof(uint8_t)},
		{.name = "store_intv",	 	.addr = LIBHK_STORE_INTERVAL,		.type = PARAM_UINT32,	.size = sizeof(uint32_t)},
		{.name = "store_max", 		.addr = LIBHK_STORE_MAXCNT,			.type = PARAM_UINT16,	.size = sizeof(uint16_t)},

};

#define LIBHK_PARAM_COUNT (sizeof(libhk_param) / sizeof(libhk_param[0]))

#endif /* LIBHK_PARAM_H_ */

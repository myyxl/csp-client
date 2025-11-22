#ifndef LIBHK_PARSER_PARAM_H_
#define LIBHK_PARSER_PARAM_H_

#include <stddef.h>
#include <stdint.h>
#include <param/param_types.h>

#define LIBHK_PARSER							18

/**
 * Define memory space
 */
#define LIBHK_PARSER_EN							0x00
#define LIBHK_PARSER_MONGO_EN					0x01
#define LIBHK_PARSER_INFLUX_EN					0x02

#define LIBHK_PARSER_MONGO_DB_SERVER			0x10
#define LIBHK_PARSER_MONGO_DB_SERVER_LEN		0x40
#define LIBHK_PARSER_MONGO_DB_PORT				0x50
#define LIBHK_PARSER_MONGO_DB_NAME				0x60
#define LIBHK_PARSER_MONGO_DB_NAME_LEN			0x20

#define LIBHK_PARSER_OFFSET						0x80
#define LIBHK_PARSER_OFFSET_UPDATE_NEXT			0x84

#define LIBHK_PARSER_INFLUX_POST_URL			0x90
#define LIBHK_PARSER_INFLUX_POST_URL_LEN		0x70

#define LIHBK_PARSER_SIZE						0x100

/**
 * Setup info about parameters
 */
static const param_table_t libhk_parser_param[] = {

		{.name = "parser_en", 			.addr = LIBHK_PARSER_EN,					.type = PARAM_BOOL,		.size = sizeof(uint8_t)},
		{.name = "mongo_en", 			.addr = LIBHK_PARSER_MONGO_EN,				.type = PARAM_BOOL,		.size = sizeof(uint8_t)},
		{.name = "influx_en", 			.addr = LIBHK_PARSER_INFLUX_EN,				.type = PARAM_BOOL,		.size = sizeof(uint8_t)},

		{.name = "mongo_srv", 			.addr = LIBHK_PARSER_MONGO_DB_SERVER,		.type = PARAM_STRING,	.size = LIBHK_PARSER_MONGO_DB_SERVER_LEN},
		{.name = "mongo_port", 			.addr = LIBHK_PARSER_MONGO_DB_PORT,			.type = PARAM_UINT16,	.size = sizeof(uint16_t)},
		{.name = "mongo_db", 			.addr = LIBHK_PARSER_MONGO_DB_NAME,			.type = PARAM_STRING,	.size = LIBHK_PARSER_MONGO_DB_NAME_LEN},

		{.name = "offset", 				.addr = LIBHK_PARSER_OFFSET,				.type = PARAM_INT32,	.size = sizeof(int32_t)},
		{.name = "offset_update", 		.addr = LIBHK_PARSER_OFFSET_UPDATE_NEXT,	.type = PARAM_BOOL,		.size = sizeof(uint8_t)},

		{.name = "influx_url", 			.addr = LIBHK_PARSER_INFLUX_POST_URL,		.type = PARAM_STRING,	.size = LIBHK_PARSER_INFLUX_POST_URL_LEN},

};

#define LIBHK_PARSER_PARAM_COUNT (sizeof(libhk_parser_param) / sizeof(libhk_parser_param[0]))

#endif /* LIBHK_PARSER_PARAM_H_ */

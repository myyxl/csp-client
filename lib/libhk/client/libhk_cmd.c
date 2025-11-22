#include <stdio.h>
#include <stdlib.h>
#include <libhk.h>
#include <util/clock.h>
#include <csp/csp.h>
#include <csp/csp_endian.h>
#include <command/command.h>
#include <hk_bcn_file.h>
#include <conf_hk.h>
#if ENABLE_PARSER
#include <hk_parser.h>
#endif

static int node = 1;
static int port = 21;

int cmd_hk_server(struct command_context *ctx) {
	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;
	node = atoi(ctx->argv[1]);
	port = atoi(ctx->argv[2]);
	return CMD_ERROR_NONE;
}

int cmd_hk_get(struct command_context *ctx) {

	if (ctx->argc > 6)
		return CMD_ERROR_SYNTAX;

	libhk_service_request_t req;
	req.type = 0;
	req.count = 10;
	req.interval = 60;
	req.t0 = 0;
	req.path[0] = 0;

	if (ctx->argc > 1)
		req.type = atoi(ctx->argv[1]);
	if (ctx->argc > 2)
		req.interval = atoi(ctx->argv[2]);
	if (ctx->argc > 3)
		req.count = atoi(ctx->argv[3]);
	if (ctx->argc > 4){
		int32_t t0 = atoi(ctx->argv[4]);
		/* Autogenerate t0 */
		if (t0 < 0){
			timestamp_t time;
			clock_get_time(&time);
			req.t0 = (time.tv_sec + t0);
			printf("Requesting HK from %"PRIu32"\r\n",req.t0);
		}
		else{
			req.t0 = t0;
		}
	}
	if (ctx->argc > 5)
		strncpy(req.path, ctx->argv[5], LIBHK_PATH_LENGTH);

	/* Deal with endianness */
	req.count = csp_hton32(req.count);
	req.interval = csp_hton32(req.interval);
	req.t0 = csp_hton32(req.t0);

	csp_transaction(CSP_PRIO_NORM, node, port, 0, &req, sizeof(req), NULL, 0);
	return CMD_ERROR_NONE;
}


#if ENABLE_PARSER
int cmd_hk_load(struct command_context *ctx) {

	if (ctx->argc > 4)
		return CMD_ERROR_SYNTAX;

	char *filename = ctx->argv[1];
	uint32_t offset = 0;
	uint32_t count = 0;
	if (ctx->argc > 2)
		offset = atoi(ctx->argv[2]);
	if (ctx->argc > 3)
		count = atoi(ctx->argv[3]);
	FILE * fp = hk_bcn_file_open_read(filename);
	if (fp) {
		uint32_t timestamp;
		uint8_t node;
		uint8_t type;
		uint16_t length;
		uint8_t data[200];
		uint32_t idx = 0;
		uint32_t first_beacon_ts = 0;
		uint32_t last_beacon_ts = 0;
		for (idx = 0; idx < offset; idx++) {
			if (hk_bcn_file_read(fp, &timestamp, &node, &type, &length, data, 198) == -1) {
				break;
			}
		}
		if (idx == offset) {
			for (idx = 0; idx < count; idx++) {
				if (hk_bcn_file_read(fp, &timestamp, &node, &type, &length, data, 198) == -1) {
					if (feof(fp)) {
						printf("End of file reached after reading %"PRIu32" beacons from file '%s' at offset %"PRIu32" (%"PRIu32" total)\r\n",
								idx, filename, offset, idx + offset);
					} else {
						printf("Error reading from file after reading %"PRIu32" beacons from file '%s' at offset %"PRIu32" (%"PRIu32" total)\r\n",
								idx, filename, offset, idx + offset);
					}
					break;
				}
				if (node == 0) {
					/* Assume OBC (node 1) if node not set */
					node = 1;
				}
				last_beacon_ts = hk_parser_direct(node, length, data);
				if (idx == 0) {
					first_beacon_ts = last_beacon_ts;
				}
			}
			printf("Read %"PRIu32" beacons (%"PRIu32" requested) from '%s'. ", idx, count, filename);
			printf("File ts: %"PRIu32", first beacon ts: %"PRIu32", last beacon ts: %"PRIu32"\r\n", timestamp, first_beacon_ts, last_beacon_ts);
		} else {
			printf("Could only offset by %"PRIu32" beacons (%"PRIu32" requested) in file '%s'\r\n", idx, offset, filename);
		}
		hk_bcn_file_close(fp);
		return CMD_ERROR_NONE;
	}
	return CMD_ERROR_FAIL;
}
#endif

command_t hk_commands[] = {
	{
		.name = "server",
		.help = "Setup hk server",
		.usage = "<node> <port>",
		.handler = cmd_hk_server,
	},
	{
		.name = "get",
		.help = "Request housekeeping",
		.usage = "[type] [interval] [count] [t0] [path]",
		.handler = cmd_hk_get,
	},
#if ENABLE_PARSER
	{
		.name = "load",
		.help = "Load beacons from file",
		.usage = "filename [offset] [count]",
		.handler = cmd_hk_load,
	},
#endif
};

command_t __root_command hk_root_command[] = {
	{
		.name = "hk",
		.help = "Housekeeping",
		.chain = INIT_CHAIN(hk_commands),
	},
};

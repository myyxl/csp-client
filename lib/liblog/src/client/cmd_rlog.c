/**
 * GomSpace Remote Log System
 *
 * @author Kristian Bay
 * Copyright 2015 GomSpace ApS. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <util/clock.h>
#include <util/log.h>

#include <command/command.h>
#include <csp/csp_endian.h>
#include <csp/csp.h>

#include <log/log.h>
#include <log/rlog_types.h>

#define MAX_RLOG_GROUPS   32
#define RLOG_DEFAULT_PORT 11

static int rlog_node = -1;
static int rlog_port = RLOG_DEFAULT_PORT;;
static char rlog_groups[MAX_RLOG_GROUPS][16] = {{0}};

static uint32_t rlog_query_level_mask = 0;
static uint32_t rlog_query_group_mask = 0;
static uint32_t rlog_no_groups = 0;
static timestamp_t rlog_latest_log_ts = {0};

int cmd_rlog_list(struct command_context *ctx) {
	if (ctx->argc < 2)
		return CMD_ERROR_SYNTAX;

	/* CSP node address */
	rlog_node = atoi(ctx->argv[1]);
	if ((rlog_node < 0) || (rlog_node > 31))
		return CMD_ERROR_SYNTAX;

	/* CSP Port */
	rlog_port = RLOG_DEFAULT_PORT;
	if (ctx->argc > 2) {
		rlog_port = atoi(ctx->argv[2]);
		if ((rlog_port < 0) || (rlog_port > 63))
			return CMD_ERROR_SYNTAX;
	}

	rlog_request req;
	memset(&req, 0, sizeof(rlog_request));
	req.action = RLOG_LIST;

	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, rlog_node, rlog_port, 10000, CSP_O_CRC32);
	if (conn == NULL)
		return CMD_ERROR_FAIL;

	/* Allocate outgoing buffer */
	csp_packet_t * packet = csp_buffer_get(sizeof(rlog_request));
	if (packet == NULL) {
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	/* Copy data to send */
	packet->length = sizeof(rlog_request);
	memcpy(packet->data, &req, packet->length);

	/* Send packet */
	if (!csp_send(conn, packet, 0)) {
		csp_buffer_free(packet);
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	/* Receive remote log groups list */
	packet = csp_read(conn, 10000);
	if (packet == NULL) {
		printf("No reply\r\n");
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	rlog_reply * reply = (rlog_reply *)packet->data;
	printf("RLOG_LIST: action:%u status:%u no_elem:%u\r\n", reply->action, reply->status, reply->no_elem);
	uint32_t offset = 0;
	log_group_entry_t group_entry;
	log_group_entry_t * group_entry_p;
	if (reply->no_elem > MAX_RLOG_GROUPS) {
		printf("Error: Too many remote log groups\r\n");
		reply->no_elem = MAX_RLOG_GROUPS;
	}
	rlog_no_groups = reply->no_elem;
	rlog_query_level_mask = 0x0000001F;
	rlog_query_group_mask = 0;
	for (int i = 0; i < reply->no_elem; i++) {
		group_entry_p = (log_group_entry_t *)&reply->data[offset];
		/* Handle alignment and endianess */
		memcpy(&group_entry, group_entry_p, sizeof(group_entry));
		uint8_t pads = (group_entry.len > 10) ? 0 : (10 - group_entry.len);
		printf("%2u %.*s %.*s %c%c%c%c%c %c%c%c%c%c\r\n", group_entry.idx, group_entry.len, group_entry_p->name, pads, "         ",
		       /* Print masks */
		       (group_entry.print_mask & LOG_ERROR_MASK) ? 'E' : '.',
		       (group_entry.print_mask & LOG_WARNING_MASK)  ? 'W' : '.',
		       (group_entry.print_mask & LOG_INFO_MASK) ? 'I' : '.',
		       (group_entry.print_mask & LOG_DEBUG_MASK) ? 'D' : '.',
		       (group_entry.print_mask & LOG_TRACE_MASK) ? 'T' : '.',
		       /* Store masks */
		       (group_entry.store_mask & LOG_ERROR_MASK) ? 'E' : '.',
		       (group_entry.store_mask & LOG_WARNING_MASK) ? 'W' : '.',
		       (group_entry.store_mask & LOG_INFO_MASK) ? 'I' : '.',
		       (group_entry.store_mask & LOG_DEBUG_MASK) ? 'D' : '.',
		       (group_entry.store_mask & LOG_TRACE_MASK) ? 'T' : '.');
		uint8_t len = (group_entry.len > 15) ? 15 : group_entry.len;
		strncpy(rlog_groups[group_entry.idx], group_entry_p->name, len);
		rlog_groups[group_entry.idx][len] = 0;
		rlog_query_group_mask |= (1 << group_entry.idx);
		offset += sizeof(log_group_entry_t) + group_entry.len;
	}
	clock_get_time(&rlog_latest_log_ts);
	csp_buffer_free(packet);
	csp_close(conn);

	return CMD_ERROR_NONE;
}

static int rlog_group_index(char *name) {
	for (int i = 0; i < rlog_no_groups; i++) {
		if (!strcmp(rlog_groups[i], name)) {
			return i;
		}
	}

	return -1;
}

static int string_to_mask(char *str)
{
	int mask = -1;

	if (str[0] == 't') {
		mask = (LOG_ERROR_MASK | LOG_WARNING_MASK | LOG_INFO_MASK | LOG_DEBUG_MASK | LOG_TRACE_MASK);
	} else if (str[0] == 'd') {
		mask = (LOG_ERROR_MASK | LOG_WARNING_MASK | LOG_INFO_MASK | LOG_DEBUG_MASK);
	} else if (str[0] == 'i') {
		mask = (LOG_ERROR_MASK | LOG_WARNING_MASK | LOG_INFO_MASK);
	} else if (str[0] == 'w') {
		mask = (LOG_ERROR_MASK | LOG_WARNING_MASK);
	} else if (str[0] == 'e') {
		mask = (LOG_ERROR_MASK);
	} else if (str[0] == 's') {
		mask = LOG_DEFAULT_MASK;
	} else if (str[0] == 'a') {
		mask = LOG_ALL_MASK;
	} else if (str[0] == 'n') {
		mask = 0;
	}

	return mask;
}

int cmd_rlog_set_mask(struct command_context *ctx) {
	if (rlog_node < 0) {
		printf("You must run 'rlog list' to initialize remote log to a node\r\n");
		return CMD_ERROR_FAIL;
	}

	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	uint8_t action;
	if (strcmp(ctx->argv[0], "print") == 0)
		action = RLOG_PRINT;
	else if (strcmp(ctx->argv[0], "store") == 0)
		action = RLOG_STORE;
	else
		return CMD_ERROR_SYNTAX;
	int group_idx = rlog_group_index(ctx->argv[1]);
	if (group_idx < 0)
		return CMD_ERROR_SYNTAX;
	int mask = string_to_mask(ctx->argv[2]);
	if (mask < 0)
		return CMD_ERROR_SYNTAX;

	rlog_request req;
	memset(&req, 0, sizeof(rlog_request));
	req.action = action;
	req.level_mask = csp_hton32(mask);
	req.group_mask = csp_hton32((1 << group_idx));

	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, rlog_node, rlog_port, 10000, CSP_O_CRC32);
	if (conn == NULL)
		return CMD_ERROR_FAIL;

	/* Allocate outgoing buffer */
	csp_packet_t * packet = csp_buffer_get(sizeof(rlog_request));
	if (packet == NULL) {
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	/* Copy data to send */
	packet->length = sizeof(rlog_request);
	memcpy(packet->data, &req, packet->length);

	/* Send packet */
	if (!csp_send(conn, packet, 0)) {
		csp_buffer_free(packet);
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	/* Receive remote log groups list */
	packet = csp_read(conn, 10000);
	if (packet == NULL) {
		printf("No reply\r\n");
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	rlog_reply * reply = (rlog_reply *)packet->data;
	printf("%s: action:%u status:%u no_elem:%u\r\n", (action == RLOG_PRINT) ? "RLOG_PRINT" : "RLOG_STORE", reply->action, reply->status, reply->no_elem);
	return CMD_ERROR_NONE;
}

int cmd_rlog_hist(struct command_context *ctx) {

	if (rlog_node < 0) {
		printf("You must run 'rlog list' to initialize remote log to a node\r\n");
		return CMD_ERROR_FAIL;
	}

	if (ctx->argc < 2)
		return CMD_ERROR_SYNTAX;

	rlog_request req;
	memset(&req, 0, sizeof(rlog_request));
	if (ctx->argc >= 2) {
		if (ctx->argv[1][0] == '-') {
			req.line_count = 0;
			req.csp_packets = (uint16_t)atoi(&ctx->argv[1][1]);
		} else {
			req.line_count = (uint16_t)atoi(ctx->argv[1]);
			req.csp_packets = 0;
		}
	}
	if (ctx->argc >= 3) {
		if (ctx->argv[2][0] == '-') {
			char * pch = strtok(&ctx->argv[2][1], ".,");
			if (pch != NULL) {
				req.ts_sec = (uint32_t)atoi(pch);
				req.ts_nsec = 0;
				pch = strtok(NULL, ".,");
				if (pch != NULL) {
					char nsec_str[10];
					strncpy(nsec_str, pch, 9);
					while (strlen(nsec_str) < 9)
						strcat(nsec_str, "0");
					req.ts_nsec = (uint32_t)atoi(nsec_str);
					if (req.ts_nsec >= NSEC_SEC) {
						req.ts_nsec = 0;
					}
				}
				/* if -1 was specified, use last known log timestamp as timestamp filter value */
				if ((req.ts_sec == 1) && (req.ts_nsec == 0)) {
					req.ts_sec = rlog_latest_log_ts.tv_sec;
					req.ts_nsec = rlog_latest_log_ts.tv_nsec;
				}
			}
			req.line_offset = 0;
		} else {
			req.line_offset = (uint16_t)atoi(ctx->argv[2]);
			req.ts_sec = 0;
			req.ts_nsec = 0;
		}
	}
	if (ctx->argc >= 4) {
		req.cap_len = atoi(ctx->argv[3]);
	}

	req.action = RLOG_HIST;
	req.level_mask = csp_hton32(rlog_query_level_mask);
	req.group_mask = csp_hton32(rlog_query_group_mask);
	req.ts_sec = csp_hton32(req.ts_sec);
	req.ts_nsec = csp_hton32(req.ts_nsec);
	req.line_count = csp_hton16(req.line_count);
	req.line_offset = csp_hton16(req.line_offset);
	req.cap_len = csp_hton16(req.cap_len);
	req.csp_packets = csp_hton16(req.csp_packets);

	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, rlog_node, rlog_port, 10000, CSP_O_CRC32);
	if (conn == NULL)
		return CMD_ERROR_FAIL;

	/* Allocate outgoing buffer */
	csp_packet_t * packet = csp_buffer_get(sizeof(rlog_request));
	if (packet == NULL) {
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	/* Copy data to send */
	packet->length = sizeof(rlog_request);
	memcpy(packet->data, &req, packet->length);

	/* Send packet */
	if (!csp_send(conn, packet, 0)) {
		csp_buffer_free(packet);
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	uint8_t log_cnt = 0;
	while (1) {
		/* Read reply */
		packet = csp_read(conn, 10000);
		if (packet == NULL) {
			printf("No reply\r\n");
			csp_close(conn);
			break;
		}

		rlog_reply * reply = (rlog_reply *)packet->data;
		printf("RLOG_HIST (total msg sz: %u): action:%u status:%u no_elem:%u total_elem:%u\r\n",
				packet->length, reply->action, reply->status, reply->no_elem, reply->total_elem);
		log_store_entry_t log_entry;
		log_store_entry_t * log_entry_p;
		uint32_t offset = 0;
		for (int i = 0; i < reply->no_elem; i++) {
			log_entry_p = (log_store_entry_t *)&reply->data[offset];
			/* Handle alignment and endianess */
			memcpy(&log_entry, log_entry_p, sizeof(log_entry));
			log_entry.sec = csp_ntoh32(log_entry.sec);
			log_entry.nsec = csp_ntoh32(log_entry.nsec);
			if (i == 0) {
				/* Update newest log line timestamp */
				rlog_latest_log_ts.tv_sec = log_entry.sec;
				rlog_latest_log_ts.tv_nsec = log_entry.nsec;
			}

			/* Check if any lines lost */
			if (log_cnt != (i + reply->idx_first)) {
				printf("\E[1;31m ... lost %u lines (got %u, expected %u) ... \E[0m\r\n", (i + reply->idx_first) - log_cnt, i + reply->idx_first, log_cnt);
			}
			log_cnt = i + reply->idx_first + 1;

			/* Start color */
			switch (log_entry.level) {
				case LOG_TRACE: printf("\E[0;35m"); break;	/* Magenta */
				case LOG_DEBUG: printf("\E[0;34m"); break;	/* Blue */
				case LOG_INFO: printf("\E[0;32m"); break;	/* Green */
				case LOG_WARNING: printf("\E[0;33m"); break;	/* Yellow */
				case LOG_ERROR: printf("\E[1;31m"); break;	/* Red */
				default: break;
			}

			/* Log message */
			printf("%2d: %04"PRIu32".%06"PRIu32" %.10s %.*s",
					i + reply->idx_first, log_entry.sec, log_entry.nsec / 1000, rlog_groups[log_entry.group], log_entry.len, log_entry_p->data);

			/* End color */
			printf("\E[0m\r\n");

			offset += sizeof(log_store_entry_t) + log_entry.len;
		}
		uint8_t no_elem = reply->no_elem;
		uint8_t total_elem = reply->total_elem;
		csp_buffer_free(packet);
		if ((no_elem == 0) || (total_elem != 0)) {
			if (no_elem == 0)
				printf("Empty log history\r\n");
			break;
		}
	}
	csp_close(conn);

	return CMD_ERROR_NONE;
}

static void print_rlog_query(void) {
	printf("Log query settings:\r\n");
	printf("  level (EWIDT)  : %c%c%c%c%c\r\n",
       (rlog_query_level_mask & LOG_ERROR_MASK) ? 'E' : '.',
       (rlog_query_level_mask & LOG_WARNING_MASK)  ? 'W' : '.',
       (rlog_query_level_mask & LOG_INFO_MASK) ? 'I' : '.',
       (rlog_query_level_mask & LOG_DEBUG_MASK) ? 'D' : '.',
       (rlog_query_level_mask & LOG_TRACE_MASK) ? 'T' : '.');
	printf("  groups enabled :");
	for (int i = 0; i < rlog_no_groups; i++) {
		if (rlog_query_group_mask & (1 << i)) {
			printf(" %s", rlog_groups[i]);
		}
	}
	printf("\r\n");
	printf("  groups disabled:");
	for (int i = 0; i < rlog_no_groups; i++) {
		if (!(rlog_query_group_mask & (1 << i))) {
			printf(" %s", rlog_groups[i]);
		}
	}
	printf("\r\n");
}

int cmd_rlog_query(struct command_context *ctx) {
	if (ctx->argc == 1) {
		print_rlog_query();
	} else if (ctx->argc >= 3) {
		if (strcmp(ctx->argv[1], "level") == 0) {
			char * pch = strtok (ctx->argv[2], ",");
			while (pch != NULL)
			{
				if (strlen(pch) > 0) {
					uint8_t action = '+';
					if ((pch[0] == '-') || (pch[0] == '+')) {
						action = pch[0];
						pch++;
					}
					uint8_t level = pch[0];
					if (level > 0) {
						int idx = -1;
						switch (level) {
						case 'e': idx = LOG_ERROR; break;
						case 'w': idx = LOG_WARNING; break;
						case 'i': idx = LOG_INFO; break;
						case 'd': idx = LOG_DEBUG; break;
						case 't': idx = LOG_TRACE; break;
						default: idx = -1; break;
						}
						if (idx >= 0) {
							if (action == '+') {
								rlog_query_level_mask |= (1 << idx);
							} else {
								rlog_query_level_mask &= ~(1 << idx);
							}
						}
					} else {
						/* Enable/disable all */
						if (action == '+') {
							rlog_query_level_mask = 0x1F;
						} else {
							rlog_query_level_mask = 0;
						}
					}
				}
				pch = strtok(NULL, ",");
			}
			print_rlog_query();
		} else if (strcmp(ctx->argv[1], "group") == 0) {
			char * pch = strtok (ctx->argv[2], ",");
			while (pch != NULL)
			{
				if (strlen(pch) > 0) {
					uint8_t action = '+';
					if ((pch[0] == '-') || (pch[0] == '+')) {
						action = pch[0];
						pch++;
					}
					if (pch[0] > 0) {
						int idx = rlog_group_index(pch);
						if (idx >= 0) {
							if (action == '+') {
								rlog_query_group_mask |= (1 << idx);
							} else {
								rlog_query_group_mask &= ~(1 << idx);
							}
						}
					} else {
						/* Enable/disable all */
						if (action == '+') {
							rlog_query_group_mask = ((1 << rlog_no_groups) - 1);
						} else {
							rlog_query_group_mask = 0;
						}
					}
				}
				pch = strtok(NULL, ",");
			}
			print_rlog_query();
		} else {
			return CMD_ERROR_SYNTAX;
		}
	} else {
		return CMD_ERROR_SYNTAX;
	}

	return CMD_ERROR_NONE;
}


command_t rlog_commands[] = {
	{
		.name = "list",
		.help = "Init remote log node and List remote log groups",
		.usage = "<node> <port>",
		.handler = cmd_rlog_list,
	},{
		.name = "print",
		.help = "Remote log print settings",
		.usage = "<group> <error|warn|info|debug|trace|none>",
		.handler = cmd_rlog_set_mask,
	},{
		.name = "store",
		.help = "Remote log store settings",
		.usage = "<node> <group> <error|warn|info|debug|trace|none>",
		.handler = cmd_rlog_set_mask,
	},{
		.name = "hist",
		.help = "Get remote log history",
		.usage = "<count> <offset> <cap_len>",
		.handler = cmd_rlog_hist,
	},{
		.name = "query",
		.help = "Set remote log query filter (query alone to display current query)",
		.usage = "<level> <+/-level> | <group> <+/-group>",
		.handler = cmd_rlog_query,
	}
};

command_t __root_command rlog_root_command[] = {
	{
		.name = "rlog",
		.help = "log: Remote Log System",
		.chain = INIT_CHAIN(rlog_commands),
	},
};

void cmd_rlog_setup(void) {
	command_register(rlog_root_command);
}

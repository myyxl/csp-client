/**
 * GomSpace Parameter System
 *
 * @author Johan De Claville Christiansen
 * Copyright 2014 GomSpace ApS. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <util/log.h>

#include <command/command.h>
#include <csp/csp_endian.h>
#include <csp/csp.h>

#include <param/param_serializer.h>
#include <param/param_string.h>
#include <param/param_fletcher.h>

#include <param/param_types.h>
#include <param/rparam_client.h>

/** Remote param system setup */
static param_index_t rparam_mem_i = {0};
static uint16_t rparam_checksum = 0;
static int rparam_node = 0;
static int rparam_port = 7;
static int rparam_autosend = 1;

int cmd_rparam_set_autosend(struct command_context *ctx) {
	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;
	rparam_autosend = atoi(ctx->argv[1]);
	printf("rparam autosend set to %u\r\n", rparam_autosend);
	return CMD_ERROR_NONE;
}

int cmd_rparam_list(struct command_context *ctx) {
	if (rparam_mem_i.table == NULL) {
		printf("Run download or init to setup table\n");
		return CMD_ERROR_FAIL;
	}

	param_list(&rparam_mem_i, 1);
	return CMD_ERROR_NONE;
}

int cmd_rparam_init_from_local_file(struct command_context *ctx) {
	if ((ctx->argc != 2) && (ctx->argc != 3))
		return CMD_ERROR_SYNTAX;
	rparam_node =  atoi(ctx->argv[1]);
	if (ctx->argc >= 3)
		rparam_mem_i.mem_id = atoi(ctx->argv[2]);

	rparam_set_quiet(RPARAM_NOT_QUIET);

	char fname[16];
	sprintf(fname, "param-%u-%u.bin", rparam_node, rparam_mem_i.mem_id);
	printf("Loading table specification from %s\r\n", fname);
	int rc = rparam_load_table_spec_from_file(&fname[0], &rparam_mem_i, &rparam_checksum);
	if (rc == CMD_ERROR_NONE)
		param_list(&rparam_mem_i, 0);

	return rc;
}

int cmd_rparam_init_from_remote_node(struct command_context *ctx) {
	if ((ctx->argc != 2) && (ctx->argc != 3))
		return CMD_ERROR_SYNTAX;
	rparam_node = atoi(ctx->argv[1]);
	if (ctx->argc >= 3)
		rparam_mem_i.mem_id = atoi(ctx->argv[2]);

	rparam_set_quiet(RPARAM_NOT_QUIET);

	printf("Downloading table specification %u from node %u on port %u\r\n", rparam_mem_i.mem_id, rparam_node, rparam_port);

	char fname[16];
	sprintf(fname, "param-%u-%u.bin", rparam_node, rparam_mem_i.mem_id);

	int rc = rparam_download_table_spec_from_remote_and_save_to_file(&fname[0], rparam_node, rparam_port, &rparam_mem_i, &rparam_checksum);
	if (rc == CMD_ERROR_NONE)
		param_list(&rparam_mem_i, 0);

	return rc;
}

int cmd_rparam_send(struct command_context *ctx) {

	int override_crc = 0;
	if (ctx->argc == 2) {
		override_crc = atoi(ctx->argv[1]);
	} else if (ctx->argc > 2) {
		return CMD_ERROR_SYNTAX;
	}

	if (rparam_mem_i.table == NULL) {
		printf("Run download or init to setup table\n");
		return CMD_ERROR_FAIL;
	}

	uint16_t crc_to_use = rparam_checksum;
	if (override_crc)
		crc_to_use = 0x0BB0;

	int rc = rparam_query_send(&rparam_mem_i, rparam_node, rparam_port, crc_to_use);
	if (rc == CMD_ERROR_NONE)
		rparam_query_reset();
	return rc;
}

int cmd_rparam_get(struct command_context *ctx) {

	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;

	if (rparam_mem_i.table == NULL) {
		printf("Run download or init to setup table\n");
		return CMD_ERROR_FAIL;
	}

	int rc = rparam_query_get(&rparam_mem_i, ctx->argv[1]);

	/* Autosend */
	if (rparam_autosend && rc == CMD_ERROR_NONE) {
		rc = rparam_query_send(&rparam_mem_i, rparam_node, rparam_port, rparam_checksum);
		if (rc == CMD_ERROR_NONE)
			rparam_query_reset();
	}

	return rc;
}

int cmd_rparam_getall(struct command_context *ctx) {
	printf("Downloading table contents for table %i from node %i on port %i\n", rparam_mem_i.mem_id, rparam_node, rparam_port);
	int rc = rparam_get_full_table(&rparam_mem_i, rparam_node, rparam_port, rparam_mem_i.mem_id, 10000);
	if (rc != CMD_ERROR_NONE)
		return rc;

	param_list(&rparam_mem_i, 1);
	return CMD_ERROR_NONE;
}

int cmd_rparam_set(struct command_context *ctx) {

	if (ctx->argc < 3)
		return CMD_ERROR_SYNTAX;

	if (rparam_mem_i.table == NULL) {
		printf("Run download or init to setup table\n");
		return CMD_ERROR_FAIL;
	}

	int rc = rparam_query_set(&rparam_mem_i, ctx->argv[1], &ctx->argv[2], ctx->argc - 2);

	/* Autosend */
	if (rparam_autosend && rc == CMD_ERROR_NONE) {
		rc = rparam_query_send(&rparam_mem_i, rparam_node, rparam_port, rparam_checksum);
		if (rc == CMD_ERROR_NONE)
			rparam_query_reset();
	}

	return rc;
}

int cmd_rparam_print(struct command_context *ctx) {

	if (rparam_mem_i.table == NULL) {
		printf("Run download or init to setup table\n");
		return CMD_ERROR_FAIL;
	}

	rparam_query_print(&rparam_mem_i);
	return CMD_ERROR_NONE;
}

int cmd_rparam_copy(struct command_context *ctx) {
	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;
	int from = atoi(ctx->argv[1]);
	int to = atoi(ctx->argv[2]);

	if (rparam_query_is_set())
		printf("You have a query set, use the send command before copy for it to take effect.");

	if (rparam_copy(rparam_node, rparam_port, 10000, from, to))
		return CMD_ERROR_NONE;
	return CMD_ERROR_FAIL;
}

int cmd_rparam_load(struct command_context *ctx) {
	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;
	int from = atoi(ctx->argv[1]);
	int to = atoi(ctx->argv[2]);

	if (rparam_load(rparam_node, rparam_port, 10000, from, to))
		return CMD_ERROR_NONE;
	return CMD_ERROR_FAIL;
}

int cmd_rparam_save(struct command_context *ctx) {
	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;
	int from = atoi(ctx->argv[1]);
	int to = atoi(ctx->argv[2]);

	if (rparam_query_is_set())
		printf("You have a query set, use the send command before save for it to take effect.");

	if(rparam_save(rparam_node, rparam_port, 10000, from, to))
		return CMD_ERROR_NONE;
	return CMD_ERROR_FAIL;
}

int cmd_rparam_clear(struct command_context *ctx) {
	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;
	int file_id = atoi(ctx->argv[1]);

	if (rparam_clear(rparam_node, rparam_port, 100, file_id))
		return CMD_ERROR_NONE;
	return CMD_ERROR_FAIL;
}


int cmd_rparam_reset(struct command_context *ctx) {
	rparam_query_reset();
	return CMD_ERROR_NONE;
}

command_t rparam_commands[] = {
	{
		.name = "init",
		.help = "Setup node/port, loads table spec from file",
		.usage = "<node> [mem]",
		.handler = cmd_rparam_init_from_local_file,
	},{
		.name = "download",
		.help = "Setup node/port, loads table spec from remote",
		.usage = "<node> [mem]",
		.handler = cmd_rparam_init_from_remote_node,
	},{
		.name = "list",
		.help = "Lists the table specification",
		.handler = cmd_rparam_list,
	},{
		.name = "get",
		.help = "Add a 'get' to the current query transaction",
		.usage = "<name>",
		.handler = cmd_rparam_get,
	},{
		.name = "set",
		.help = "Add a 'set' to the current query transaction",
		.usage = "<name> <value>",
		.handler = cmd_rparam_set,
	},{
		.name = "copy",
		.usage = "<from> <to>",
		.help = "Query copy",
		.handler = cmd_rparam_copy,
	},{
		.name = "load",
		.usage = "<from> <to>",
		.help = "Load from fs/fram to mem",
		.handler = cmd_rparam_load,
	},{
		.name = "save",
		.usage = "<from> <to>",
		.help = "Save from mem to fs/fram",
		.handler = cmd_rparam_save,
	},{
		.name = "clear",
		.usage = "<file_id>",
		.help = "Clear data from fs/fram",
		.handler = cmd_rparam_clear,
	},{
		.name = "print",
		.help = "Print the current query",
		.handler = cmd_rparam_print,
	},{
		.name = "reset",
		.help = "Reset the current query",
		.handler = cmd_rparam_reset,
	},{
		.name = "send",
		.usage = "[crc override]",
		.help = "Send the current query transaction",
		.handler = cmd_rparam_send,
	},{
		.name = "autosend",
		.usage = "<bool>",
		.help = "Enable/disable autosend for set and get queries",
		.handler = cmd_rparam_set_autosend,
	},{
		.name = "getall",
		.help = "Download full table contents from the remote",
		.handler = cmd_rparam_getall,
	}
};

command_t __root_command rparam_root_command[] = {
	{
		.name = "rparam",
		.help = "param: Remote Param",
		.chain = INIT_CHAIN(rparam_commands),
	},
};

void cmd_rparam_setup(void) {
	command_register(rparam_root_command);
}

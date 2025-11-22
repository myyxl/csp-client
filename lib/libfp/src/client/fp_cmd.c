/**
 * @file fp_cmd.c
 * Flight planner debugging commands.
 *
 * @author Jeppe Ledet-Pedersen
 * Copyright 2011 GomSpace ApS. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <util/console.h>
#include <util/timestamp.h>
#include <util/clock.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>

#include <fp/fp.h>
#include <fp/fp_list.h>
#include <fp/fp_timer.h>
#include <fp/fp_types.h>
#include <fp/fp_client.h>

int cmd_fp_host(struct command_context *ctx)
{
	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	uint8_t node = atoi(ctx->argv[1]);
	uint8_t port = atoi(ctx->argv[2]);

	return fp_client_host(node, port) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_load_fp(struct command_context *ctx)
{
	char *args = command_args(ctx);
	char path[100];

	if (args == NULL || sscanf(args, "%99s", path) != 1)
		return CMD_ERROR_SYNTAX;

	return fp_client_fp_load(path) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_store_fp(struct command_context *ctx)
{
	char *args = command_args(ctx);
	char path[100];

	if (args == NULL || sscanf(args, "%99s", path) != 1)
		return CMD_ERROR_SYNTAX;

	return fp_client_fp_store(path) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_flush_fp(struct command_context *ctx)
{
	return fp_client_fp_flush() < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_timer_create(struct command_context *ctx)
{
	if (ctx->argc < 4)
		return CMD_ERROR_SYNTAX;

	char *timer = ctx->argv[1];
	char *when = ctx->argv[2];
	char *cmd = ctx->argv[3];

	uint16_t repeat = 1;
	if (ctx->argc > 4)
		repeat = atoi(ctx->argv[4]);

	fp_timer_state_t s = FP_TIME_ACTIVE;
	if (ctx->argc > 5) {
		printf("formats: %s\r\n", ctx->argv[5]);
		if (!strcmp(ctx->argv[5], "active"))
			s = FP_TIME_ACTIVE;
		else if (!strcmp(ctx->argv[5], "dormant"))
			s = FP_TIME_DORMANT;
		else
			return CMD_ERROR_SYNTAX;
	}

	fp_timer_basis_t b;

	uint32_t sec;
	if (when[0] == '+') {
		b = FP_TIME_RELATIVE;
		sec = atoi(when + 1);
	} else {
		b = FP_TIME_ABSOLUTE;
		sec = atoi(when);
	}

	timestamp_t t;
	t.tv_sec = sec;
	t.tv_nsec = 0;

	return fp_client_timer_create(timer, cmd, b, s, &t, repeat) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_timer_delete(struct command_context *ctx)
{
	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;

	char *timer = ctx->argv[1];

	return fp_client_timer_delete(timer) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_timer_set_active(struct command_context *ctx)
{
	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;

	char * timer = ctx->argv[1];

	return fp_client_timer_set_active(timer) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_timer_set_all_active(struct command_context *ctx)
{
	return fp_client_timer_set_all_active() < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_timer_set_dormant(struct command_context *ctx)
{
	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;

	char * timer = ctx->argv[1];

	return fp_client_timer_set_dormant(timer) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_timer_set_repeat(struct command_context *ctx)
{
	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	char * timer = ctx->argv[1];
	uint16_t count = atoi(ctx->argv[2]);

	return fp_client_timer_set_repeat(timer, count) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_timer_set_time(struct command_context *ctx)
{
	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	char * timer = ctx->argv[1];
	char * when = ctx->argv[2];

	fp_timer_basis_t b;

	uint32_t sec;
	if (when[0] == '+') {
		b = FP_TIME_RELATIVE;
		sec = atoi(when + 1);
	} else {
		b = FP_TIME_ABSOLUTE;
		sec = atoi(when);
	}

	timestamp_t t;
	t.tv_sec = sec;
	t.tv_nsec = 0;

	return fp_client_timer_set_time(timer, b, &t) < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

int cmd_fp_timer_list(struct command_context *ctx)
{
	return fp_client_timer_list() < 0 ? CMD_ERROR_FAIL : CMD_ERROR_NONE;
}

command_t fp_subcommands[] = {
	{
		.name = "server",
		.help = "Set FP server",
		.usage = "<node> <port>",
		.handler = cmd_fp_host,
	},{
		.name = "flush",
		.help = "Flush current flight plan",
		.handler = cmd_fp_flush_fp,
	},{
		.name = "load",
		.help = "Load flight plan from file",
		.usage = "<path>",
		.handler = cmd_fp_load_fp,
	},{
		.name = "store",
		.help = "Store current flight plan to file",
		.usage = "<path>",
		.handler = cmd_fp_store_fp,
	},{
		.name = "create",
		.help = "Create new timer",
		.usage = "<name> [+]<sec> <command> [repeat] [state]",
		.handler = cmd_fp_timer_create,
	},{
		.name = "delete",
		.help = "Delete timer",
		.usage = "<timer>",
		.handler = cmd_fp_timer_delete,
	},{
		.name = "active",
		.help = "Set timer active",
		.usage = "<timer>",
		.handler = cmd_fp_timer_set_active,
	},{
		.name = "allactive",
		.help = "Set all timers active",
		.usage = "<timer>",
		.handler = cmd_fp_timer_set_all_active,
	},{
		.name = "dormant",
		.help = "Set timer dormant",
		.usage = "<timer>",
		.handler = cmd_fp_timer_set_dormant,
	},{
		.name = "repeat",
		.help = "Set timer repeat",
		.usage = "<timer> <repeat count>",
		.handler = cmd_fp_timer_set_repeat,
	},{
		.name = "time",
		.help = "Set execution time",
		.usage = "<timer> [+]<sec>",
		.handler = cmd_fp_timer_set_time,
	},{
		.name = "list",
		.help = "List timers",
		.handler = cmd_fp_timer_list,
	},
};

command_t __root_command fp_commands[] = {
	{
		.name = "fp",
		.help = "Flight planner commands",
		.chain = INIT_CHAIN(fp_subcommands),
	}
};

/**
 * @file getopt.c
 *
 * This implements optional argument parsing
 *
 * @author Jeppe Ledet-Pedersen
 * Copyright 2015 GomSpace ApS. All rights reserved.
 */

#include <gosh/getopt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <command/command.h>

/*
 * Adapted from AT&T public domain source from:
 * http://www.informatica.co.cr/unix-source-code/research/1985/1103.html
 */

int gosh_getopt(struct command_context *ctx, const char *opts)
{
	int c;
	char *cp;

	if (ctx->optsp == 1) {
		if (ctx->optind >= ctx->argc ||
		    ctx->argv[ctx->optind][0] != '-' ||
		    ctx->argv[ctx->optind][1] == '\0') {
			return EOF;
		} else if (!strcmp(ctx->argv[ctx->optind], "--")) {
			ctx->optind++;
			return EOF;
		}
	}

	ctx->optopt = c = ctx->argv[ctx->optind][ctx->optsp];
	if (c == ':' || (cp = strchr(opts, c)) == NULL) {
		printf("illegal option -- %c\r\n", c);
		if (ctx->argv[ctx->optind][++ctx->optsp] == '\0') {
			ctx->optind++;
			ctx->optsp = 1;
		}
		return '?';
	}

	if (*++cp == ':') {
		if (ctx->argv[ctx->optind][ctx->optsp+1] != '\0') {
			ctx->optarg = &ctx->argv[ctx->optind++][ctx->optsp+1];
		} else if(++ctx->optind >= ctx->argc) {
			printf("option requires an argument -- %c\r\n", c);
			ctx->optsp = 1;
			return '?';
		} else {
			ctx->optarg = ctx->argv[ctx->optind++];
		}
		ctx->optsp = 1;
	} else {
		if (ctx->argv[ctx->optind][++ctx->optsp] == '\0') {
			ctx->optsp = 1;
			ctx->optind++;
		}
		ctx->optarg = NULL;
	}

	return c;
}

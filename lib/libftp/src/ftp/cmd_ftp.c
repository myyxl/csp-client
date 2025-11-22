/**
 * @file cmd_ftp.c
 * FTP Client Command interface
 *
 * @author Johan De Claville Christiansen
 * Copyright 2012 GomSpace ApS. All rights reserved.
 */

#include <inttypes.h>
#include <libgen.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <ftp/ftp_client.h>
#include <ftp/ftp_types.h>

#include <util/console.h>
#include <util/timestamp.h>
#include <util/log.h>
#include <util/bytesize.h>

#if 0
int ftp_upload(uint8_t host, uint8_t port, const char * path, uint8_t type, int chunk_size, uint32_t addr, const char * remote_path, uint32_t * size, uint32_t * checksum);
int ftp_status_request(void);
int ftp_data(int count);
#endif

/* Settings */
static unsigned int ftp_host = 1;
static unsigned int ftp_port = 9;
static unsigned int ftp_chunk_size = 185;
static unsigned int ftp_backend = 3; // Use file backend as standard

/* State variables */
static uint32_t ftp_size;

int cmd_ftp_set_backend(struct command_context *ctx) {

	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;

	ftp_backend = atoi(ctx->argv[1]);

	return CMD_ERROR_NONE;
}

int cmd_ftp_set_host_port(struct command_context *ctx) {

	if (ctx->argc < 2)
		return CMD_ERROR_SYNTAX;

	ftp_host = atoi(ctx->argv[1]);

	if (ctx->argc > 2)
		ftp_port = atoi(ctx->argv[2]);
	
	if (ctx->argc > 3)
		ftp_chunk_size = atoi(ctx->argv[3]);

	return CMD_ERROR_NONE;

}

/* Some versions of newlib do not have basename
 * (looking at you, Atmel)
 */
static char *ftp_basename(char *path)
{
	char *base = strrchr(path, '/');
	return base ? base+1 : path;
}

int cmd_ftp_download_file(struct command_context *ctx) {

	if ((ctx->argc < 2) || (ctx->argc > 3))
		return CMD_ERROR_SYNTAX;

	char * remote_path = ctx->argv[1];

	if (strlen(remote_path) > FTP_PATH_LENGTH)
		return CMD_ERROR_SYNTAX;

	char * local_path;
	if (ctx->argc == 3)
		local_path = ctx->argv[2];
	else
		local_path = ftp_basename(remote_path);

	int status = ftp_download(ftp_host, ftp_port, local_path, ftp_backend, ftp_chunk_size, 0, 0, remote_path, &ftp_size);

	if (status != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}

	if (ftp_status_reply() != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}
	if (ftp_crc() != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}

	ftp_done(1);

	return CMD_ERROR_NONE;

}


int cmd_ftp_upload_file(struct command_context *ctx) {

	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	char * remote_path = ctx->argv[2];
	char * local_path = ctx->argv[1];

	if (strlen(remote_path) > FTP_PATH_LENGTH || strlen(remote_path) < 3)
		return CMD_ERROR_SYNTAX;
	if (strlen(local_path) < 3)
		return CMD_ERROR_SYNTAX;


	int status = ftp_upload(ftp_host, ftp_port, local_path, ftp_backend, ftp_chunk_size,  0, remote_path, &ftp_size, NULL);
	if (status != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}
	if (ftp_status_request() != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}
	if (ftp_data(0) != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}
	if (ftp_crc() != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}

	ftp_done(0);

	return CMD_ERROR_NONE;

}

int cmd_ftp_download_mem(struct command_context *ctx) {

	if (ctx->argc != 4)
		return CMD_ERROR_SYNTAX;

	char * local_path = ctx->argv[1];
	int memaddr;
	int memsize;

	if (strlen(local_path) > FTP_PATH_LENGTH || strlen(local_path) < 3)
		return CMD_ERROR_SYNTAX;

	if (sscanf(ctx->argv[2], "%x", &memaddr) != 1)
		return CMD_ERROR_SYNTAX;

	if (sscanf(ctx->argv[3], "%u", &memsize) != 1)
		return CMD_ERROR_SYNTAX;

	printf("Downloading from addr 0x%X size 0x%X to file %s\r\n", memaddr, memsize, local_path);

	int status = ftp_download(ftp_host, ftp_port, local_path, 0, ftp_chunk_size, memaddr, memsize, NULL, &ftp_size);

	if (status != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}

	if (ftp_status_reply() != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}
	if (ftp_crc() != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}

	ftp_done(1);

	return CMD_ERROR_NONE;

}

int cmd_ftp_upload_mem(struct command_context *ctx) {

	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	char * local_path = ctx->argv[1];
	int memaddr;

	if (strlen(local_path) > FTP_PATH_LENGTH || strlen(local_path) < 3)
		return CMD_ERROR_SYNTAX;

	if (sscanf(ctx->argv[2], "%x", &memaddr) != 1)
		return CMD_ERROR_SYNTAX;

	printf("Uploading from %s to 0x%X\r\n", local_path, memaddr);

	int status = ftp_upload(ftp_host, ftp_port, local_path, 0, ftp_chunk_size, memaddr, "", &ftp_size, NULL);
	if (status != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}
	if (ftp_status_request() != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}
	if (ftp_data(0) != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}
	if (ftp_crc() != 0) {
		ftp_done(0);
		return CMD_ERROR_FAIL;
	}

	ftp_done(1);

	return CMD_ERROR_NONE;

}

int cmd_ftp_download_run(struct command_context *ctx) {
	if (ftp_status_reply() != 0)
		return CMD_ERROR_FAIL;
	return CMD_ERROR_NONE;
}

int cmd_ftp_crc(struct command_context *ctx) {
	if (ftp_crc() != 0)
		return CMD_ERROR_FAIL;
	return CMD_ERROR_NONE;
}

int cmd_ftp_zip(struct command_context *ctx) {
	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	uint8_t action;
	if (!strcmp(ctx->argv[0], "zip")) {
		action = FTP_ZIP;
	} else {
		action = FTP_UNZIP;
	}
	char * src_path = ctx->argv[1];
	if (strlen(src_path) > FTP_PATH_LENGTH || strlen(src_path) < 3)
		return CMD_ERROR_SYNTAX;

	char * dest_path = ctx->argv[2];
	if (strlen(dest_path) > FTP_PATH_LENGTH || strlen(dest_path) < 3)
		return CMD_ERROR_SYNTAX;

	if (ftp_zip(ftp_host, ftp_port, ftp_backend, src_path, dest_path, action) != 0)
		return CMD_ERROR_FAIL;
	return CMD_ERROR_NONE;
}

int cmd_ftp_done(struct command_context *ctx) {
	if (ftp_done(1) != 0)
		return CMD_ERROR_FAIL;
	return CMD_ERROR_NONE;
}

static int ftp_ls_print_callback(uint16_t entries, ftp_list_entry_t *entry, void *data)
{
	char bytebuf[25];

	bytesize(bytebuf, 25, entry->size);

	printf("%6s %s%s\r\n", bytebuf, entry->path, entry->type ? "/" : "");

	return 0;
}

int cmd_ftp_ls(struct command_context *ctx) {

	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;

	char * remote_path = ctx->argv[1];

	if (strlen(remote_path) > FTP_PATH_LENGTH)
		return CMD_ERROR_SYNTAX;

	if (ftp_list(ftp_host, ftp_port, ftp_backend, remote_path, ftp_ls_print_callback, NULL) != 0)
		return CMD_ERROR_FAIL;
	return CMD_ERROR_NONE;

}

int cmd_ftp_remove(struct command_context *ctx) {

	if (ctx->argc != 2)
		return CMD_ERROR_SYNTAX;

	char * remote_path = ctx->argv[1];

	if (strlen(remote_path) > FTP_PATH_LENGTH)
		return CMD_ERROR_SYNTAX;

	if (ftp_remove(ftp_host, ftp_port, ftp_backend, remote_path) != 0)
		return CMD_ERROR_FAIL;
	return CMD_ERROR_NONE;

}

int cmd_ftp_mkfs(struct command_context *ctx) {

	char *path;
	bool force = false;

	if (ctx->argc < 2)
		return CMD_ERROR_SYNTAX;

	if (ctx->argc > 2) {
		if (!strcmp(ctx->argv[2], "1") || !strcmp(ctx->argv[2], "yes") || !strcmp(ctx->argv[2], "true"))
			force = true;
		else if (!strcmp(ctx->argv[2], "0") || !strcmp(ctx->argv[2], "no") || !strcmp(ctx->argv[2], "false"))
			force = false;
		else
			return CMD_ERROR_SYNTAX;
	}

	path = ctx->argv[1];

	if (ftp_mkfs(ftp_host, ftp_port, ftp_backend, path, force) != 0)
		return CMD_ERROR_FAIL;

	return CMD_ERROR_NONE;

}

int cmd_ftp_move(struct command_context *ctx) {

	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	char * from_path = ctx->argv[1];
	if (strlen(from_path) > FTP_PATH_LENGTH || strlen(from_path) < 3)
		return CMD_ERROR_SYNTAX;

	char * to_path = ctx->argv[2];
	if (strlen(to_path) > FTP_PATH_LENGTH || strlen(to_path) < 3)
		return CMD_ERROR_SYNTAX;

	if (ftp_move(ftp_host, ftp_port, ftp_backend, from_path, to_path) != 0)
		return CMD_ERROR_FAIL;
	return CMD_ERROR_NONE;

}

int cmd_ftp_copy(struct command_context *ctx) {

	if (ctx->argc != 3)
		return CMD_ERROR_SYNTAX;

	char * from_path = ctx->argv[1];
	if (strlen(from_path) > FTP_PATH_LENGTH || strlen(from_path) < 3)
		return CMD_ERROR_SYNTAX;

	char * to_path = ctx->argv[2];
	if (strlen(to_path) > FTP_PATH_LENGTH || strlen(to_path) < 3)
		return CMD_ERROR_SYNTAX;

	if (ftp_copy(ftp_host, ftp_port, ftp_backend, from_path, to_path) != 0)
		return CMD_ERROR_FAIL;
	return CMD_ERROR_NONE;

}

int cmd_ftp_timeout(struct command_context *ctx) {
	if (ctx->argc > 2)
		return CMD_ERROR_SYNTAX;

	if (ctx->argc == 2) {
		int timeout = atoi(ctx->argv[1]);
		ftp_set_timeout(timeout);
	}
	printf("ftp timeout is %d msec\r\n", ftp_get_timeout());
	return CMD_ERROR_NONE;
}

command_t __sub_command ftp_subcommands[] = {
	{
		.name = "ls",
		.help = "list files",
		.usage = "<path>",
		.handler = cmd_ftp_ls,
	},{
		.name = "rm",
		.help = "rm files",
		.usage = "<path>",
		.handler = cmd_ftp_remove,
	},{
		.name = "mkfs",
		.help = "make file system",
		.usage = "<path> [force]",
		.handler = cmd_ftp_mkfs,
	},{
		.name = "mv",
		.help = "move files",
		.usage = "<from> <to>",
		.handler = cmd_ftp_move,
	},{
		.name = "cp",
		.help = "copy files",
		.usage = "<from> <to>",
		.handler = cmd_ftp_copy,
	},{
		.name = "zip",
		.help = "zip file",
		.usage = "<path>",
		.handler = cmd_ftp_zip,
	},{
		.name = "unzip",
		.help = "unzip file",
		.usage = "<path>",
		.handler = cmd_ftp_zip,
	},{
		.name = "server",
		.help = "set host and port",
		.usage = "<server> [port] [chunk size]",
		.handler = cmd_ftp_set_host_port,
	},{
		.name = "backend",
		.help = "Set filesystem backend",
		.usage = "<backend, 0=mem, 1=fat, 2=vfs, 3=file>",
		.handler = cmd_ftp_set_backend,
	},{
		.name = "upload_file",
		.help = "Upload file",
		.usage = "<local filename> <remote filename> ",
		.handler = cmd_ftp_upload_file,
	},{
		.name = "download_file",
		.help = "Download file",
		.usage = "<remote filename> [local filename]",
		.handler = cmd_ftp_download_file,
	},{
		.name = "upload_mem",
		.help = "Upload to memory",
		.usage = "<local_file> <remote_addr>",
		.handler = cmd_ftp_upload_mem,
	},{
		.name = "download_mem",
		.help = "Download memory",
		.usage = "<local_file> <remote_addr> <length>",
		.handler = cmd_ftp_download_mem,
	},{
		.name = "timeout",
		.help = "Get or Set general ftp timeout",
		.usage = "[<timeout ms>]",
		.handler = cmd_ftp_timeout,
	}
};

command_t __root_command ftp_commands[] = {
	{
		.name = "ftp",
		.help = "ftp: File transfer protocol",
		.chain = INIT_CHAIN(ftp_subcommands),
	}
};

void cmd_ftp_setup(void) {
	command_register(ftp_commands);
}

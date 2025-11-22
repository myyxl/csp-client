#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <csp/csp.h>
#include <csp/csp_cmp.h>
#include <csp/csp_endian.h>
#include <util/console.h>
#include <util/hexdump.h>
#include <util/base16.h>
#include <util/log.h>

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#include <conf_gosh.h>
uint64_t clock_get_nsec(void);

int cmd_ping(struct command_context *ctx) {

	char * args = command_args(ctx), features[10] = "";
	unsigned int timeout = 1000, size = 1, node = 0;
	uint32_t options = CSP_O_NONE;
	char name[11] = "";

	int scanned = sscanf(args, "%10s %u %u %9s", name, &timeout, &size, features);

	if (scanned < 1 || scanned > 4)
		return CMD_ERROR_SYNTAX;
	
	if (scanned == 4) {
		if (strchr(features, 'r'))
			options |= CSP_O_RDP;
		if (strchr(features, 'x'))
			options |= CSP_O_XTEA;
		if (strchr(features, 'h'))
			options |= CSP_O_HMAC;
		if (strchr(features, 'c'))
			options |= CSP_O_CRC32;
	}

	printf("Ping name %s, timeout %u, size %u: ", name, timeout, size);

	char * endptr;
	long int name_int = strtol(name, &endptr, 10);
	if (name_int != LONG_MIN && name_int != LONG_MAX && endptr != name) {
		node = name_int;
	}

	uint64_t start, stop;
	start = clock_get_nsec();

	int time = csp_ping(node, timeout, size, options);

	stop = clock_get_nsec();

	if (time < 0) {
		printf("Timeout after %.03f ms\r\n", (stop - start) / 1E6);
		return CMD_ERROR_FAIL;
	}

	printf("Reply in %.03f ms\r\n", (stop - start) / 1E6);

	return CMD_ERROR_NONE;
}

int cmd_ps(struct command_context *ctx) {

	char * args = command_args(ctx);
	unsigned int node, timeout = 1000;

	if (sscanf(args, "%u %u", &node, &timeout) < 1)
		return CMD_ERROR_SYNTAX;

	csp_ps(node, timeout);

	return CMD_ERROR_NONE;
}

int cmd_memfree(struct command_context *ctx) {

	char * args = command_args(ctx);
	unsigned int node, timeout = 1000;

	if (sscanf(args, "%u %u", &node, &timeout) < 1)
		return CMD_ERROR_SYNTAX;

	csp_memfree(node, timeout);

	return CMD_ERROR_NONE;
}

int cmd_reboot(struct command_context *ctx) {

	char * args = command_args(ctx);
	unsigned int node;

	if (sscanf(args, "%u", &node) != 1)
		return CMD_ERROR_SYNTAX;

	csp_reboot(node);

	return CMD_ERROR_NONE;
}

int cmd_shutdown(struct command_context *ctx) {

	char * args = command_args(ctx);
	unsigned int node;

	if (sscanf(args, "%u", &node) != 1)
		return CMD_ERROR_SYNTAX;

	csp_shutdown(node);

	return CMD_ERROR_NONE;
}

int cmd_buf_free(struct command_context *ctx) {
	char * args = command_args(ctx);
	unsigned int node, timeout = 1000;

	if (sscanf(args, "%u %u", &node, &timeout) < 1)
		return CMD_ERROR_SYNTAX;

	csp_buf_free(node, timeout);

	return CMD_ERROR_NONE;
}

int cmd_uptime(struct command_context *ctx) {
	char * args = command_args(ctx);
	unsigned int node = csp_get_address(), timeout = 1000;

	sscanf(args, "%u %u", &node, &timeout);

	csp_uptime(node, timeout);

	return CMD_ERROR_NONE;
}

#ifdef CSP_DEBUG

int cmd_csp_route_print_table(struct command_context *ctx) {
	csp_route_print_table();
	return CMD_ERROR_NONE;
}

int cmd_csp_route_print_interfaces(struct command_context *ctx) {
	csp_route_print_interfaces();
	return CMD_ERROR_NONE;
}
int cmd_csp_conn_print_table(struct command_context *ctx) {
	csp_conn_print_table();
	return CMD_ERROR_NONE;
}

#endif

#if CSP_USE_RDP
int cmd_csp_rdp_set_opt(struct command_context *ctx) {
	char * args = command_args(ctx);
	unsigned int window_size;
	unsigned int conn_timeout;
	unsigned int packet_timeout;
	unsigned int delayed_acks;
	unsigned int ack_timeout;
	unsigned int ack_delay_count;
	if (sscanf(args, "%u %u %u %u %u %u", &window_size, &conn_timeout,
			&packet_timeout, &delayed_acks, &ack_timeout, &ack_delay_count) != 6)
		return CMD_ERROR_SYNTAX;

	printf("Setting arguments to: window size %u, conn timeout %u, packet timeout %u, delayed acks %u, ack timeout %u, ack delay count %u\r\n", window_size, conn_timeout, packet_timeout, delayed_acks, ack_timeout, ack_delay_count);

	csp_rdp_set_opt(window_size, conn_timeout, packet_timeout,
					delayed_acks, ack_timeout, ack_delay_count);

	return CMD_ERROR_NONE;
}
#endif

int cmd_cmp_ident(struct command_context *ctx) {

	int ret;
	unsigned int node = csp_get_address();
	unsigned int timeout = 1000;
	struct csp_cmp_message msg;

	sscanf(command_args(ctx), "%u %u", &node, &timeout);

	ret = csp_cmp_ident(node, timeout, &msg);
	if (ret != CSP_ERR_NONE) {
		printf("error: %d\r\n", ret);
		return CMD_ERROR_FAIL;
	}

	printf("Hostname: %s\r\n", msg.ident.hostname);
	printf("Model:    %s\r\n", msg.ident.model);
	printf("Revision: %s\r\n", msg.ident.revision);
	printf("Date:     %s\r\n", msg.ident.date);
	printf("Time:     %s\r\n", msg.ident.time);
    
	return CMD_ERROR_NONE;
}

int cmd_cmp_route_set(struct command_context *ctx) {

	if (ctx->argc != 6)
		return CMD_ERROR_SYNTAX;

	uint8_t node = atoi(ctx->argv[1]);
	uint32_t timeout = atoi(ctx->argv[2]);
	printf("Sending route_set to node %"PRIu8" timeout %"PRIu32"\r\n", node, timeout);

	struct csp_cmp_message msg;
	msg.route_set.dest_node = atoi(ctx->argv[3]);
	msg.route_set.next_hop_mac = atoi(ctx->argv[4]);
	strncpy(msg.route_set.interface, ctx->argv[5], 10);
	printf("Dest_node: %u, next_hop_mac: %u, interface %s\r\n", msg.route_set.dest_node, msg.route_set.next_hop_mac, msg.route_set.interface);

	int ret = csp_cmp_route_set(node, timeout, &msg);
	if (ret != CSP_ERR_NONE) {
		printf("CSP Error: %d\r\n", ret);
		return CMD_ERROR_FAIL;
	} else {
		printf("Success\r\n");
	}

	return CMD_ERROR_NONE;
}

int cmd_cmp_ifc(struct command_context *ctx) {

	uint8_t node;
	uint32_t timeout;
	char * interface;

	if (ctx->argc > 4 || ctx->argc < 3)
		return CMD_ERROR_SYNTAX;

	node = atoi(ctx->argv[1]);
	interface = ctx->argv[2];

	if (ctx->argc < 4)
		timeout = 1000;
	else
		timeout = atoi(ctx->argv[3]);

	struct csp_cmp_message msg;
	strncpy(msg.if_stats.interface, interface, CSP_CMP_ROUTE_IFACE_LEN);

	printf("Requesting interface stats for interface %s\r\n", interface);

	int ret = csp_cmp_if_stats(node, timeout, &msg);
	if (ret != CSP_ERR_NONE) {
		printf("Error: %d\r\n", ret);
		return CMD_ERROR_FAIL;
	}

	msg.if_stats.tx = csp_ntoh32(msg.if_stats.tx);
	msg.if_stats.rx = csp_ntoh32(msg.if_stats.rx);
	msg.if_stats.tx_error = csp_ntoh32(msg.if_stats.tx_error);
	msg.if_stats.rx_error = csp_ntoh32(msg.if_stats.rx_error);
	msg.if_stats.drop = csp_ntoh32(msg.if_stats.drop);
	msg.if_stats.autherr = csp_ntoh32(msg.if_stats.autherr);
	msg.if_stats.frame = csp_ntoh32(msg.if_stats.frame);
	msg.if_stats.txbytes = csp_ntoh32(msg.if_stats.txbytes);
	msg.if_stats.rxbytes = csp_ntoh32(msg.if_stats.rxbytes);
	msg.if_stats.irq = csp_ntoh32(msg.if_stats.irq);

	printf("%-5s   tx: %05"PRIu32" rx: %05"PRIu32" txe: %05"PRIu32" rxe: %05"PRIu32"\r\n"
	       "        drop: %05"PRIu32" autherr: %05"PRIu32 " frame: %05"PRIu32"\r\n"
	       "        txb: %"PRIu32" rxb: %"PRIu32"\r\n\r\n",
			msg.if_stats.interface, msg.if_stats.tx, msg.if_stats.rx, msg.if_stats.tx_error, msg.if_stats.rx_error, msg.if_stats.drop,
			msg.if_stats.autherr, msg.if_stats.frame, msg.if_stats.txbytes, msg.if_stats.rxbytes);

	return CMD_ERROR_NONE;
}

int cmd_cmp_peek(struct command_context *ctx) {

	uint8_t node;
	uint32_t timeout;
	uint32_t addr, len;

	if ((ctx->argc != 4) && (ctx->argc != 5))
		return CMD_ERROR_SYNTAX;

	node = atoi(ctx->argv[1]);
	if (sscanf(ctx->argv[2], "%"SCNx32, &addr) != 1)
		return CMD_ERROR_SYNTAX;
	if (sscanf(ctx->argv[3], "%"SCNx32, &len) != 1)
		return CMD_ERROR_SYNTAX;
	if (len > CSP_CMP_PEEK_MAX_LEN) {
		printf("Max length is: %u\r\n", CSP_CMP_PEEK_MAX_LEN);
		return CMD_ERROR_SYNTAX;
	}
	if (ctx->argc < 5)
		timeout = 1000;
	else
		timeout = atoi(ctx->argv[4]);

	printf("Dumping mem from node %u addr 0x%"PRIx32" len %"PRIx32" timeout %"PRIu32"\r\n", node, addr, len, timeout);

	struct csp_cmp_message msg;
	msg.peek.addr = csp_hton32(addr);
	msg.peek.len = len;

	int ret = csp_cmp_peek(node, timeout, &msg);
	if (ret != CSP_ERR_NONE) {
		printf("Error: %d\r\n", ret);
		return CMD_ERROR_FAIL;
	}

	hex_dump(msg.peek.data, len);

	return CMD_ERROR_NONE;
}

int cmd_cmp_dump(struct command_context *ctx) {

	uint8_t node;
	uint32_t timeout;

	uint32_t len = 0xC8;
	uint32_t addr = 0x00;

	uint32_t memsize = 0xFFF; // TODO: figure out how to get memory size of node

	if ((ctx->argc != 2) && (ctx->argc != 3))
		return CMD_ERROR_SYNTAX;

	node = atoi(ctx->argv[1]);
	if (ctx->argc < 3)
		timeout = 1000;
	else
		timeout = atoi(ctx->argv[1]);

	uint32_t dump[memsize];

	printf("Dumping mem from node %u to file dump.bin\r\n", node);

	for(uint32_t caddr = 0x00; caddr < memsize; caddr += addr) {

		struct csp_cmp_message msg;
		msg.peek.addr = csp_hton32(caddr);
		msg.peek.len = len;

		int ret = csp_cmp_peek(node, timeout, &msg);

		if (ret != CSP_ERR_NONE) {
			printf("Error: %d\r\n", ret);
			return CMD_ERROR_FAIL;
		}

		memcpy(&dump[caddr], &msg.peek.data[caddr], len);
	}

	hex_dump(dump, memsize);

	return CMD_ERROR_NONE;
}

int cmd_cmp_poke(struct command_context *ctx) {

	uint8_t node;
	uint32_t timeout;
	uint32_t addr, len;
	unsigned char data[CSP_CMP_POKE_MAX_LEN];

	if ((ctx->argc != 4) && (ctx->argc != 5))
		return CMD_ERROR_SYNTAX;

	node = atoi(ctx->argv[1]);
	if (sscanf(ctx->argv[2], "%"SCNx32, &addr) != 1)
		return CMD_ERROR_SYNTAX;

	len = base16_decode(ctx->argv[3], data);
	if (len > CSP_CMP_PEEK_MAX_LEN) {
		printf("Max length is: %u\r\n", CSP_CMP_PEEK_MAX_LEN);
		return CMD_ERROR_SYNTAX;
	}

	if (ctx->argc < 5)
		timeout = 1000;
	else
		timeout = atoi(ctx->argv[4]);

	printf("Writing to mem at node %u addr 0x%"PRIx32" len %"PRIx32" timeout %"PRIu32"\r\n", node, addr, len, timeout);
	hex_dump(data, len);

	struct csp_cmp_message msg;
	msg.poke.addr = csp_hton32(addr);
	msg.poke.len = len;
	memcpy(msg.poke.data, data, CSP_CMP_POKE_MAX_LEN);

	int ret = csp_cmp_poke(node, timeout, &msg);
	if (ret != CSP_ERR_NONE) {
		printf("Error: %d\r\n", ret);
		return CMD_ERROR_FAIL;
	}

	printf("Done\r\n");
	return CMD_ERROR_NONE;
}

int cmd_cmp_clock(struct command_context *ctx) {

	if ((ctx->argc < 3 || ctx->argc > 5) && ctx->argc != 2)
		return CMD_ERROR_SYNTAX;

	int node = atoi(ctx->argv[1]);
	int32_t sec = 0;
	int32_t nsec = 0;
	csp_timestamp_t t1, t2;
	int64_t local, remote, rtt;
	double diff;

#ifdef HAVE_TIME_H
	time_t tt;
#endif
	struct csp_cmp_message msg = {};
	msg.clock.tv_sec = 0;
	msg.clock.tv_nsec = 0;

	if (ctx->argc > 2) {
		sec = atol(ctx->argv[2]);

		if (ctx->argc > 3)
			nsec = atol(ctx->argv[3]);

		/* Get time from OS */
		if (sec == -1) {
			clock_get_time(&msg.clock);
		} else {
			msg.clock.tv_sec = sec;
			msg.clock.tv_nsec = nsec;
		}
#ifdef HAVE_TIME_H
		tt = msg.clock.tv_sec;
		printf("Set time: %s", ctime(&tt));
#endif
	}

	int timeout = 1000;
	if (ctx->argc >= 5)
		timeout = atoi(ctx->argv[4]);

	msg.clock.tv_sec = csp_hton32(msg.clock.tv_sec);
	msg.clock.tv_nsec = csp_hton32(msg.clock.tv_nsec);

	clock_get_time(&t1);

	if (csp_cmp_clock(node, timeout, &msg) != CSP_ERR_NONE)
		return CMD_ERROR_FAIL;

	clock_get_time(&t2);

	/* Calculate round-trip time */
	rtt = ((uint64_t)t2.tv_sec * 1000000000 + t2.tv_nsec) - ((uint64_t)t1.tv_sec * 1000000000 + t1.tv_nsec);

	msg.clock.tv_sec = csp_ntoh32(msg.clock.tv_sec);
	msg.clock.tv_nsec = csp_ntoh32(msg.clock.tv_nsec);

#ifdef HAVE_TIME_H
	tt = msg.clock.tv_sec;
	printf("Get time: %s", ctime(&tt));
#else
	printf("Get time: %"PRIu32".%06"PRIu32"\r\n", msg.clock.tv_sec, msg.clock.tv_nsec / 1000);
#endif

	/* Calculate time difference to local clock. This takes the round-trip
	 * into account, but assumes a symmetrical link */
	remote = (uint64_t)msg.clock.tv_sec * 1000000000 + msg.clock.tv_nsec;
	local = (uint64_t)t1.tv_sec * 1000000000 + t1.tv_nsec + rtt / 2;

	diff = (remote - local) / 1000000.0;
	printf("Remote is %f ms %s local time\r\n", fabs(diff), diff > 0 ? "ahead of" : "behind");

	return CMD_ERROR_NONE;
}

command_t __sub_command cmp_commands[] = {
	{
		.name = "ident",
		.help = "Node id",
		.usage = "[node] [timeout]",
		.handler = cmd_cmp_ident,
	},{
		.name = "route_set",
		.help = "Update table",
		.usage = "<node> <timeout> <addr> <mac> <ifstr>",
		.handler = cmd_cmp_route_set,
	},{
		.name = "ifc",
		.help = "Remote IFC",
		.usage = "<node> <interface> [timeout]",
		.handler = cmd_cmp_ifc,
	},{
		.name = "peek",
		.help = "Show remote memory",
		.usage = "<node> <addr> <len> [timeout]",
		.handler = cmd_cmp_peek,
	},{
		.name = "dump",
		.help = "dump remote memory to file",
		.usage = "<node> [timeout]",
		.handler = cmd_cmp_dump,
	},{
		.name = "poke",
		.help = "Modify remote memory",
		.usage = "<node> <addr> <base16_data> [timeout]",
		.handler = cmd_cmp_poke,
	},{
		.name = "clock",
		.help = "Get/Set clock, get t=0, setsys t=-1",
		.usage = "<node> [<sec> <nsec>] [timeout]",
		.handler = cmd_cmp_clock,
	}
};

command_t __root_command csp_commands[] = {
	{
		.name = "ping",
		.help = "csp: Ping",
		.usage = "<node> [timeout] [size] [opt]",
		.handler = cmd_ping,
	},{
		.name = "rps",
		.help = "csp: Remote ps",
		.usage = "<node> <timeout>",
		.handler = cmd_ps,
	},{
		.name = "memfree",
		.help = "csp: Memory free",
		.usage = "<node> <timeout>",
		.handler = cmd_memfree,
	},{
		.name = "buffree",
		.help = "csp: Buffer free",
		.usage = "<node> <timeout>",
		.handler = cmd_buf_free,
	},{
		.name = "reboot",
		.help = "csp: Reboot",
		.usage = "<node>",
		.handler = cmd_reboot,
	},{
		.name = "shutdown",
		.help = "csp: Shutdown",
		.usage = "<node>",
		.handler = cmd_shutdown,
	},{
		.name = "uptime",
		.help = "csp: Uptime",
		.usage = "<node> <timeout>",
		.handler = cmd_uptime,
	},{
		.name = "cmp",
		.help = "csp: Management",
		.chain = INIT_CHAIN(cmp_commands),
	},
#ifdef CSP_DEBUG
	{
		.name = "route",
		.help = "csp: Show routing table",
		.handler = cmd_csp_route_print_table,
	},{
		.name = "ifc",
		.help = "csp: Show interfaces",
		.handler = cmd_csp_route_print_interfaces,
	},{
		.name = "conn",
		.help = "csp: Show connection table",
		.handler = cmd_csp_conn_print_table,
	},
#endif
#if CSP_USE_RDP
	{
		.name = "rdpopt",
		.help = "csp: Set RDP options",
		.handler = cmd_csp_rdp_set_opt,
		.usage = "<window size> <conn timeout> <packet timeout> <delayed ACKs> <ACK timeout> <ACK delay count>"
	},
#endif
};

void csp_console_init(void) {
	command_register(csp_commands);
}

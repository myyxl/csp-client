/**
 * GomSpace Parameter System
 *
 * @author Johan De Claville Christiansen
 * Copyright 2014 GomSpace ApS. All rights reserved.
 */

#include <stdio.h>
#include <stddef.h>
#include <alloca.h>
#include <stdlib.h>


#include <csp/csp.h>
#include <csp/csp_endian.h>

#include <param/rparam_client.h>
#include <param/param_types.h>
#include <param/param_serializer.h>
#include <param/param_fletcher.h>
#include <param/param_string.h>
#include <param/param_memio.h>

#include <util/log.h>
#include <command/command.h>

/////
// query interface globals
/////
static uint8_t query_buf[RPARAM_QUERY_MAX_LEN+10] = {0};
static rparam_query * query = (void *) &query_buf;
static int query_get_size = 0;

static uint8_t quiet = RPARAM_QUIET;

int rparam_query_is_set() {
	return query->length;
}

void rparam_query_reset() {
	query->action = RPARAM_GET;
	query->length = 0;
	query_get_size = 0;
}

void rparam_set_quiet(uint8_t _quiet) {
	quiet = _quiet;
}

void rparam_query_print(param_index_t* index) {
	unsigned int start = 0;

	if (index->table == NULL)
		return;

	switch(query->action) {
	case RPARAM_GET:
		param_serialize_list(index, query->addr, query->length/2, &start, NULL, RPARAM_QUERY_MAX_LEN, F_DRY_RUN | F_TO_BIG_ENDIAN | F_PRINT_GET);
		break;
	case RPARAM_SET:
		param_deserialize(index, query->packed, query->length, F_DRY_RUN | F_FROM_BIG_ENDIAN | F_PRINT_SET);
		break;
	case RPARAM_REPLY:
		param_deserialize(index, query->packed, query->length, F_DRY_RUN | F_FROM_BIG_ENDIAN | F_PRINT_REP);
		break;
	case RPARAM_COPY:
		printf("  COPY from <%u> to <%u>\r\n", query->copy.from, query->copy.to);
		break;
	case RPARAM_LOAD:
		printf("  LOAD from <%u> to <%u>\r\n", query->copy.from, query->copy.to);
		break;
	case RPARAM_CLEAR:
		printf("  CLEAR <%u>\r\n", query->clear.id);
		break;
	case RPARAM_SAVE:
		printf("  SAVE from <%u> to <%u>\r\n", query->copy.from, query->copy.to);
		break;
	default:
		printf("Unknown rparm action\r\n");
		break;
	}
	return;
}

int rparam_copy(uint8_t node, uint8_t port, uint32_t timeout, uint8_t from, uint8_t to) {
	return rparam_command(node, port, timeout, RPARAM_COPY, from, to);
}
int rparam_save(uint8_t node, uint8_t port, uint32_t timeout, uint8_t from, uint8_t to) {
	return rparam_command(node, port, timeout, RPARAM_SAVE, from, to);
}
int rparam_clear(uint8_t node, uint8_t port, uint32_t timeout, uint8_t id) {
	return rparam_command(node, port, timeout, RPARAM_CLEAR, id, 0);
}
int rparam_load(uint8_t node, uint8_t port, uint32_t timeout, uint8_t from, uint8_t to) {
	return rparam_command(node, port, timeout, RPARAM_LOAD, from, to);
}

int rparam_command(uint8_t node, uint8_t port, uint32_t timeout, uint8_t action, uint8_t from, uint8_t to) {

	/* Calculate length */
	size_t query_size = offsetof(rparam_query, packed) + sizeof(uint16_t);
	rparam_query * query = alloca(query_size);

	query->action = action;
	query->mem = from;
	query->length = 0;

	switch (action) {
	case RPARAM_COPY:
	case RPARAM_SAVE:
	case RPARAM_LOAD:
		query->copy.from = from;
		query->copy.to = to;
		break;
	case RPARAM_CLEAR:
		query->clear.id = from;
		break;
	default:
		if (!quiet)
			printf("Invalid remote parameter action: %u\r\n", action);
		return 0;
		break;
	}
	query->checksum = csp_hton16(0xB00B); /* Ignore checksum */
	query->length = csp_hton16(query->length);

	/* Run single packet transaction */
	char reply = 0;
	if (csp_transaction(CSP_PRIO_HIGH, node, port, timeout, query, query_size, &reply, 1) <= 0) {
		if (!quiet)
			printf("Error in csp_transaction\r\n");
		return 0;
	}
	switch (action) {
	case RPARAM_COPY:
		if (reply == RPARAM_COPY_OK) {
			if (!quiet)
				printf("remote parameter table copy ok\r\n");
		} else {
			if (!quiet)
				printf("Error copying remote parameter table: %u\r\n", reply);
			return 0;
		}
		break;
	case RPARAM_SAVE:
		if (reply == RPARAM_SAVE_OK) {
			if (!quiet)
				printf("remote parameter table save ok\r\n");
		} else {
			if (!quiet)
				printf("Error saving remote parameter table: %u\r\n", reply);
			return 0;
		}
		break;
	case RPARAM_LOAD:
		if (reply == RPARAM_LOAD_OK) {
			if (!quiet)
				printf("remote parameter table load ok\r\n");
		} else {
			if (!quiet)
				printf("Error loading remote parameter table: %u\r\n", reply);
			return 0;
		}
		break;
	case RPARAM_CLEAR:
		if (reply == RPARAM_CLEAR_OK) {
			if (!quiet)
				printf("remote parameter table clear ok\r\n");
		} else {
			if (!quiet)
				printf("Error clearing remote parameter table: %u\r\n", reply);
			return 0;
		}
		break;
	}

	return 1;
}

int rparam_query_get(param_index_t* index, char* param_name) {

	if (index->table == NULL)
		return CMD_ERROR_FAIL;

	/* Ensure correct header */
	if (query->action != RPARAM_GET) {
		query->length = 0;
		query->action = RPARAM_GET;
		query_get_size = 0;
	}

	char shortname[strlen(param_name)];
	int array_index = -1;
	sscanf(param_name, "%[^[][%d]", shortname, &array_index);

	const param_table_t * param = param_find_name(index->table, index->count, shortname);
	if (param == NULL) {
		printf("Unknown parameter %s, check 'rparam list' for names\r\n", param_name);
		return CMD_ERROR_INVALID;
	}

	if (array_index >= param->count) {
		printf("Array index out of bounds, max index = %u\r\n", param->count - 1);
		return CMD_ERROR_FAIL;
	}

	if (array_index >= 0) {

		/* Size check */
		if (query_get_size + param->size + sizeof(uint16_t) > RPARAM_QUERY_MAX_LEN)
			return CMD_ERROR_FAIL;

		/* Add to query */
		query->addr[query->length/2] = param->addr + (param->size * array_index);
		query->length += sizeof(uint16_t);
		query_get_size += param->size + sizeof(uint16_t);

	} else {

		int count = param->count;
		if (count == 0)
			count = 1;

		for (int i = 0; i < count; i++) {

			/* Size check */
			if (query_get_size + param->size + sizeof(uint16_t) > RPARAM_QUERY_MAX_LEN)
				return CMD_ERROR_FAIL;

			/* Add to query */
			query->addr[query->length/2] = param->addr + (param->size * i);
			query->length += sizeof(uint16_t);
			query_get_size += param->size + sizeof(uint16_t);

		}

	}

	/* Perform dry run serialization on table */
	unsigned int start = 0;
	int flags = F_DRY_RUN;
	if (!quiet)
		flags |= F_PRINT_GET;
	param_serialize_list(index, query->addr, query->length/2, &start, NULL, RPARAM_QUERY_MAX_LEN, flags);

	return CMD_ERROR_NONE;
}

int rparam_query_set(param_index_t* index, char* param_name, char* values[], uint8_t value_count) {

	/* Ensure correct header */
	if (query->action != RPARAM_SET) {
		query->length = 0;
		query->action = RPARAM_SET;
	}

	char shortname[strlen(param_name)];
	int array_index = -1;
	sscanf(param_name, "%[^[][%d]", shortname, &array_index);

	const param_table_t * param = param_find_name(index->table, index->count, shortname);
	if (param == NULL) {
		printf("Unknown parameter %s\r\n", param_name);
		return CMD_ERROR_FAIL;
	}

	if (array_index >= param->count) {
		printf("Array index out of bounds, max index = %u\r\n", param->count - 1);
		return CMD_ERROR_FAIL;
	}

	if (array_index >= 0) {

		if (value_count != 1) {
			printf("Use only one argument for array offsets");
			return CMD_ERROR_FAIL;
		}

		/* Parse input */
		uint8_t value[param->size];
		if (param_from_string(param, values[0], value) < 0)
			return CMD_ERROR_INVALID;

		/* Actual set query */
		param_serialize_item(param, param->addr + (param->size * array_index), query->packed, &query->length, RPARAM_QUERY_MAX_LEN, value, F_TO_BIG_ENDIAN);

		uint8_t string[100];
		param_to_string(param, string, 0, value, 0, 100);
		if (!quiet)
			printf("  INP %s[%u] = %s\r\n", shortname, array_index, string);

	} else {

		/* Check count for arrays */
		if ((param->count != 0) && (value_count > param->count)) {
			printf("Too many arguments for %s[%u]\r\n", param->name, param->count);
			return CMD_ERROR_FAIL;
		}

		for (int i = 0; i < value_count; i++) {

			/* Parse input */
			uint8_t value[param->size];
			if (param_from_string(param, values[i], value) < 0)
				return CMD_ERROR_INVALID;

			/* Actual set query */
			param_serialize_item(param, param->addr + (param->size * i), query->packed, &query->length, RPARAM_QUERY_MAX_LEN, value, F_TO_BIG_ENDIAN);

			uint8_t string[100];
			param_to_string(param, string, 0, value, 0, 100);

			if (param->count > 1) {
				if (!quiet)
					printf("  INP %s[%u] = %s\r\n", param->name, i, string);
			} else {
				if (!quiet)
					printf("  INP %s = %s\r\n", param->name, string);
			}
		}
	}

	return CMD_ERROR_NONE;
}

int rparam_query_send(param_index_t* index, uint8_t node, uint8_t port, uint16_t checksum) {

	if (index->table == NULL)
		return CMD_ERROR_FAIL;

	if (query->length == 0)
		query->action = RPARAM_GET;

	/* Allocate outgoing buffer */
	csp_packet_t * packet = csp_buffer_get(RPARAM_QUERY_MAX_LEN);
	if (packet == NULL)
		return CMD_ERROR_FAIL;

	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, port, 10000, CSP_O_CRC32);

	/* Copy data to send */
	packet->length = query->length + offsetof(rparam_query, addr);
	memcpy(packet->data, query, packet->length);
	rparam_query * net_query = (rparam_query *) packet->data;

	/* Checksum */
	net_query->checksum = checksum;

	/* Set mem */
	net_query->mem = index->mem_id;

	/* Deal with endianness */
	if (query->action == RPARAM_GET)
		for (int i = 0; i < query->length; i++)
			net_query->addr[i] = csp_hton16(net_query->addr[i]);
	net_query->length = csp_hton16(net_query->length);
	net_query->checksum = csp_hton16(net_query->checksum);

	/* Send packet */
	if (!csp_send(conn, packet, 0)) {
		if (!quiet)
			printf("Failed to send query\r\n");
		csp_buffer_free(packet);
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	/* Read reply */
	packet = csp_read(conn, 1000);
	if (packet == NULL) {
		if (!quiet)
			printf("No reply\r\n");
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	if (packet->length <= 1) {
		int rc = CMD_ERROR_NONE;
		if (packet->length == 1) {
			if (!quiet)
				printf("Result: %u\r\n", packet->data[0]);
			if (packet->data[0] == RPARAM_ERROR)
				rc = CMD_ERROR_FAIL;
		} else {
			if (!quiet)
				printf("Error: empty reply\r\n");
		}
		csp_buffer_free(packet);
		csp_close(conn);
		return rc;
	}

	/* We have a reply */
	rparam_query * reply = (rparam_query *) packet->data;
	reply->length = csp_ntoh16(reply->length);

	if (reply->action == RPARAM_REPLY) {
		int flags = F_FROM_BIG_ENDIAN;
		if (!quiet)
			flags |= F_PRINT_REP;
		param_deserialize(index, reply->packed, reply->length, flags);
	}

	csp_buffer_free(packet);
	csp_close(conn);
	return CMD_ERROR_NONE;
}

int rparam_load_table_spec_from_file(const char* fname, param_index_t* index, uint16_t* checksum) {

	/** Local storage of parameters */
	if (index->physaddr == NULL) {
		index->physaddr = calloc(0x400, 1);
		index->size = 0x400;
	}

	FILE * fd = fopen(fname, "r");
	if (fd == NULL) {
		if (!quiet)
			printf("Unknown parameter list or file: %s\r\n", fname);
		return CMD_ERROR_INVALID;
	}

	if (index->table != NULL)
		free((void *) index->table);
	index->table = calloc(10000, 1);
	if (index->table == NULL)
		return CMD_ERROR_NOMEM;

	int bytes = fread(checksum, 1, sizeof(uint16_t), fd);
	if (!quiet)
		printf("Read %d bytes from %s\r\n", bytes, fname);
	bytes = fread((void *)index->table, 1, 10000, fd);
	if (!quiet)
		printf("Read %d bytes from %s\r\n", bytes, fname);
	fclose(fd);

	index->count = bytes / sizeof(param_table_t);

	return CMD_ERROR_NONE;
}

int rparam_download_table_spec_from_remote_and_save_to_file(const char* fname, uint8_t rparam_node, uint8_t rparam_port, param_index_t* index, uint16_t* checksum) {
	return rparam_download_table_spec_from_remote_and_save_to_file2(fname, rparam_node, rparam_port, index, index->mem_id, checksum);
}

int rparam_download_table_spec_from_remote_and_save_to_file2(const char* fname, uint8_t rparam_node, uint8_t rparam_port, param_index_t* index, uint8_t remote_table_id, uint16_t* checksum) {

	query->action = RPARAM_TABLE;
	query->length = 0;
	query->mem = remote_table_id;

	if (index->physaddr == NULL) {
		index->physaddr = calloc(0x400, 1);
		if (index->physaddr != NULL)
			index->size = 0x400;
	}
	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, rparam_node, rparam_port, 10000, CSP_O_CRC32);
	if (conn == NULL)
		return CMD_ERROR_FAIL;

	/* Allocate outgoing buffer */
	csp_packet_t * packet = csp_buffer_get(RPARAM_QUERY_MAX_LEN);
	if (packet == NULL) {
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	/* Copy data to send */
	packet->length = query->length + offsetof(rparam_query, packed);
	memcpy(packet->data, query, packet->length);

	/* Send packet */
	if (!csp_send(conn, packet, 0)) {
		csp_buffer_free(packet);
		csp_close(conn);
		return CMD_ERROR_FAIL;
	}

	/* Receive remote parameter table, in host byte order
	 * Note: This is necessary, because the SFP functions does not know the dataformat
	 * and hence cannot be converted server-side. */
	void * dataout = NULL;
	int totalsize = 0;
	int result = csp_sfp_recv(conn, &dataout, &totalsize, 10000);
	csp_close(conn);

	if (result < 0) {
		if (dataout != NULL)
			free(dataout);
		return CMD_ERROR_FAIL;
	}

	param_table_t * table = dataout;
	uint8_t count = totalsize / sizeof(param_table_t);

	/* Calculate checksum on table (before converting endians!) */
	*checksum = fletcher16((void *) table, totalsize);
	if (!quiet)
		printf("Checksum is: 0x%X\r\n", *checksum);

	/* Autodetect Endians */
	int sum_first = 0, sum_last = 0;
	for (int i = 0; i < count; i++) {
		sum_first += (table[i].addr & 0xFF00) >> 8;
		sum_last += table[i].addr & 0xFF;
	}

	/* Correct endians */
	if (sum_first > sum_last) {
		for (int i = 0; i < count; i++) {
			table[i].addr = (((table[i].addr & 0xff00) >> 8) | ((table[i].addr & 0x00ff) << 8));
		}
	}

	/* Store table specification to file */
	FILE * fd = fopen(fname, "w");
	if (fd != NULL) {
		int count = 0;
		count = fwrite(checksum, 1, sizeof(uint16_t), fd);
		if (!quiet)
			printf("Wrote %u bytes to %s\r\n", count, fname);
		count = fwrite(table, 1, totalsize, fd);
		if (!quiet)
			printf("Wrote %u bytes to %s\r\n", count, fname);
		fclose(fd);
	}

	/* Store table specification to memory */
	if (index->table != NULL)
		free((void *) index->table);
	index->table = table;
	index->count = count;

	return CMD_ERROR_NONE;
}

int rparam_get_full_table(param_index_t * index, int node, int port, int index_id, int timeout) {

	int ret = 0;
	csp_conn_t *conn;
	csp_packet_t *request, *reply;

	request = csp_buffer_get(RPARAM_QUERY_MAX_LEN);
	if (request == NULL)
		return -1;

	conn = csp_connect(CSP_PRIO_HIGH, node, port, timeout, CSP_O_NONE);
	if (!conn) {
		csp_buffer_free(request);
		return -1;
	}

	rparam_query * query = (void *) request->data;
	query->action = RPARAM_GET;
	query->checksum = csp_hton16(0xB00B);
	query->length = csp_hton16(0);
	query->mem = index_id;

	request->length = query->length + offsetof(rparam_query, addr);

	if (!csp_send(conn, request, timeout)) {
		csp_buffer_free(request);
		csp_close(conn);
		return -1;
	}

	while ((reply = csp_read(conn, timeout)) != NULL) {

		/* We have a reply */
		query = (void *) reply->data;
		query->length = csp_ntoh16(query->length);

		query->seq = csp_ntoh16(query->seq);
		query->total = csp_ntoh16(query->total);

		if (query->action == RPARAM_REPLY)
			param_deserialize(index, query->packed, query->length, F_FROM_BIG_ENDIAN);
		csp_buffer_free(reply);

		if (query->seq >= query->total)
			break;
	}

	if (!reply)
		ret = -1;

	csp_close(conn);

	return ret;
}

int rparam_get_single(void * out, uint16_t addr, param_type_t type, int size, int index_id, int node, int port, int timeout) {

	/* Calculate length */
	size_t query_size = offsetof(rparam_query, packed) + sizeof(uint16_t);
	size_t reply_size = offsetof(rparam_query, packed) + size + sizeof(uint16_t);

	rparam_query * query = alloca(reply_size);
	query->action = RPARAM_GET;
	query->mem = index_id;

	/* Add to query */
	query->addr[0] = csp_hton16(addr);
	query->checksum = csp_hton16(0xB00B);
	query->length = csp_hton16(sizeof(query->addr[0]));

	/* Run single packet transaction */
	if (csp_transaction(CSP_PRIO_HIGH, node, port, timeout, query, query_size, query, reply_size) <= 0)
		return -1;

	/* We have a reply */
	query->length = csp_ntoh16(query->length);

	if (query->length != sizeof(uint16_t) + size) {
		printf("Invalid reply size %u\r\n", query->length);
		return -1;
	}

	/* Read address */
	query->addr[0] = csp_betoh16(query->addr[0]);
	if (query->addr[0] != addr) {
		printf("Invalid address in reply %u\r\n", query->addr[0]);
		return -1;
	}

	/* Read value */
	memcpy(out, &query->packed[2], size);
	param_betoh(type, out);

	return size;

}

int rparam_set_single(void * in, uint16_t addr, param_type_t type, int size, int index_id, int node, int port, int timeout) {

	/* Calculate length */
	size_t query_size = offsetof(rparam_query, packed) + size + sizeof(uint16_t);

	rparam_query * query = alloca(query_size);
	query->action = RPARAM_SET;
	query->mem = index_id;
	query->length = 0;
	query->checksum = csp_hton16(0xB00B);

	/* Actual set query */
	if (param_serialize_item_direct(addr, type, size, query->packed, &query->length, RPARAM_QUERY_MAX_LEN, in, F_TO_BIG_ENDIAN) < 0)
		return -1;

	/* Add to query */
	query->length = csp_hton16(query->length);

	/* Run single packet transaction */
	if (csp_transaction(CSP_PRIO_HIGH, node, port, timeout, query, query_size, query, 1) <= 0)
		return -1;

	/* We have a reply */
	query->length = csp_ntoh16(query->length);

	if (query->length != sizeof(uint16_t) + size) {
		printf("Invalid reply size %u\r\n", query->length);
		return -1;
	}

	return size;

}

int rparam_query_get_value(param_index_t* index, char* param_name, uint16_t param_no, void* val_p, uint16_t val_size) {

	const param_table_t* t = param_find_name(index->table, index->count, param_name);
	if (t == NULL)
		return -1;

	if (val_size < t->size)
		return -2;

	void* p = param_read_addr(t->addr + param_no * t->size, index,  param_no * t->size);
	if (p == NULL)
		return -3;

	memcpy(val_p, p, t->size);
	return 0;
}



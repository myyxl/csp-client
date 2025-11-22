#include <stdio.h>
#include <string.h>

#include "param/rparam_client.h"
#include <util/vmem.h>
// needed by libutil to link
const vmem_t vmem_map[] = {{0}};

#define CSP_NODE_ADDR    8
#define CSP_RPARAM_PORT  7
#define CSP_TIMEOUT      100

#define TABLE_INDEX      1
#define PARAM1_ADDR      0x10
#define PARAM2_ADDR      0x20

int main(void) {

	/**
	 * Fetching a remote parameter
	 */
	uint8_t val;
	int rc = rparam_get_uint8(&val, PARAM1_ADDR, TABLE_INDEX, CSP_NODE_ADDR, CSP_RPARAM_PORT, CSP_TIMEOUT);
	if (rc > 0)
		printf("parameter value is %i\n", val);
	else
		printf("rparam_get_uint8() failed, rc=%i\n", rc);

	/**
	 * Setting a remote parameter
	 */
	char string[20];
	strncpy(string, "hello world", 20);
	rc = rparam_set_string(string, 20, PARAM2_ADDR, TABLE_INDEX, CSP_NODE_ADDR, CSP_RPARAM_PORT, CSP_TIMEOUT);
	if (rc > 0)
		printf("parameter set succesfully\n");
	else
		printf("rparam_set_string() failed, rc=%i\n", rc);
	return 0;

	/*
	 * Getting multiple parameters with the query interface
	 */
	uint16_t crc = 0;
	static param_index_t index = {0};
	index.mem_id = TABLE_INDEX;

	// download table spec
	int rc = rparam_download_table_spec_from_remote_and_save_to_file("table_spec.bin", CSP_NODE_ADDR, CSP_RPARAM_PORT, &index, &crc);
	if (rc !== CMD_ERROR_NONE)
		exit(-1);

	// build a multi-part query
	rparam_query_get(&index, "p1");
	rparam_query_get(&index, "p2");
	rparam_query_get(&index, "p3");

	// send the query
	rc = rparam_query_send(&index, CSP_NODE_ADDR, CSP_RPARAM_PORT, crc);
	if (rc != CMD_ERROR_NONE)
		exit(-2);

	// print the fetched parameters
	uint16_t value;
	int rc = rparam_query_get_value(&index, "p1", 0, &value, sizeof(value));
	if (rc == 0)
		printf("value of p1 is %i\n", value);
	rc = rparam_query_get_value(&index, "p2", 0, &value, sizeof(value));
	if (rc == 0)
		printf("value of p2 is %i\n", value);
	rc = rparam_query_get_value(&index, "p3", 0, &value, sizeof(value));
		if (rc == 0)
			printf("value of p3 is %i\n", value);

}

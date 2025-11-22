#ifndef HK_PARSER_H_
#define HK_PARSER_H_

#include <csp/csp.h>
#include <param/param.h>

#include <util/log.h>
#include <hk_types.h>
LOG_GROUP_EXTERN(log_hk_parser);

/**
 * Call this function to initialize hk_parser db connection
 * @param dbserver name/ip of db server
 * @param dbport tcp port number to connect on
 * @param dbtable name of table to insert data to
 */
void hk_parser_init(char * dbserver, uint16_t dbport, char * dbtable);

/**
 * Call this function to parse a packet containing beacon data
 * @param conn pointer to connection
 * @param packet pointer to packet
 */
void hk_parser(csp_conn_t * conn, csp_packet_t * packet);

/**
 * Call this function to parse a data buffer containing beacon data
 * @param node node id of sending node
 * @param length length of beacon data
 * @param data pointer to beacon data
 */
uint32_t hk_parser_direct(uint8_t node, uint16_t length, uint8_t * data);

hk_beacon_t * hk_beacon_find(uint8_t node, uint8_t type);
hk_table_t * hk_beacon_table_find(uint8_t memid);

#endif /* HK_PARSER_H_ */

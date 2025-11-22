#ifndef HK_SERVICE_H_
#define HK_SERVICE_H_

#include <stdio.h>
#include <param/param.h>
#include <csp/arch/csp_thread.h>

/**
 * Set beacon destination address and port
 * @param dst_addr beacon destination CSP address
 * @param dst_port beacon destination CSP port
 * @return -1 if err, 0 if ok
 */
int hk_set_beacon_dest(uint8_t dst_addr, uint8_t dst_port);

/**
 * Send out beacon
 * @param conn pointer to active conn or NULL for default ground station destination
 * @param type beacon type-id
 * @param time timestamp to search from (t0) or zero to send out current value
 * @param found pointer to arary of cached table pointers
 * @return -1 if err, 0 if ok
 */
int hk_service_send(csp_conn_t * conn, uint8_t type, uint32_t time, param_pool_t * found[PARAM_MAX_TABLES], FILE * fp);

/**
 * Task
 */
CSP_DEFINE_TASK(hk_service);

#endif /* HK_SERVICE_H_ */

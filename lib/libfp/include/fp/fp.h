/**
 * @file fp.h
 * Main flight planner header file
 *
 * @author Jeppe Ledet-Pedersen
 * Copyright 2011 GomSpace ApS. All rights reserved.
 */

#ifndef _FP_H_
#define _FP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <log/log.h>

#include <stdint.h>

/** Common FP log group */
LOG_GROUP_EXTERN(fp_group);

/** Maximum size of keys for events, triggers, sequences and commands */
#define FP_KEY_SIZE	30

/** Maximum size of argument structures for commands */
#define FP_CMD_SIZE	60

/** fp_init
 * Initialise flight planner.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_init(void);

/** fp_start
 * Start the flight planner tasks.
 * @param port Listen port for the flight planner server.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_start(uint8_t port);

/** fp_read_lock
 * Lock flight planner lock for reading
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_read_lock(void);

/** fp_read_unlock
 * Unlock flight planner lock after reading
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_read_unlock(void);

/** fp_write_lock
 * Lock flight planner lock for writing
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_write_lock(void);

/** fp_write_unlock
 * Unlock flight planner lock after writing
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_write_unlock(void);

/** fp_flush
 * Flush current flight plan.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_flush(void);

/** fp_store_save
 * Store the current flight plan
 * @param path
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_store_save(char * path);

/** fp_store_load
 * Load a stored flight plan and make it the current one
 * @param path
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_store_load(char * path);

/**
 * Start server
 * @param stack number of items to allocate on stack
 * @param port port number to start server on
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_server_start(int stack, uint8_t port);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _FP_H_ */

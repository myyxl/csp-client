/**
 * @file fp_client.h
 * Flight planner client functions.
 *
 * @author Jeppe Ledet-Pedersen
 * Copyright 2011 GomSpace ApS. All rights reserved.
 */

#ifndef _FP_CLIENT_H_
#define _FP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <fp/fp.h>
#include <fp/fp_list.h>
#include <fp/fp_types.h>

/* Command line initialisation */
void cmd_fp_setup(void);

/* General Flight Planner functions */

/**
 * Set which node and port the flight planner server is listening on
 * @param node csp node
 * @param port csp port number
 * @return 0
 */
int fp_client_host(uint8_t node, uint8_t port);

/**
 * Flush current flight plan. This deletes all sequences,
 * timers, triggers and dynamic events.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_client_fp_flush(void);

/**
 * Store current flight plan to file.
 * @param filename Path where the flight plan should be stored.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_client_fp_store(char * filename);

/**
 * Load flight plan from file. This flushes the current flight
 * plan.
 * @param filename Path form where the flight plan should be loaded.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_client_fp_load(char * filename);

/* Timer functions */

/**
 * Create new timer.
 * @param timer Timer to create.
 * @param cmd Cmd to trigger when timer expires.
 * @param basis Must either be FP_TIME_ABSOLUTE or FP_TIME_INTERVAL.
 * @param state Initial sate of timer FP_TIME_ACTIVE or FP_TIME_DORMANT
 * @param when Initial timer value.
 * @param repeat Number of repetitions. Use FP_TIME_UNLIMITED for
 * recurring timer.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_client_timer_create(char * key, char * cmd, fp_timer_basis_t basis, fp_timer_state_t state, timestamp_t *when, uint16_t repeat);

/**
 * Delete timer.
 * @param timer Timer to delete.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_client_timer_delete(char * key);

/**
 * Set timer active.
 * @param timer Timer to activate.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_client_timer_set_active(char * key);

/**
 * Set timer dormant.
 * @param timer Timer to set dormant.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_client_timer_set_dormant(char * key);

int fp_client_timer_set_repeat(char * key, uint16_t count);
int fp_client_timer_set_time(char * key, fp_timer_basis_t basis, timestamp_t *when);
int fp_client_timer_set_all_active(void);
/**
 * List current timers.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_client_timer_list(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _FP_CLIENT_H_ */

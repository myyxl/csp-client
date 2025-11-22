/**
 * Flight planner timer-table
 *
 * @author Johan De Claville Christiansen
 * Copyright 2012 GomSpace ApS. All rights reserved.
 */

#ifndef _FP_TIME_H_
#define _FP_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <util/timestamp.h>

#include <fp/fp.h>
#include <fp/fp_list.h>

/** Unlimited repetitions, i.e. recurring event */
#define FP_TIME_UNLIMITED UINT16_MAX

/** Timer basis */
typedef enum {
	FP_TIME_ABSOLUTE = 0,		/**< Absolute timer (once) */
	FP_TIME_RELATIVE = 1,		/**< Relative to another timer execution */
} fp_timer_basis_t;

/** Timer state */
typedef enum {
	FP_TIME_ACTIVE = 0,		/**< Active timer */
	FP_TIME_DORMANT = 1,		/**< Dormant timer */
} fp_timer_state_t;

typedef struct {
	char timer[FP_KEY_SIZE];	/**< Timer ID */
	char cmd[FP_CMD_SIZE];		/**< Command */
	fp_timer_state_t state;		/**< Timer state */
	fp_timer_basis_t basis;		/**< Timer basis */
	timestamp_t last;		/**< Timer last execution */
	timestamp_t when;		/**< Timer expiration time */
	uint16_t repeat;		/**< Timer Repetitions */
	uint16_t remain;		/**< Timer Remaining repetitions */
} fp_timer_t;

/**
 * Initialize timer list.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_timer_init(void);

/**
 * Create new timer.
 * @param id Timer to create.
 * @param basis Timer basis. Must either be FP_TIME_ABSOLUTE or
 * FP_TIME_RELATIVE.
 * @param when Timer expiration time.
 * @param repeat Number of repetitions. Use FP_TIME_UNLIMITED
 * for at recurring timer.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_timer_create(char * key, char * cmd, fp_timer_basis_t basis, fp_timer_state_t state, timestamp_t * when, uint16_t repeat);

/**
 * Delete timer.
 * @param id Timer to delete.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_timer_delete(char * key);

/**
 * Iterate over all timers and apply function.
 * @param apply Function to apply.
 * @param ret Optional pointer to a return value.
 * @param retsize Size of return value.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_timer_iterate(fp_iter_func_t apply, void * ret, size_t retsize);

/**
 * Number of timers.
 * @return On success, the number of timers is returned.
 * On error, -1 is returned.
 */
int fp_timer_count(void);

/**
 * Search for specific timer.
 * @param id Timer to search for.
 * @return On success, a pointer to the timer is returned.
 * On error, NULL is returned.
 */
fp_timer_t * fp_timer_search(char * key);

/**
 * Get timer at position.
 * @param pos Timer position.
 * @return On success, a pointer to the timer is returned.
 * On error, NULL is returned.
 */
fp_timer_t * fp_timer_get(int pos);

/**
 * Set timer active.
 * @param id: Timer to set active.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_timer_set_active(char * key);

/**
 * Set timer dormant.
 * @param id: Timer to set dormant.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_timer_set_dormant(char * key);

/**
 * Set repeat count on timer
 * @param key id: Timer to set
 * @param count number of repeats
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_timer_set_repeat(char * key, uint16_t count);

/**
 * Set execution time on timer
 * @param key id: Timer to set
 * @param basis Timer basis. Must either be FP_TIME_ABSOLUTE or
 * FP_TIME_RELATIVE
 * @param when Timer expiration time.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_timer_set_time(char * key, uint8_t basis, timestamp_t * when);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _FP_TIME_H_ */

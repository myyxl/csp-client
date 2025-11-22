/**
 * @file fp_rwl.h
 * Flight planner Readers-Writer lock.
 * An RWL allows mutual exclusion of either
 * one writer or multiple readers.
 *
 * @author Jeppe Ledet-Pedersen
 * Copyright 2011 GomSpace ApS. All rights reserved.
 */

#ifndef _FP_RWL_H_
#define _FP_RWL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <fp/fp.h>

#include <csp/arch/csp_semaphore.h>

/** RWL structure */
typedef struct {
	uint32_t readers;
	xSemaphoreHandle reader_sem;
	xSemaphoreHandle access_sem;
} fp_rwl_t;

/** fp_rwl_init
 * Initialize lock.
 * @param lock Lock to initialize.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_rwl_init(fp_rwl_t * lock);

/** fp_rwl_write_lock
 * Lock RWL for writing.
 * @param lock Lock to lock.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_rwl_write_lock(fp_rwl_t * lock);

/** fp_rwl_write_unlock
 * Unlock RWL after writing.
 * @param lock Lock to unlock.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_rwl_write_unlock(fp_rwl_t * lock);

/** fp_rwl_read_lock
 * Lock RWL for reading.
 * @param lock Lock to lock.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_rwl_read_lock(fp_rwl_t * lock);

/** fp_rwl_read_unlock
 * Unlock RWL after reading.
 * @param lock Lock to unlock.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_rwl_read_unlock(fp_rwl_t * lock);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _FP_RWL_H_ */

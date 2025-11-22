#include <stdio.h>
#include <stdint.h>

#ifndef HK_PERSIST_H_
#define HK_PERSIST_H_

typedef struct {
	uint32_t * persist_interval;
	uint8_t * persist_en;
	uint16_t * gc_max_count;
} hk_persist_args_t;

/**
 * Initialize hk_persist task
 * @param stack_depth number of stack elemnets for hk_persis task
 * @param param_list_id id of param list, 0 for sat, 1 for gnd
 * @return
 */
void init_persist_task(uint32_t stack_depth, int param_list_id);

#endif /* HK_PERSIST_H_ */

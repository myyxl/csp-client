#include <param/param.h>
#include <uthash/utlist.h>

#ifndef HK_STORE_H_
#define HK_STORE_H_

/**
 * Add a parameter table to a hk store
 * The parameters will be serialized into a new dynamic allocated memory area
 * @param head pointer to storage pool (double pointer)
 * @param param pointer to parameter table
 * @return pointer to allocated hk element if OK
 */
param_pool_t * hk_store_add(param_index_t * param);

/**
 * Delete an element from storage pool
 * @param head pointer to storage pool (double pointer)
 * @param elem pointer to hk element to free
 */
void hk_store_del(param_pool_t ** head, param_pool_t * elem);

/**
 * Append an element to storage pool
 * @param head pointer to storage pool (double pointer)
 * @param elem pointer to hk element to append
 */
void hk_store_append(param_pool_t ** head, param_pool_t * elem);

/**
 * Truncate storage pool to max_elem elements
 * @param head pointer to storage pool (double pointer)
 * @param max_elem max hk elements to keep
 */
void hk_store_truncate(param_pool_t ** head, uint32_t max_elem);

/**
 * Iterate through list of stored elements
 * @param head pointer to storage pool
 * @param elem pointer to update for each iteration
 */
#define HK_STORE_FOREACH(head, elem) DL_FOREACH(head, elem)

/**
 * Find closest match to timestamp
 * @param head pointer to storage pool
 * @param prev pointer to previously found element (use this to save time)
 * @param timestamp time to search for
 * @return
 */
param_pool_t * hk_store_search_timestamp(param_pool_t * head, param_pool_t * prev, uint32_t timestamp);

#endif /* HK_STORE_H_ */

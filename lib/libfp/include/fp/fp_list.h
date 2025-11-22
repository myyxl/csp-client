/**
 * @file fp_list.h
 * Flight planner linked list.
 * This is a simple open-ended, doubly-linked list.
 *
 * @author Jeppe Ledet-Pedersen
 * Copyright 2011 GomSpace ApS. All rights reserved.
 */

#ifndef _FP_LIST_H_
#define _FP_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <fp/fp.h>

#define FP_LIST_ID_NONE UINT16_MAX

/** List iterate function */
typedef int (*fp_iter_func_t)(int pos, char * key, void * entry, void * ret, size_t retsize);

/** List entry structure */
typedef struct fp_list_entry_s fp_list_entry_t;
struct fp_list_entry_s {
	void * entry;
	char key[FP_KEY_SIZE];
	fp_list_entry_t * next;
	fp_list_entry_t * prev;
	uint16_t ref_count; // Not currently used
};

/** List structure */
typedef struct {
	fp_list_entry_t * head;
	fp_list_entry_t * tail;
	uint16_t count;
} fp_list_t;

/** fp_list_empty
 * Test if list is empty.
 * @param list List to test.
 * @return If list is empty, 1 is returned, otherwise 0.
 * On error, -1 is returned.
 */
int fp_list_empty(fp_list_t * list);

/** fp_list_init
 * Initialize list.
 * @param list List to initialize.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_list_init(fp_list_t * list);

/** fp_list_insert
 * Insert new item into list.
 * @param list List where item should be inserted.
 * @param pos Position in the list to insert.
 * @param key Key of inserted item.
 * @param entry Pointer to the item.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_list_insert(fp_list_t * list, int pos, char * key, void * entry);

/** fp_list_insert_unique
 * Inser unique item into list.
 * @param list List where item should be inserted.
 * @param pos Position in the list to insert.
 * @param id Id of inserted item.
 * @param entry Pointer to the item.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_list_insert_unique(fp_list_t * list, int pos, char * key, void * entry);

/** fp_list_append
 * Append item to end of list.
 * @param list List where item should be inserted.
 * @param id Id of inserted item.
 * @param entry Pointer to the item.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_list_append(fp_list_t * list, char * key, void * entry);

/** fp_list_append_unique
 * Append unique item to end of list.
 * @param list List where item should be inserted.
 * @param id Id of inserted item.
 * @param entry Pointer to the item.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_list_append_unique(fp_list_t * list, char * key, void * entry);

/** fp_list_search
 * Search list for item.
 * @param list List to search.
 * @param id Id of item to search.
 * @return On success, a pointer to the item is returned.
 * On error, NULL is returned.
 */
void * fp_list_search(fp_list_t * list, char * key);

/** fp_list_get
 * Get item at position.
 * @param list List to get from.
 * @param pos Item position
 * @return On success, a pointer to the item is returned.
 * On error, NULL is returned.
 */
void * fp_list_get(fp_list_t * list, int pos);

/** fp_list_remove
 * Remove item from list.
 * @param list List to remove item from.
 * @param id Item Id to remove.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_list_remove(fp_list_t * list, char * key);

/** fp_list_count
 * Number of items in list.
 * @return On success, the number of items is returned.
 * On error, -1 is returned.
 */
int fp_list_count(fp_list_t * list);

/** fp_list_iterate
 * Iterate over all items and apply function.
 * @param list List to iterate.
 * @param apply Function to apply.
 * @param ret Optional pointer to a return value.
 * @param retsize Size of return value.
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int fp_list_iterate(fp_list_t * list, fp_iter_func_t apply, void * ret, size_t retsize);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _FP_LIST_H_ */

#ifndef INCLUDE_HK_DB_H_
#define INCLUDE_HK_DB_H_

#include <param/param_types.h>

/* Opaque pointer */
struct hk_db_instance;

/**
 * Call this to start a bulk database insertion
 * @return instance of db
 */
struct hk_db_instance * hk_db_begin(void);

/**
 * Call this to insert to the query
 * @param db pointer to instance
 * @param time
 * @param param
 * @param addr
 * @param node
 * @param table
 * @param data
 */
void hk_db_add(struct hk_db_instance * db, uint32_t time, const param_table_t * param, uint16_t addr, int node, int table, void * data);

/**
 * When query is finished with begin and add call commit to run the query
 * This will also free the instance object
 * @param db
 */
void hk_db_commit(struct hk_db_instance * db);

#endif /* INCLUDE_HK_DB_H_ */

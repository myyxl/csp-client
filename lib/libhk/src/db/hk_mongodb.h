#ifndef HK_MONGODB_H_
#define HK_MONGODB_H_

#include <mongoc.h>
mongoc_bulk_operation_t * hk_mongodb_begin(void);
void hk_mongodb_add(mongoc_bulk_operation_t * bulk, uint32_t time, const param_table_t * param, uint16_t addr, int node, int table, void * data);
void hk_mongodb_add_latest(mongoc_bulk_operation_t * bulk, uint32_t time, const param_table_t * param, uint16_t addr, int node, int table, void * data);
void hk_mongodb_commit(mongoc_bulk_operation_t * bulk, const char * collection);

#endif /* HK_MONGODB_H_ */

#ifndef HK_INDEX_H_
#define HK_INDEX_H_

#include <param/param.h>

typedef struct {
	const uint16_t * addrs;
	int count;
	uint16_t param_interval;
	uint8_t node;
	uint8_t type;
	int (*enabled_fct)(void);
} hk_beacon_t;

typedef struct {
	uint8_t node;
	uint8_t tableid;
	uint8_t memid;
	uint16_t param_interval;
	const param_table_t * table;
	int (*collect_fct)(void);
	int (*init_fct)(void);
	int size;
	unsigned int count;
	const char * nodename;
} hk_table_t;

extern hk_table_t hk_tables[];
extern int hk_tables_count;

extern hk_beacon_t hk_beacons[];
extern int hk_beacons_count;

#endif /* HK_INDEX_H_ */

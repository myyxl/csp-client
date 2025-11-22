#ifndef HK_INFLUXDB_H_
#define HK_INFLUXDB_H_

#define INFLUX_MAX_QUERY_LEN 10000

struct influxdb_bulk {
	int len;
	char buf[INFLUX_MAX_QUERY_LEN];
};

struct influxdb_bulk * hk_influxdb_begin(void);
void hk_influxdb_add(struct influxdb_bulk * bulk, uint32_t time, const param_table_t * param, uint16_t addr, int node, int table, void * data);
void hk_influxdb_commit(struct influxdb_bulk * bulk);

#endif /* HK_INFLUXDB_H_ */

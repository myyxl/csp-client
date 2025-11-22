#include <stdio.h>
#include <stdint.h>

#ifndef HK_BCN_FILE_H_
#define HK_BCN_FILE_H_

/**
 * Open beacon file for write/append
 * @param filename filename for beacon file
 * @param append whether to append or create new file/truncate existing
 * @return valid file stream pointer if ok, NULL on error
 */
FILE * hk_bcn_file_open(char *filename, int append);

/**
 * Open beacon file for read
 * @param filename filename for beacon file
 * @return valid file stream pointer if ok, NULL on error
 */
FILE * hk_bcn_file_open_read(char *filename);

/**
 * Close beacon file
 * @param fp pointer to file stream to close
 * @return 0 if ok, -1 on error
 */
int hk_bcn_file_close(FILE *fp);

/**
 * Append beacon record to beacon file
 * @param fp pointer to file stream for appending
 * @param timestamp timestamp for this beacon record
 * @param node node id for beacon record
 * @param type type for beacon record
 * @param length length of data
 * @param data pointer to data
 * @return 0 if ok, -1 on error
 */
int hk_bcn_file_append(FILE *fp, uint32_t timestamp, uint8_t node, uint8_t type, uint16_t length, void * data);

/**
 * Read beacon record from beacon file
 * @param fp pointer to file stream for reading
 * @param timestamp pointer to timestamp for this beacon record
 * @param node pointer node id for beacon record
 * @param type pointer to type for beacon record
 * @param length pointer to length of data
 * @param data pointer to data
 * @param max_length max length of data
 * @return 0 if ok, -1 on error
 */
int hk_bcn_file_read(FILE *fp, uint32_t * timestamp, uint8_t * node, uint8_t * type, uint16_t * length, void * data, uint16_t max_length);

#endif /* HK_BCN_FILE_H_ */

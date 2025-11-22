#include <param/param.h>
#include <uthash/utlist.h>

#ifndef HK_FILE_H_
#define HK_FILE_H_

/**
 * Save hk serialized and packed data to file
 * @param param pointer to param index to save
 * @param filename pointer to filename, if NULL, default filename is used
 * @param max_elem maximum number of elements to save
 * @return
 */
int hk_file_save(param_index_t * param, char * filename, uint32_t max_elem);

/**
 * Load hk serialized and packed data from file
 * @param param pointer to param index to save
 * @param filename pointer to filename, if NULL, default filename is used
 * @param max_elem maximum number of elements to load
 * @return
 */
int hk_file_load(param_index_t * param, char * filename, uint32_t max_elem);

#endif /* HK_FILE_H_ */

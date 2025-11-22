/**
 * Satellite logging client
 *
 * @author Jeppe Ledet-Pedersen
 * Copyright 2010 GomSpace ApS. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <alloca.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>

#include <util/timestamp.h>
#include <util/byteorder.h>
#include <util/clock.h>
#include <util/color_printf.h>
#include <uthash/uthash.h>

#include <log/log.h>
#include <conf_log.h>

#include "../pack/log_pack.h"

extern log_group_t __start_log_groups;
extern log_group_t __stop_log_groups;

LOG_GROUP_VERBOSE(log_default, "default");
log_group_t * LOG_DEFAULT = &log_default_s;

log_group_t * log_find_group(const char * group_name) {
	log_group_t * found_group = NULL;
	for (log_group_t * group = &__start_log_groups; group != (log_group_t *) &__stop_log_groups; group++) {
		if (strcmp(group->name, group_name) == 0) {
			found_group = group;
		}
	}
	return found_group;
}

#define MASK_SET	0
#define MASK_AND	1
#define MASK_OR		2

/* If op is MASK_SET, set all level bits equal or lover */
#define mask_set(_mask, _op) \
	(_op == MASK_SET ? LOG_ALL_MASK & ~(_mask - 1) : _mask)

int log_string_to_mask(char *str, uint8_t old)
{
	char *token, *saveptr;
	uint8_t mask;
	int op;

	token = strtok_r(str, ",", &saveptr);
	while (token != NULL) {
		/* Check prefix */
		if (*token == '+') {
			op = MASK_OR;
			token++;
		} else if (*token == '-') {
			op = MASK_AND;
			token++;
		} else {
			op = MASK_SET;
		}

		/* Check length */
		if (strlen(token) < 1)
			return -1;

		/* Check mask */
		if (!strncasecmp(token, "trace", strlen(token))) {
			mask = mask_set(LOG_TRACE_MASK, op);
		} else if (!strncasecmp(token, "debug", strlen(token))) {
			mask = mask_set(LOG_DEBUG_MASK, op);
		} else if (!strncasecmp(token, "info", strlen(token))) {
			mask = mask_set(LOG_INFO_MASK, op);
		} else if (!strncasecmp(token, "warning", strlen(token))) {
			mask = mask_set(LOG_WARNING_MASK, op);
		} else if (!strncasecmp(token, "error", strlen(token))) {
			mask = mask_set(LOG_ERROR_MASK, op);
		} else if (!strncasecmp(token, "standard", strlen(token))) {
			mask = LOG_DEFAULT_MASK;
			op = MASK_SET;
		} else if (!strncasecmp(token, "all", strlen(token))) {
			mask = LOG_ALL_MASK;
			op = MASK_SET;
		} else if (!strncasecmp(token, "none", strlen(token))) {
			mask = 0;
			op = MASK_SET;
		} else {
			return -1;
		}

		/* Apply operation */
		if (op == MASK_OR) {
			old |= mask;
		} else if (op == MASK_AND) {
			old &= ~mask;
		} else if (op == MASK_SET) {
			old = mask;
		}

		token = strtok_r(NULL, ",", &saveptr);
	}

	return old;
}

/* Set print log levels from string. The format is:
 * group=[+-]level[,[+-]level][;group...] */
int log_set_from_string(const char *str)
{
	char *token, *group, *groupstr, *maskstr, *sp_entry, *sp_group;
	char *strbuf = NULL;
	int mask = 0;

	/* strtok writes to the string, so we need to duplicate
	 * it to avoid writing to read-only memory */
	strbuf = strndup(str, 100);
	if (!strbuf) {
		log_warning("failed to allocate log string buffer");
		return -1;
	}

	token = strtok_r(strbuf, ";", &sp_entry);
	while (token != NULL) {
		/* Find '=' sign */
		maskstr = strchr(token, '=');
		if (!maskstr) {
			log_warning("invalid format: %s", token);
			goto next;
		}
		groupstr = token;

		/* Skip and null '=' sign */
		*maskstr++ = '\0';

		/* Loop over groups */
		group = strtok_r(groupstr, ",", &sp_group);
		while (group != NULL) {
			/* Find current mask if other group than all */
			if (strcmp(group, "all")) {
				mask = log_get_mask(group, 1);
				if (mask < 0) {
					log_warning("invalid group: %s", group);
					goto next;
				}
			} else {
				mask = 0;
			}

			/* Calculate new mask */
			mask = log_string_to_mask(maskstr, mask);
			if (mask < 0) {
				log_warning("invalid mask: %s", maskstr);
				goto next;
			}

			/* Update mask */
			log_set_mask(group, (uint8_t)mask, 1);

			/* Next group */
			group = strtok_r(NULL, ",", &sp_group);
		}
next:
		/* Next entry */
		token = strtok_r(NULL, ";", &sp_entry);
	}

	free(strbuf);

	return 0;
}

void log_print_groups(void) {
	printf("Group           Print Store\r\n");
	for (log_group_t * group = &__start_log_groups; group != (log_group_t *) &__stop_log_groups; group++) {

		printf("%-15s %c%c%c%c%c %c%c%c%c%c\r\n", group->name,
		       /* Print masks */
		       (group->print_mask & LOG_ERROR_MASK)    ? 'E' : '.',
		       (group->print_mask & LOG_WARNING_MASK)  ? 'W' : '.',
		       (group->print_mask & LOG_INFO_MASK)     ? 'I' : '.',
		       (group->print_mask & LOG_DEBUG_MASK)    ? 'D' : '.',
		       (group->print_mask & LOG_TRACE_MASK)    ? 'T' : '.',
		       /* Store masks */
		       (group->store_mask & LOG_ERROR_MASK)    ? 'E' : '.',
		       (group->store_mask & LOG_WARNING_MASK)  ? 'W' : '.',
		       (group->store_mask & LOG_INFO_MASK)     ? 'I' : '.',
		       (group->store_mask & LOG_DEBUG_MASK)    ? 'D' : '.',
		       (group->store_mask & LOG_TRACE_MASK)    ? 'T' : '.');
	}
}

int log_get_mask(char * name, int is_print_mask)
{
	log_group_t * group = log_find_group(name);
	if (group == NULL)
		return -1;

	if (is_print_mask) {
		return group->print_mask;
	} else {
		return group->store_mask;
	}
}

void log_set_mask(char * name, uint8_t mask, int is_print_mask) {

	/* For all */
	if (strcmp(name, "all") == 0) {
		for (log_group_t * group = &__start_log_groups; group != (log_group_t *) &__stop_log_groups; group++) {
			if (is_print_mask) {
				group->print_mask = mask;
			} else {
				group->store_mask = mask;
			}
		}
		return;
	}

	log_group_t * group = log_find_group(name);
	if (group == NULL)
		return;

	if (is_print_mask) {
		group->print_mask = mask;
	} else {
		group->store_mask = mask;
	}

}

void log_set_printmask(char * name, uint8_t mask) {
	log_set_mask(name, mask, 1);
}

void log_set_storemask(char * name, uint8_t mask) {
	log_set_mask(name, mask, 0);
}

int log_event_group_args(log_level_t level, log_group_t * group, int do_print, int do_store, const char * format, va_list args) {

	/* Get time as close as possible to the event */
	timestamp_t ts;
	clock_get_time(&ts);

	/**
	 * Store in a nice compact format
	 */
#ifdef ENABLE_LOG_STORE
	if (do_store) {

		va_list store_args;
		va_copy(store_args, args);

		/* Try to pack data on stack */
		log_store_t * store = alloca(100);

		store->level = level;
		store->group = group;
		store->format = format;
		store->sec = ts.tv_sec;
		store->nsec = ts.tv_nsec;

		store->len = log_pack(store->data, 80, format, store_args);

		va_end(store_args);

		log_store((void *) store, store->len + offsetof(log_store_t, data));

	}
#endif

	/**
	 * Print out nicely
	 */
	if (do_print) {

		/* Start color */
		switch (level) {
			default:
			case LOG_TRACE: printf("\E[0;35m"); break;	/* Magenta */
			case LOG_DEBUG: printf("\E[0;34m"); break;	/* Blue */
			case LOG_INFO: printf("\E[0;32m"); break;	/* Green */
			case LOG_WARNING: printf("\E[0;33m"); break;	/* Yellow */
			case LOG_ERROR: printf("\E[1;31m"); break;	/* Red */
		}

		/* Print time and group name */
		printf("%04"PRIu32".%03"PRIu32" %s: ", ts.tv_sec, ts.tv_nsec / 1000000, group->name);

		/* Print log message */
		vprintf(format, args);

		/* End color */
		printf("\E[0m\r\n");

	}

	return 0;

}

int log_event_group(log_level_t level, log_group_t * group, const char * format, ...) {

	if (group == NULL)
		group = LOG_DEFAULT;

	int do_print = ((group->print_mask & (1 << level)) > 0);
	int do_store = ((group->store_mask & (1 << level)) > 0);

	if (do_print == 0 && do_store == 0)
		return -1;

	va_list va_args;
	va_start(va_args, format);
	int result = log_event_group_args(level, group, do_print, do_store, format, va_args);
	va_end(va_args);
	return result;

}

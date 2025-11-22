/**
 * @file ftp_client.c
 * FTP Client API for X86
 *
 * @author Jeppe Ledet-Pedersen & Johan De Claville Christiansen
 * Copyright 2011 GomSpace ApS. All rights reserved.
 */

#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include <csp/csp.h>
#include <csp/csp_endian.h>

#include <ftp/ftp_types.h>

#include <util/crc32.h>
#include <util/color_printf.h>
#include <util/bytesize.h>
#include <util/log.h>
#include <util/clock.h>

#include <ftp/ftp_client.h>

static int ftp_timeout = 30000;

/* Chunk status markers */
static const char const * packet_missing = "-";
static const char const * packet_ok = "+";

static FILE * fp, * fp_map;
static csp_conn_t * conn = NULL;

static int ftp_chunk_size = 175;
static uint32_t ftp_file_size = 0;
static uint32_t ftp_chunks = 0;
static uint32_t ftp_checksum = 0xABCD0123;
char ftp_file_name[FTP_PATH_LENGTH];

static ftp_status_element_t last_status[FTP_STATUS_CHUNKS];
static uint32_t last_entries = 0;
static ftp_progress_handler progress_handler = NULL;
static void* progress_handler_data = NULL;

static double timespec_diff(timestamp_t * start, timestamp_t * end) {
	struct timespec temp;
	if (((double) end->tv_nsec - (double) start->tv_nsec) < 0) {
		temp.tv_sec = end->tv_sec-start->tv_sec - 1;
		temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
	} else {
		temp.tv_sec = end->tv_sec - start->tv_sec;
		temp.tv_nsec = end->tv_nsec - start->tv_nsec;
	}
	return (double)(temp.tv_sec + (double)temp.tv_nsec/1000000000);
}

static int ftp_perror(ftp_return_t ret) {
	switch (ret) {
	case FTP_RET_OK:
		color_printf(COLOR_GREEN, "No error\r\n");
		break;
	case FTP_RET_PROTO:
		color_printf(COLOR_RED, "Protocol error\r\n");
		break;
	case FTP_RET_NOENT:
		color_printf(COLOR_YELLOW, "No such file or directory\r\n");
		break;
	case FTP_RET_INVAL:
		color_printf(COLOR_YELLOW, "Invalid argument\r\n");
		break;
	case FTP_RET_NOSPC:
		color_printf(COLOR_RED, "No space left on device\r\n");
		break;
	case FTP_RET_IO:
		color_printf(COLOR_RED, "IO error\r\n");
		break;
	case FTP_RET_FBIG:
		color_printf(COLOR_RED, "File to large\r\n");
		break;
	case FTP_RET_EXISTS:
		color_printf(COLOR_YELLOW, "File exists\r\n");
		break;
	case FTP_RET_NOTSUP:
		color_printf(COLOR_RED, "Operation not supported\r\n");
		break;
	case FTP_RET_BUSY:
		color_printf(COLOR_RED, "Device or resource busy\r\n");
		break;
	default:
		color_printf(COLOR_RED, "Unknown %d\r\n", ret);
		break;
	}
	return 0;
}

static char *sec_to_time(int total)
{
	static char buf[20];
	int hour, min, sec;

	hour = total / 3600;
	total -= hour * 3600;

	min = total / 60;
	total -= min * 60;

	sec = total;
	if (hour) {
		sprintf(buf, "%uh %um %us", hour, min, sec);
	} else if (min) {
		sprintf(buf, "%um %us", min, sec);
	} else if (sec) {
		sprintf(buf, "%us", sec);
	} else {
		strcpy(buf, "");
	}
	return buf;
}

static int ftp_file_crc32(uint32_t *crc_arg)
{
	size_t i, bytes;
	char buf[128];
	uint32_t crc = 0xFFFFFFFF;

	fseek(fp, 0, SEEK_SET);
	do {
		bytes = fread(buf, 1, sizeof(buf), fp);
		for (i = 0; i < bytes; i++)
			crc = chksum_crc32_step(crc, buf[i]);
	} while (bytes > 0);

	crc = (crc ^ 0xFFFFFFFF);
	*crc_arg = crc;

	return 0;
}

void ftp_set_progress_handler(ftp_progress_handler handler, void *data)
{
	progress_handler = handler;
	progress_handler_data = data;
}

int ftp_upload(uint8_t host, uint8_t port, const char * path, uint8_t backend, int chunk_size, uint32_t addr, const char * remote_path, uint32_t * size, uint32_t * checksum) {

	int req_length, rep_length;
	ftp_packet_t req, rep;

	/* Open file and read size */
	fp = fopen(path, "rb");
	if (fp == NULL) {
		color_printf(COLOR_RED, "Failed to open file %s\r\n", path);
		return -1;
	}
	struct stat statbuf;
	stat(path, &statbuf);
	color_printf(COLOR_GREEN, "File size is %u\r\n", (unsigned int) statbuf.st_size);

	/* Calculate CRC32 */
	ftp_file_crc32(&ftp_checksum);
	color_printf(COLOR_GREEN, "Checksum is %#010"PRIx32"\r\n", ftp_checksum);

	/* Copy checksum and length */
	if (checksum != NULL)
		*checksum = ftp_checksum;
	if (size != NULL)
		*size = statbuf.st_size;

	ftp_chunk_size = chunk_size;
	ftp_file_size = (uint32_t) statbuf.st_size;
	ftp_chunks = (ftp_file_size + ftp_chunk_size - 1) / ftp_chunk_size;
	strncpy(ftp_file_name, path, FTP_PATH_LENGTH);

	/* Assemble upload request */
	req.type = FTP_UPLOAD_REQUEST;
	req.up.chunk_size = csp_hton16(ftp_chunk_size);
	req.up.size = csp_hton32(ftp_file_size);
	req.up.crc32 = csp_hton32(ftp_checksum);
	req.up.mem_addr = csp_hton32(addr);
	req.up.backend = backend;
	strncpy(req.up.path, remote_path, FTP_PATH_LENGTH);

	req_length = sizeof(ftp_type_t) + sizeof(ftp_upload_request_t);
	rep_length = sizeof(ftp_type_t) + sizeof(ftp_upload_reply_t);

	conn = csp_connect(CSP_PRIO_NORM, host, port, ftp_timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL)
		return -1;

	if (csp_transaction_persistent(conn, ftp_timeout, &req, req_length, &rep, rep_length) != rep_length) {
		color_printf(COLOR_RED, "No reply to upload request received, timeout or error (timeout set to %d msec)\r\n", ftp_timeout);
		return -1;
	}

	if (rep.type != FTP_UPLOAD_REPLY || rep.uprep.ret != FTP_RET_OK) {
		ftp_perror(rep.uprep.ret);
		return -1;
	}

	return 0;

}

int ftp_download(uint8_t host, uint8_t port, const char * path, uint8_t backend, int chunk_size, uint32_t memaddr, uint32_t memsize, const char * remote_path, uint32_t * size) {

	int req_length, rep_length;
	ftp_packet_t req, rep;
	bool new_file = false;

	ftp_chunk_size = chunk_size;

	req.type = FTP_DOWNLOAD_REQUEST;
	req.down.chunk_size = csp_hton16(ftp_chunk_size);
	req.down.mem_addr = csp_hton32(memaddr);
	req.down.mem_size = csp_hton32(memsize);
	req.down.backend = backend;

	if (remote_path != NULL)
		strncpy(req.down.path, remote_path, FTP_PATH_LENGTH);
	else
		memset(req.down.path, 0, FTP_PATH_LENGTH);

	req.down.path[FTP_PATH_LENGTH - 1] = '\0';
	strncpy(ftp_file_name, path, FTP_PATH_LENGTH);
	ftp_file_name[FTP_PATH_LENGTH - 1] = '\0';

	req_length = sizeof(ftp_type_t) + sizeof(ftp_download_request_t);
	rep_length = sizeof(ftp_type_t) + sizeof(ftp_download_reply_t);

	conn = csp_connect(CSP_PRIO_NORM, host, port, 5000, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL)
		return -1;

	int res_length = csp_transaction_persistent(conn, ftp_timeout, &req, req_length, &rep, rep_length);
	if (res_length != rep_length) {
		color_printf(COLOR_RED, "Length mismatch. Expected %d, got %d\r\n", rep_length, res_length);
		return -1;
	}

	if (rep.type != FTP_DOWNLOAD_REPLY || rep.downrep.ret != FTP_RET_OK) {
		ftp_perror(rep.downrep.ret);
		return -1;
	}

	ftp_file_size = csp_ntoh32(rep.downrep.size);
	ftp_checksum = csp_ntoh32(rep.downrep.crc32);
	ftp_chunks = (ftp_file_size + ftp_chunk_size - 1) / ftp_chunk_size;
	*size = ftp_file_size;

	color_printf(COLOR_GREEN, "File size is %"PRIu32"\r\n", ftp_file_size);
	color_printf(COLOR_GREEN, "Checksum is %#010"PRIx32"\r\n", ftp_checksum);

	/* Map file name */
	char map[100];

	/* Try to open file */
	fp = fopen((const char *) path, "r+");
	if (fp == NULL) {
		/* Create new file */
		new_file = true;
		fp = fopen((const char *) path, "w+");
		if (fp == NULL) {
			color_printf(COLOR_RED, "Client: Failed to create data file\r\n");
			return -1;
		}
	}

	/* Try to create a new bitmap */
	sprintf(map, "%s.map", path);

	/* Check if file already exists */
	fp_map = fopen(map, "r+");
	if (fp_map == NULL) {
		unsigned int i;

		/* Abort if data file exists but map doesn't */
		if (!new_file) {
			color_printf(COLOR_RED, "%s already exists\r\n", path);
			fclose(fp);
			fp = NULL;
			return FTP_RET_EXISTS;
		}

		/* Create new file */
		fp_map = fopen(map, "w+");
		if (fp_map == NULL) {
			color_printf(COLOR_RED, "Failed to create bitmap\r\n");
			fclose(fp);
			fp = NULL;
			return FTP_RET_IO;
		}

		/* Clear contents */
		for (i = 0; i < ftp_chunks; i++) {
			if (fwrite(packet_missing, 1, 1, fp_map) < 1) {
				color_printf(COLOR_RED, "Failed to clear bitmap\r\n");
				fclose(fp_map);
				fclose(fp);
				fp = NULL;
				fp_map = NULL;
				return -1;
			}
		}

		fflush(fp_map);
		fsync(fileno(fp_map));
	}

	return 0;

}

int ftp_status_request(void) {

	/* Request */
	int req_length;
	ftp_packet_t req, rep;
	req.type = FTP_STATUS_REQUEST;

	req_length = sizeof(ftp_type_t);

	/* Transaction */
	if (csp_transaction_persistent(conn, ftp_timeout, &req, req_length, &rep, -1) == 0) {
		color_printf(COLOR_RED, "Failed to receive status reply\r\n");
		return -1;
	}

	if (rep.type != FTP_STATUS_REPLY || rep.statusrep.ret != FTP_RET_OK) {
		color_printf(COLOR_RED, "Reply was not STATUS_REPLY\r\n");
		return -1;
	}

	rep.statusrep.complete = csp_ntoh32(rep.statusrep.complete);
	rep.statusrep.total = csp_ntoh32(rep.statusrep.total);
	rep.statusrep.entries = csp_ntoh16(rep.statusrep.entries);

	color_printf(COLOR_BLUE, "Transfer Status: ");
	color_printf(COLOR_BLUE, "%u of %u (%.2f%%)\r\n",
			rep.statusrep.complete, rep.statusrep.total,
			(double) rep.statusrep.complete * 100 / (double) rep.statusrep.total);

	if (rep.statusrep.complete != rep.statusrep.total) {
		int i;
		for (i = 0; i < rep.statusrep.entries; i++) {
			rep.statusrep.entry[i].next = csp_ntoh32(rep.statusrep.entry[i].next);
			rep.statusrep.entry[i].count = csp_ntoh32(rep.statusrep.entry[i].count);
			last_status[i].next = rep.statusrep.entry[i].next;
			last_status[i].count = rep.statusrep.entry[i].count;
		}

		last_entries = rep.statusrep.entries;
	} else {
		last_entries = 0;
	}

	return 0;

}

#define PROGRESS_WIDTH	40

static double bps = 0.0;
static timestamp_t now, last, last_speed;
static uint32_t last_chunk = 0, eta_sec = 0;

static void progress_reset(void)
{
	bps = 0.0;
	memset(&now, 0, sizeof(now));
	memset(&last, 0, sizeof(last));
	memset(&last_speed, 0, sizeof(last_speed));
	last_chunk = 0;
	eta_sec = 0;
}

static void progress_bar(uint32_t chunk, bool clear)
{
	double sec, speed_sec, alpha = 0.25;
	char buf[200] = "";
	int index = 0;

	clock_get_monotonic(&now);
	sec = timespec_diff(&last, &now);
	speed_sec = timespec_diff(&last_speed, &now);

	/* Update progress bar every 100 ms and for last chunk */
	if (sec > 1.0 || chunk == ftp_chunks - 1) {

		double p = ((double)chunk / (double)(ftp_chunks - 1));
		int k, l = (int)(PROGRESS_WIDTH * p);

		/* Clear line */
		if (clear)
			index += snprintf(&buf[index], sizeof(buf) - index, "\033[1K\r");

		index += snprintf(&buf[index], sizeof(buf) - index, "%5.1f%% [", p * 100.0);
		for (k = 0; k < l; k++)
			index += snprintf(&buf[index], sizeof(buf) - index, "#");
		for (k = 0; k < PROGRESS_WIDTH - l; k++)
			index += snprintf(&buf[index], sizeof(buf) - index, " ");
		index += snprintf(&buf[index], sizeof(buf) - index, "] ");

		/* Show received chunks */
		index += snprintf(&buf[index], sizeof(buf) - index, "(%d/%d) ", (int)chunk + 1, (int)ftp_chunks);

		/* Update speed and ETA */
		if (speed_sec >= 1.0) {
			/* Exponentially smoothed bytes per second */
			double this_bps = (((chunk + 1 - last_chunk) * ftp_chunk_size) / speed_sec);
			bps = bps > 0 ? this_bps * alpha + bps * (1 - alpha) : this_bps;

			/* Remaining time */
			eta_sec = ((ftp_chunks - chunk + 1) * ftp_chunk_size) / bps;

			/* Update last time */
			last_chunk = chunk;
			last_speed.tv_sec = now.tv_sec;
			last_speed.tv_nsec = now.tv_nsec;
		}

		/* Show speed and ETA if not last chunk */
		if (last_chunk > 0 && chunk != ftp_chunks - 1) {
			/* Show speed */
			index += snprintf(&buf[index], sizeof(buf) - index, "%4.1f kB/s ", bps / 1024);
			index += snprintf(&buf[index], sizeof(buf) - index, "eta %s ", sec_to_time(eta_sec));
		}

		if (!clear)
			index += snprintf(&buf[index], sizeof(buf) - index, "\r\n");

		buf[sizeof(buf) - 1] = '\0';
		color_printf(COLOR_BLUE, "%s", buf);
		fflush(stdout);

		/* Update last time */
		last.tv_sec = now.tv_sec;
		last.tv_nsec = now.tv_nsec;
	}
}

int ftp_status_reply(void) {

	ftp_packet_t ftp_packet;
	ftp_status_reply_t * status = (ftp_status_reply_t *) &ftp_packet.statusrep;
	ftp_packet.type = FTP_STATUS_REPLY;

	/* Build status reply */
	unsigned int i = 0, next = 0, count = 0;

	status->entries = 0;
	status->complete = 0;
	for (i = 0; i < ftp_chunks; i++) {
		int s;
		char cstat;

		/* Read chunk status */
		if ((unsigned int) ftell(fp_map) != i) {
			if (fseek(fp_map, i, SEEK_SET) != 0) {
				color_printf(COLOR_RED, "fseek failed\r\n");
				return -1;
			}
		}
		if (fread(&cstat, 1, 1, fp_map) != 1) {
			color_printf(COLOR_RED, "fread byte %u failed\r\n", i);
			return -1;
		}

		s = (cstat == *packet_ok);

		/* Increase complete counter if chunk was received */
		if (s) status->complete++;

		/* Add chunk status to packet */
		if (status->entries < FTP_STATUS_CHUNKS) {
			if (!s) {
				if (!count)
					next = i;
				count++;
			}

			if (count > 0 && (s || i == ftp_chunks - 1)) {
				status->entry[status->entries].next = csp_hton32(next);
				status->entry[status->entries].count = csp_hton32(count);
				status->entries++;
			}
		}
	}

	color_printf(COLOR_BLUE, "Transfer Status: ");
	color_printf(COLOR_BLUE, "%u of %u (%.2f%%)\r\n",
			status->complete, ftp_chunks,
			(double) status->complete * 100 / (double) ftp_chunks);

	status->entries = csp_hton16(status->entries);
	status->complete = csp_hton32(status->complete);
	status->total = csp_hton32(ftp_chunks);
	status->ret = FTP_RET_OK;

	/* Send reply */
	if (csp_transaction_persistent(conn, ftp_timeout, &ftp_packet, sizeof(ftp_type_t) + sizeof(ftp_status_reply_t), NULL, 0) != 1) {
		color_printf(COLOR_RED, "Failed to send status reply\r\n");
		return -1;
	}

	/* Reset progress bar */
	progress_reset();

	/* Read data */
	csp_packet_t * packet;
	while (1) {
		packet = csp_read(conn, ftp_timeout);

		if (!packet) {
			color_printf(COLOR_RED, "Timeout while waiting for data\r\n");
			return -1;
		}

		ftp_packet_t * ftp_packet = (ftp_packet_t *) &packet->data;

		ftp_packet->data.chunk = csp_ntoh32(ftp_packet->data.chunk);
		unsigned int size;

		if (ftp_packet->type == FTP_RET_IO) {
			color_printf(COLOR_RED, "Server failed to read chunk\r\n");
			csp_buffer_free(packet);
			return -1;
		}

		if (ftp_packet->data.chunk >= ftp_chunks) {
			color_printf(COLOR_RED, "Bad chunk number %u > %u\r\n", ftp_packet->data.chunk, ftp_chunks);
			csp_buffer_free(packet);
			continue;
		}

		if (ftp_packet->data.chunk == ftp_chunks - 1) {
			size = ftp_file_size % ftp_chunk_size;
			if (size == 0)
				size = ftp_chunk_size;
		} else {
			size = ftp_chunk_size;
		}

		if ((unsigned int) ftell(fp) != ftp_packet->data.chunk * ftp_chunk_size) {
			if (fseek(fp, ftp_packet->data.chunk * ftp_chunk_size, SEEK_SET) != 0) {
				color_printf(COLOR_RED, "Seek error\r\n");
				csp_buffer_free(packet);
				return -1;
			}
		}

		if (fwrite(ftp_packet->data.bytes, 1, size, fp) != size) {
			color_printf(COLOR_RED, "Write error\r\n");
			csp_buffer_free(packet);
			return -1;
		}
		fflush(fp);

		if ((unsigned int) ftell(fp_map) != ftp_packet->data.chunk) {
			if (fseek(fp_map, ftp_packet->data.chunk, SEEK_SET) != 0) {
				color_printf(COLOR_RED, "Map Seek error\r\n");
				return -1;
			}
		}

		if (fwrite(packet_ok, 1, 1, fp_map) != 1) {
			color_printf(COLOR_RED, "Map write error\r\n");
			return -1;
		}
		fflush(fp_map);

		/* Show progress bar */
		progress_bar(ftp_packet->data.chunk, true);

		/* Free buffer element */
		csp_buffer_free(packet);

		/* Break if all packets were received */
		if (ftp_packet->data.chunk == ftp_chunks - 1)
			break;
	}

	color_printf(COLOR_NONE, "\r\n");

	/* Sync file to disk */
	fflush(fp);
	fsync(fileno(fp));

	fflush(fp_map);
	fsync(fileno(fp_map));

	return 0;
}

int ftp_data(int count) {

	unsigned int i, j;
	int ret;

	/* Reset progress bar */
	progress_reset();

	/* Request */
	ftp_packet_t packet;
	packet.type = FTP_DATA;
	for (i = 0; i < last_entries; i++) {
		ftp_status_element_t * n = &last_status[i];

		for (j = 0; j < n->count; j++) {
			/* Calculate chunk number */
			packet.data.chunk = n->next + j;

			/* Print progress bar */
			progress_bar(packet.data.chunk, true);

			/* Read chunk */
			if ((unsigned int) ftell(fp) != packet.data.chunk * ftp_chunk_size)
				fseek(fp, packet.data.chunk * ftp_chunk_size, SEEK_SET);
			ret = fread(packet.data.bytes, ftp_chunk_size, 1, fp);
			if (ret < 0) {
				if (!feof(fp))
					break;
			}

			/* Chunk number MUST be little-endian!
			 * Note: Yes, this is due to an old mistake, and now we are stuck with it! :( */
			packet.data.chunk = csp_htole32(packet.data.chunk);

			/* Send data */
			int length = sizeof(ftp_type_t) + sizeof(uint32_t) + ftp_chunk_size;
			if (csp_transaction_persistent(conn, ftp_timeout, &packet, length, NULL, 0) != 1) {
				color_printf(COLOR_RED, "Data transaction failed\r\n");
				csp_close(conn);
				break;
			}
		}

		color_printf(COLOR_NONE, "\r\n");

	}

	return 0;

}

int ftp_done(unsigned int remove_map) {

	/* Close file and map */
	if (fp_map) {
		fclose(fp_map);
		fp_map = NULL;
	}
	if (fp) {
		fclose(fp);
		fp = NULL;
	}

	/* Delete map file if it exits */
	if (remove_map) {
		char map[FTP_PATH_LENGTH];
		snprintf(map, FTP_PATH_LENGTH, "%s.map", ftp_file_name);
		int status = remove(map);
		if (status != 0)
			color_printf(COLOR_RED, "Failed to remove %s %d\r\n", map, status);
	}

	/* Request */
	ftp_packet_t packet;
	packet.type = FTP_DONE;

	csp_transaction_persistent(conn, ftp_timeout, &packet, sizeof(ftp_type_t), NULL, 0);
	csp_close(conn);
	conn = NULL;

	last_entries = 0;
	progress_handler = NULL;
	progress_handler_data = NULL;
	return 0;

}

int ftp_crc(void) {

	/* Request */
	ftp_packet_t packet;
	packet.type = FTP_CRC_REQUEST;

	/* Reply */
	int repsiz = sizeof(ftp_type_t) + sizeof(ftp_crc_reply_t);
	if (csp_transaction_persistent(conn, ftp_timeout, &packet, sizeof(ftp_type_t), &packet, repsiz) != repsiz) {
		color_printf(COLOR_RED, "No reply to crc request received, timeout or error (timeout set to %d msec)\r\n", ftp_timeout);
		return -1;
	}
	if (packet.type != FTP_CRC_REPLY)
		return -1;
	if (packet.crcrep.ret != FTP_RET_OK) {
		ftp_perror(packet.crcrep.ret);
		return -1;
	}

	/* Calculate CRC32 */
	ftp_file_crc32(&ftp_checksum);

	/* Validate checksum */
	packet.crcrep.crc = csp_ntoh32(packet.crcrep.crc);
	color_printf(COLOR_GREEN, "CRC Remote: 0x%"PRIx32", Local: 0x%"PRIx32"\r\n", packet.crcrep.crc, ftp_checksum);
	if (packet.crcrep.crc != ftp_checksum) {
		printf("CRC Failed!\r\n");
		return -1;
	}

	return 0;

}

int ftp_move(uint8_t host, uint8_t port, uint8_t backend, char * from, char * to) {

	/* Request */
	ftp_packet_t packet;
	packet.type = FTP_MOVE_REQUEST;
	strncpy(packet.move.from, from, FTP_PATH_LENGTH);
	packet.move.from[FTP_PATH_LENGTH-1] = '\0';
	strncpy(packet.move.to, to, FTP_PATH_LENGTH);
	packet.move.to[FTP_PATH_LENGTH-1] = '\0';
	packet.move.backend = backend;

	int reqsiz = sizeof(ftp_type_t) + sizeof(ftp_move_request_t);
	int repsiz = sizeof(ftp_type_t) + sizeof(ftp_move_reply_t);

	if (csp_transaction(CSP_PRIO_NORM, host, port,
			ftp_timeout, &packet, reqsiz, &packet, repsiz) != repsiz)
		return -1;
	if (packet.type != FTP_MOVE_REPLY)
		return -1;
	if (packet.moverep.ret != FTP_RET_OK) {
		ftp_perror(packet.moverep.ret);
		return -1;
	}

	return 0;

}

int ftp_copy(uint8_t host, uint8_t port, uint8_t backend, char * from, char * to) {

	/* Request */
	ftp_packet_t packet;
	packet.type = FTP_COPY_REQUEST;
	strncpy(packet.copy.from, from, FTP_PATH_LENGTH);
	packet.copy.from[FTP_PATH_LENGTH-1] = '\0';
	strncpy(packet.copy.to, to, FTP_PATH_LENGTH);
	packet.copy.to[FTP_PATH_LENGTH-1] = '\0';
	packet.copy.backend = backend;

	int reqsiz = sizeof(ftp_type_t) + sizeof(ftp_copy_request_t);
	int repsiz = sizeof(ftp_type_t) + sizeof(ftp_copy_reply_t);

	if (csp_transaction(CSP_PRIO_NORM, host, port,
			ftp_timeout, &packet, reqsiz, &packet, repsiz) != repsiz)
		return -1;
	if (packet.type != FTP_COPY_REPLY)
		return -1;
	if (packet.copyrep.ret != FTP_RET_OK) {
		ftp_perror(packet.copyrep.ret);
		return -1;
	}

	return 0;

}

int ftp_remove(uint8_t host, uint8_t port, uint8_t backend, char * path) {

	/* Request */
	ftp_packet_t packet;
	packet.type = FTP_REMOVE_REQUEST;
	strncpy(packet.remove.path, path, FTP_PATH_LENGTH);
	packet.remove.path[FTP_PATH_LENGTH-1] = '\0';
	packet.remove.backend = backend;

	int reqsiz = sizeof(ftp_type_t) + sizeof(ftp_remove_request_t);
	int repsiz = sizeof(ftp_type_t) + sizeof(ftp_remove_reply_t);

	if (csp_transaction(CSP_PRIO_NORM, host, port,
			ftp_timeout, &packet, reqsiz, &packet, repsiz) != repsiz)
		return -1;
	if (packet.type != FTP_REMOVE_REPLY)
		return -1;
	if (packet.removerep.ret != FTP_RET_OK) {
		ftp_perror(packet.removerep.ret);
		return -1;
	}

	return 0;

}

int ftp_mkfs(uint8_t host, uint8_t port, uint8_t backend, char *path, bool force) {

	/* Request */
	ftp_packet_t packet;
	packet.type = FTP_MKFS_REQUEST;
	strncpy(packet.mkfs.path, path, FTP_PATH_LENGTH);
	packet.mkfs.backend = backend;
	packet.mkfs.force = force ? 1 : 0;

	int reqsiz = sizeof(ftp_type_t) + sizeof(ftp_mkfs_request_t);
	int repsiz = sizeof(ftp_type_t) + sizeof(ftp_mkfs_reply_t);

	/* The default FTP timeout is not long enough for a full UFFS format.
	 * Bump to 120 seconds for this command */
	if (csp_transaction(CSP_PRIO_NORM, host, port,
			120000, &packet, reqsiz, &packet, repsiz) != repsiz)
		return -1;
	if (packet.type != FTP_MKFS_REPLY)
		return -1;
	if (packet.mkfsrep.ret != FTP_RET_OK) {
		ftp_perror(packet.mkfsrep.ret);
		return -1;
	}

	return 0;
}

int ftp_list(uint8_t host, uint8_t port, uint8_t backend, char * path, ftp_list_callback_t callback, void *data) {

	/* Request */
	int entries;
	csp_conn_t * c;
	csp_packet_t * p;
	ftp_packet_t packet;
	packet.type = FTP_LIST_REQUEST;
	packet.list.backend = backend;
	strncpy(packet.list.path, path, FTP_PATH_LENGTH);
	packet.list.path[FTP_PATH_LENGTH-1] = '\0';

	int reqsiz = sizeof(ftp_type_t) + sizeof(ftp_list_request_t);
	int repsiz = sizeof(ftp_type_t) + sizeof(ftp_list_reply_t);

	c = csp_connect(CSP_PRIO_NORM, host, port, ftp_timeout, CSP_O_RDP);
	if (c == NULL)
		return -1;
	if (csp_transaction_persistent(c, ftp_timeout, &packet, reqsiz, &packet, repsiz) != repsiz)
		return -1;
	if (packet.type != FTP_LIST_REPLY)
		return -1;
	if (packet.listrep.ret != FTP_RET_OK) {
		ftp_perror(packet.listrep.ret);
		return -1;
	}

	entries = csp_ntoh16(packet.listrep.entries);

	/* Read entries */
	ftp_packet_t * f;
	while (entries && ((p = csp_read(c, ftp_timeout)) != NULL)) {
		f = (ftp_packet_t *) &(p->data);
		f->listent.entry = csp_ntoh16(f->listent.entry);
		f->listent.size = csp_ntoh32(f->listent.size);

		callback(entries, &f->listent, data);

		uint16_t ent = f->listent.entry;
		csp_buffer_free(p);
		if (ent == entries - 1)
			break;
	}

	csp_close(c);

	return 0;

}

int ftp_zip(uint8_t host, uint8_t port, uint8_t backend, char * src, char * dest, uint8_t action) {

	/* Request */
	ftp_packet_t packet;
	packet.type = FTP_ZIP_REQUEST;
	strncpy(packet.zip.src, src, FTP_PATH_LENGTH);
	packet.zip.src[FTP_PATH_LENGTH-1] = '\0';
	strncpy(packet.zip.dest, dest, FTP_PATH_LENGTH);
	packet.zip.dest[FTP_PATH_LENGTH-1] = '\0';
	packet.zip.action = action;
	packet.zip.backend = backend;

	int reqsiz = sizeof(ftp_type_t) + sizeof(ftp_zip_request_t);
	int repsiz = sizeof(ftp_type_t) + sizeof(ftp_zip_reply_t);

	if (csp_transaction(CSP_PRIO_NORM, host, port,
			ftp_timeout, &packet, reqsiz, &packet, repsiz) != repsiz)
		return -1;
	if (packet.type != FTP_ZIP_REPLY)
		return -1;
	if (packet.ziprep.ret != FTP_RET_OK) {
		ftp_perror(packet.ziprep.ret);
		return -1;
	}
	uint32_t comp_sz = csp_ntoh32(packet.ziprep.comp_sz);
	uint32_t decomp_sz = csp_ntoh32(packet.ziprep.decomp_sz);
	if (action == FTP_ZIP) {
		color_printf(COLOR_GREEN, "Zip %s to %s, unzipped: %lu, zipped: %lu\r\n",
				src, dest, decomp_sz, comp_sz);
	} else {
		color_printf(COLOR_GREEN, "Unzip %s to %s, zipped: %lu, unzipped: %lu\r\n",
				src, dest, comp_sz, decomp_sz);
	}

	return 0;

}

void ftp_set_timeout(int timeout) {
	ftp_timeout = timeout;
}

int ftp_get_timeout(void) {
	return ftp_timeout;
}

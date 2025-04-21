/*
 * The Tick, a Linux embedded backdoor.
 * 
 * Released as open source by NCC Group Plc - http://www.nccgroup.com/
 * Developed by Mario Vilas, mario.vilas@nccgroup.com
 * http://www.github.com/nccgroup/thetick
 * 
 * See the LICENSE file for further details.
*/

#ifndef FILE_H
#define FILE_H

#include <sys/types.h>
#include <cjson/cJSON.h>

typedef struct {
    int bufferSize;
    int transferFrequency;
    int burstDuration;
    int pauseBetweenBursts;
} config_t;

char* get_current_config();
ssize_t get_free_space(const char *pathname);
int copy_stream(int source, int destination, ssize_t count, const config_t *config);
int load_config(const char* config_identifier, config_t *cfg);
cJSON *read_json(const char *filename);

#endif /* FILE_H */

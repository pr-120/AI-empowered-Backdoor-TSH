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


typedef struct {
    int bufferSize;
    int transferFrequency;
    int burstDuration;
    int pauseBetweenBursts;
} config_t;

int copy_stream(int source, int destination, ssize_t count);
int read_config(const char *filename, config_t *config);


#endif /* FILE_H */

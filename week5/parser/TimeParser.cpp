#include <stdlib.h>
#include <string.h>
#include "TimeParser.h"

// time format: HHMMSS (6 characters)
int time_parse(char *time) {

	// how many seconds, default returns error
	int seconds = TIME_LEN_ERROR;

	// Check that string is not null
	if (time == NULL) {
		return TIME_NULL_ERROR;
	}

	// Check that string is of correct length
	if (strlen(time) != 6) {
		return TIME_LEN_ERROR;
	}

	// Parse values from time string
	// For example: 124033 -> 12hour 40min 33sec
    int values[3];
	values[2] = atoi(time+4); // seconds
	time[4] = 0;
	values[1] = atoi(time+2); // minutes
	time[2] = 0;
	values[0] = atoi(time); // hours
	// Now you have:
	// values[0] hours
	// values[1] minutes
	// values[2] seconds

	// Boundary checks
	if (values[2] > 59 || values[2] < 0) {
		return TIME_VALUE_ERROR;
	}

	if (values[1] > 59 || values[1] < 0) {
		return TIME_VALUE_ERROR;
	}

	if (values[0] > 23 || values[0] < 0) {
		return TIME_VALUE_ERROR;
	}

	seconds = values[0]*3600 + values[1]*60 + values[2];

	if (seconds == 0) {
		return TIME_VALUE_ERROR;
	}

	return seconds;
}

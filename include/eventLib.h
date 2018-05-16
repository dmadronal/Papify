/* Standard headers for Papify test applications.
    This file is customized to hide Windows / Unix differences.
*/

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/wait.h>
#if (!defined(NO_DLFCN) && !defined(_BGL) && !defined(_BGP))
#include <dlfcn.h>
#endif

#include <errno.h>
#if !defined(__FreeBSD__) && !defined(__APPLE__)
#include <malloc.h>
#endif
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include "papiStdEventDefs.h"

#define RED    "\033[1;31m"
#define YELLOW "\033[1;33m"
#define GREEN  "\033[1;32m"
#define NORMAL "\033[0m"

static int TESTS_COLOR = 0;

/*
* Papify_action_s structure stores the information related to the monitoring of the function being monitored
*
*/

typedef struct papify_action_s {
	char *action_id;			// Name of the function being monitored
	long long *counterValues; 		// Total number of events associated to the function execution
	long long *counterValuesStart; 		// Starting point
	long long *counterValuesStop; 		// End point (required to measure events by differences)
	char *component_id;			// PAPI component associated to the PE executing the function
	char *PE_id;			// ID associated to the eventSet to be monitored. This ID needs to be different for functions executed in parallel, as the eventSets are associated to specific threads
	int num_counters;			// Number of events being monitored
	unsigned long long time_init_action;	// Starting time of the function
	unsigned long long time_end_action;	// Ending time of the function
	FILE* papify_output_file;		// File where the monitoring data will be stored
	int* papify_eventCodeSet;		// Code of the events that are being monitored
	int papify_eventSet;			// EventSet associated to the monitoring of the function
	
} papify_action_s;

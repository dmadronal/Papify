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
 
/*
* Papify_action_s structure stores the information related to the monitoring of the function being monitored
*
*/

typedef struct papify_action_s {
	char *actor_id;				// Name of the function being monitored
	long long *counterValues; 		// Total number of events associated to the function execution
	long long *counterValuesStart; 		// Starting point
	long long *counterValuesStop; 		// End point (required to measure events by differences)
	char *component_id;			// PAPI component associated to the eventSet of the function
	char *PE_id;				// ID associated to the eventSet to be monitored. This ID needs to be different for functions executed in parallel, as the eventSets are associated to specific threads
	int num_counters;			// Number of events being monitored
	unsigned long long time_init_action;	// Starting time of the function
	unsigned long long time_end_action;	// Ending time of the function
	FILE* papify_output_file;		// File where the monitoring data will be stored
	int* papify_eventCodeSet;		// Code of the events that are being monitored
	int papify_eventSet;			// EventSet associated to the monitoring of the function
	
} papify_action_s;

/*
* PE information required for monitoring dinamically
*
*/ 

typedef struct papify_PE_s {
	char *PE_id;				// ID associated to the eventSet to be monitored.
	int papify_eventSet_ID;			// ID of the eventSet running in the current thread
	int papify_eventSet_ID_original[20];	// ID of the original eventSet associated to the monitoring of the function
	
} papify_PE_s;
 
void configure_papify_actor(papify_action_s* papify_action, char* componentName, char* actorName, int num_events, char* all_events_name, int actor_id);
void configure_papify_PE(char* PEName, int PE_id);
void event_create_eventSet(papify_action_s* papify_action, int element_id, int dynamic);
void event_init();
void event_init_event_code_set(papify_action_s* papify_action, int code_set_size, char* all_events_name);
void event_init_multiplex();
void event_init_output_file(papify_action_s* papify_action, char* actorName, char* all_events_name);
void event_init_papify_actions(papify_action_s* papify_action, char* componentName, char* actorName, int num_events);
void event_launch(papify_action_s* papify_action, int PE_id);
void event_start(papify_action_s* papify_action, int PE_id);
void event_start_papify_timing(papify_action_s* papify_action);
void event_stop(papify_action_s* papify_action, int PE_id);
void event_stop_papify_timing(papify_action_s* papify_action);
void event_write_file(papify_action_s* papify_action);
void eventSet_set_multiplex(papify_action_s* papify_action);
void init_multiplex();
void structure_test(papify_action_s *someAction, int eventCodeSetSize, int *eventCodeSet);
void test_fail(char *file, int line, char *call, int retval);



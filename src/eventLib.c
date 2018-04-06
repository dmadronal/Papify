#include "eventLib.h"
#include "papi.h"

#include <time.h>

int total = 0;

pthread_mutex_t lock;

unsigned long tot_start;
unsigned long tot_end;

unsigned long long time_zero;


void structure_test(papify_action_s *someAction, int eventCodeSetSize, int *eventCodeSet){
    int i;
    printf("Action name: %s\n", someAction->action_id);
    printf("Event Code Set:\n");
    for(i=0; i<eventCodeSetSize; i++){
        printf("\t-%d\n", eventCodeSet[i]);
    }
}

static void test_fail( char *file, int line, char *call, int retval ) {

    int line_pad;
    char buf[128];

    line_pad = (int) (50 - strlen(file));
    if (line_pad < 0) {
        line_pad = 0;
    }

    memset(buf, '\0', sizeof(buf));

    if (TESTS_COLOR) {
        fprintf(stdout, "%-*s %sFAILED%s\nLine # %d\n", line_pad, file, RED,
                NORMAL, line);
    } else {
        fprintf(stdout, "%-*s FAILED\nLine # %d\n", line_pad, file, line);
    }

    if (retval == PAPI_ESYS) {
        sprintf(buf, "System error in %s", call);
        perror(buf);
    } else if (retval > 0) {
        fprintf(stdout, "Error: %s\n", call);
    } else if (retval == 0) {
    #if defined(sgi)
        fprintf( stdout, "SGI requires root permissions for this test\n" );
    #else
        fprintf(stdout, "Error: %s\n", call);
    #endif
    } else {
        fprintf(stdout, "Error in %s: %s\n", call, PAPI_strerror(retval));
    }

    fprintf(stdout, "\n");

    exit(1);
}

static void init_multiplex( void ) {

    int retval;
    const PAPI_hw_info_t *hw_info, *hw_info_register32;
    const PAPI_component_info_t *cmpinfo, *cmpinfo_register32;

    /* Initialize the library */

    /* for now, assume multiplexing on CPU compnent only */
    cmpinfo = PAPI_get_component_info(0);
    if (cmpinfo == NULL) {
        test_fail(__FILE__, __LINE__, "PAPI_get_component_info", 2);
    }

    hw_info = PAPI_get_hardware_info();
    if (hw_info == NULL) {
        test_fail(__FILE__, __LINE__, "PAPI_get_hardware_info", 2);
    }

    if ((strstr(cmpinfo->name, "perfctr.c")) && (hw_info != NULL )
            && strcmp(hw_info->model_string, "POWER6") == 0) {
        retval = PAPI_set_domain(PAPI_DOM_ALL);
        if (retval != PAPI_OK) {
            test_fail(__FILE__, __LINE__, "PAPI_set_domain", retval);
        }
    }
    retval = PAPI_multiplex_init();
    if (retval != PAPI_OK) {
        test_fail(__FILE__, __LINE__, "PAPI multiplex init fail\n", retval);
    }
}


void event_init(void) {

    int retval;

    // library initialization
    retval = PAPI_library_init( PAPI_VER_CURRENT );
    if ( retval != PAPI_VER_CURRENT )
        test_fail( __FILE__, __LINE__, "PAPI_library_init", retval );

    // place for initialization in case one makes use of threads
    retval = PAPI_thread_init((unsigned long (*)(void))(pthread_self));
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_thread_init", retval );

    printf("event_init done \n");
    time_zero = PAPI_get_real_usec();

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

}

void event_init_multiplex(void) {

    int retval;

    // library initialization
    retval = PAPI_library_init( PAPI_VER_CURRENT );
    if ( retval != PAPI_VER_CURRENT )
        test_fail( __FILE__, __LINE__, "PAPI_library_init", retval );

    // multiplex initialization
    init_multiplex(  );

    // place for initialization in case one makes use of threads
    retval = PAPI_thread_init((unsigned long (*)(void))(pthread_self));
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_thread_init", retval );

    printf("event_init done \n");
    time_zero = PAPI_get_real_usec();

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

}


void eventList_set_multiplex(int *eventSet){
	int retval;

    retval = PAPI_set_multiplex( *eventSet );
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_set_multiplex", retval );
}

void event_create_eventList(int *eventSet, int eventCodeSetSize, int *eventCodeSet, int threadID) {

    int retval, i, maxNumberHwCounters, eventCodeSetMaxSize;
    PAPI_event_info_t info;

    maxNumberHwCounters = PAPI_get_opt( PAPI_MAX_HWCTRS, NULL );
   // printf("Max number of hardware counters = %d \n", maxNumberHwCounters);

    eventCodeSetMaxSize = PAPI_get_opt( PAPI_MAX_MPX_CTRS, NULL );
   // printf("Max number of multiplexed counters = %d \n", eventCodeSetMaxSize);

    if ( eventCodeSetMaxSize < eventCodeSetSize)
        test_fail( __FILE__, __LINE__, "eventCodeSetMaxSize < eventCodeSetSize, too many performance events defined! ", retval );

    retval = PAPI_register_thread();
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_register_thread", retval );

    retval = PAPI_create_eventset( eventSet );
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_create_eventset", retval );

    retval = PAPI_assign_eventset_component( *eventSet, 0 );
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_assign_eventset_component", retval );

    for (i = 0; i < eventCodeSetSize; i++) {
        retval = PAPI_get_event_info(eventCodeSet[i], &info);
        if ( retval != PAPI_OK )
            test_fail( __FILE__, __LINE__, "PAPI_get_event_info", retval );

        retval = PAPI_add_event( *eventSet, info.event_code);
        if ( retval != PAPI_OK )
            test_fail( __FILE__, __LINE__, "PAPI_add_event", retval );
        /*else
            printf("Adding %s \n", info.symbol);*/

    }

}


void eventList_set_multiplex_unified(papify_action_s* papify_action){
	int retval;

    retval = PAPI_set_multiplex( papify_action[0].papify_eventSet );
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_set_multiplex", retval );
}

void event_create_eventList_unified(papify_action_s* papify_action) {

    int retval, i, maxNumberHwCounters, eventCodeSetMaxSize;
    PAPI_event_info_t info;

    maxNumberHwCounters = PAPI_get_opt( PAPI_MAX_HWCTRS, NULL );
    //printf("Max number of hardware counters = %d \n", maxNumberHwCounters);

    eventCodeSetMaxSize = PAPI_get_opt( PAPI_MAX_MPX_CTRS, NULL );
    //printf("Max number of multiplexed counters = %d \n", eventCodeSetMaxSize);

    if ( eventCodeSetMaxSize < papify_action[0].num_counters)
        test_fail( __FILE__, __LINE__, "eventCodeSetMaxSize < eventCodeSetSize, too many performance events defined! ", retval );

    retval = PAPI_register_thread();
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_register_thread", retval );

    retval = PAPI_create_eventset( &papify_action->papify_eventSet );
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_create_eventset", retval );

    /*if(strcmp(papify_action[0].component_id, "artico3_errors") == 0){
    	retval = PAPI_assign_eventset_component( papify_action[0].papify_eventSet, PAPI_get_component_index("artico3_errors"));
    }else{
   	retval = PAPI_assign_eventset_component( papify_action[0].papify_eventSet, 0 );
	eventList_set_multiplex_unified(papify_action);
    }*/

    retval = PAPI_assign_eventset_component( papify_action[0].papify_eventSet, PAPI_get_component_index(papify_action[0].component_id));
    if ( retval == PAPI_ENOCMP )
   	retval = PAPI_assign_eventset_component( papify_action[0].papify_eventSet, 0 );

    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_assign_eventset_component", retval );

    for (i = 0; i < papify_action[0].num_counters; i++) {
        retval = PAPI_get_event_info(papify_action[0].papify_eventCodeSet[i], &info);
        if ( retval != PAPI_OK )
            test_fail( __FILE__, __LINE__, "PAPI_get_event_info", retval );
        retval = PAPI_add_event( papify_action[0].papify_eventSet, info.event_code);
        if ( retval != PAPI_OK )
            test_fail( __FILE__, __LINE__, "PAPI_add_event", retval );
        /*else
            printf("Adding %s \n", info.symbol);*/

    }

}

void event_start(papify_action_s* papify_action, int threadID){

    int retval;

  //  struct timeval start, end;

 //   gettimeofday(&start, NULL);
    retval = PAPI_start( papify_action->papify_eventSet );
    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_start",retval );

 //   gettimeofday(&end, NULL);

 //   tot_start = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
/*    printf("Start time: %ld ", ((end.tv_sec * 1000000 + end.tv_usec)
		    - (start.tv_sec * 1000000 + start.tv_usec)));*/
}

void event_stop(papify_action_s* papify_action, int threadID) {

    int i, retval;
    char file_name[256];

	int code_0e;
 //   struct timeval start, end;

 //   gettimeofday(&start, NULL);

    retval = PAPI_stop( papify_action->papify_eventSet, papify_action->counterValues );

    if ( retval != PAPI_OK )
        test_fail( __FILE__, __LINE__, "PAPI_stop", retval );

 //   gettimeofday(&end, NULL);

 //   tot_end = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
    
   /* printf("Stop time: %ld\n", ((end.tv_sec * 1000000 + end.tv_usec)
		    - (start.tv_sec * 1000000 + start.tv_usec)));*/
 /*   snprintf(file_name, sizeof file_name, "%s%d%s", "event_measurement_", papify_action->num_counters, ".csv");
    papify_action->eventset_test_file = fopen(file_name,"a");

    fprintf(papify_action->eventset_test_file,"%ld %ld\n", tot_start, tot_end);

    fclose(papify_action->eventset_test_file);*/
}

void event_init_output_file(papify_action_s* papify_action, char* actorName, char* all_events_name) {
	
	char file_name[256];
	snprintf(file_name, sizeof file_name, "%s%s%s", "papify-output/papify_output_", actorName, ".csv");

	char output_string[1024];
	snprintf(output_string, sizeof output_string, "%s%s%s", "Actor,Action,tini,tend,", all_events_name, "\n");


	papify_action->papify_output_file = fopen(file_name,"w");
	fprintf(papify_action->papify_output_file,output_string);
	fclose(papify_action->papify_output_file);

	
}

void event_init_event_code_set(papify_action_s* papify_action, int code_set_size, char* all_events_name) {
	
	int i = 0;
	int event_code = 0;
	int retval;
	char all_events_name_local[250];

	snprintf(all_events_name_local, sizeof(all_events_name_local), "%s", all_events_name);
	char* event_name;
	char codeName_aux[250];

	event_name = strtok(all_events_name_local, ",");

	for(i = 0; i < code_set_size; i++){
		//printf("Actor %s ... event name = %s ... iter = %d\n", papify_action[0].action_id, event_name, i);
		
		retval = PAPI_event_name_to_code(event_name, &event_code);
		if ( retval != PAPI_OK ) {
	  		test_fail( __FILE__, __LINE__, 
		      "Translation of event not found\n", retval );
		}

        	event_name = strtok(NULL, ",");

		papify_action[0].papify_eventCodeSet[i] = event_code;
	}
}

void event_init_papify_actions(papify_action_s* papify_action, char* componentName, char* actorName, int num_events) {

	papify_action->counterValues = malloc(sizeof(long long) * num_events);

	papify_action[0].action_id = malloc(strlen(actorName)+1);
	snprintf(papify_action[0].action_id, (strlen(actorName)+1) * sizeof(char), "%s", actorName);

	papify_action[0].component_id = malloc(strlen(componentName)+1);
	snprintf(papify_action[0].component_id, (strlen(componentName)+1) * sizeof(char), "%s", componentName);

	papify_action[0].num_counters = num_events;

	papify_action->papify_eventCodeSet = malloc(sizeof(int) * num_events);
	papify_action->papify_eventSet = malloc(sizeof(unsigned long) * 1);
	papify_action->papify_eventSet = PAPI_NULL;
	papify_action->papify_output_file = malloc(sizeof(FILE) * 1);
}

void event_start_papify_timing(papify_action_s* papify_action){
	
	papify_action[0].time_init_action = PAPI_get_real_usec() - time_zero;
}

void event_stop_papify_timing(papify_action_s* papify_action){
	
	papify_action[0].time_end_action = PAPI_get_real_usec() - time_zero;
}

void configure_papify(papify_action_s* papify_action, char* componentName, char* actorName, int num_events, char* all_events_name){

	event_init_papify_actions(papify_action, componentName, actorName, num_events);
	event_init_output_file(papify_action, actorName, all_events_name);

	pthread_mutex_lock(&lock);
	event_init_event_code_set(papify_action, num_events, all_events_name);
	pthread_mutex_unlock(&lock);
	
	event_create_eventList_unified(papify_action);
}

void event_write_file(papify_action_s* papify_action){

	char output_file_name[250];
	int i;

	snprintf(output_file_name, sizeof output_file_name, "%s%s%s", "papify-output/papify_output_", papify_action[0].action_id, ".csv");

	papify_action[0].papify_output_file = fopen(output_file_name,"a+");

	fprintf(papify_action[0].papify_output_file, "%s,%s,%llu,%llu", papify_action[0].component_id, papify_action[0].action_id, papify_action[0].time_init_action, papify_action[0].time_end_action);

	for(i = 0; i < papify_action[0].num_counters; i++){
		fprintf(papify_action[0].papify_output_file, ",%lu", papify_action[0].counterValues[i]);
	}
	fprintf(papify_action[0].papify_output_file, "\n");

	fclose(papify_action[0].papify_output_file);
}


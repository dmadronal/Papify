#include "eventLib.h"
#include "papi.h"

#include <time.h>

pthread_mutex_t lock;

unsigned long long time_zero;

//Variables required to support dynamic reconfiguration

papify_PE_s papify_PEs_info[500];
int papify_eventSet_actor[500];
long long counterValuesTemp[50];
 
/* 
* This function configures the actor monitoring environment (initialization, file creation, event translation and eventSet creation) taking into account dynamic executions.
*/
void configure_papify_actor(papify_action_s* papify_action, char* componentName, char* actorName, int num_events, char* all_events_name, int actor_id){

	event_init_papify_actions(papify_action, componentName, actorName, num_events);
	event_init_output_file(papify_action, actorName, all_events_name);

	pthread_mutex_lock(&lock);
	event_init_event_code_set(papify_action, num_events, all_events_name);
	if(papify_eventSet_actor[actor_id] == -1){
		event_create_eventSet(papify_action, actor_id, 0);
	}
	else{
		papify_action[0].papify_eventSet = papify_eventSet_actor[actor_id];
	}
	pthread_mutex_unlock(&lock);
}

/* 
* This function configures the PE monitoring environment (initialization, file creation, event translation and eventSet creation) taking into account dynamic executions.
*/
void configure_papify_PE(char* PEName, int PE_id){

	papify_PEs_info[PE_id].PE_id = malloc(strlen(PEName)+1);
	snprintf(papify_PEs_info[PE_id].PE_id, (strlen(PEName)+1) * sizeof(char), "%s", PEName);
}

void event_create_eventSet(papify_action_s* papify_action, int element_id, int dynamic) {

	int retval, i, eventCodeSetMaxSize, maxNumberHwCounters;
	PAPI_event_info_t info;
	int papify_eventSet;
	PAPI_option_t opt;

	if(dynamic == 0){
		papify_eventSet = papify_action[0].papify_eventSet;
	}
	else{
		papify_eventSet = papify_PEs_info[element_id].papify_eventSet_ID_original[papify_action[0].papify_eventSet];
	}

	maxNumberHwCounters = PAPI_get_opt( PAPI_MAX_HWCTRS, NULL );
	eventCodeSetMaxSize = PAPI_get_opt( PAPI_MAX_MPX_CTRS, NULL );

	if ( eventCodeSetMaxSize < papify_action[0].num_counters)
		test_fail( __FILE__, __LINE__, "eventCodeSetMaxSize < eventCodeSetSize, too many performance events defined! ", 1 );


	retval = PAPI_create_eventset( &papify_eventSet );
	if ( retval != PAPI_OK )
		test_fail( __FILE__, __LINE__, "PAPI_create_eventset", retval );

	retval = PAPI_assign_eventset_component( papify_eventSet, PAPI_get_component_index(papify_action[0].component_id));
	if ( retval == PAPI_ENOCMP )
   		retval = PAPI_assign_eventset_component( papify_eventSet, 0 );

	if(maxNumberHwCounters < papify_action[0].num_counters){
		opt.multiplex.eventset = papify_eventSet;
		opt.multiplex.ns = 100;
		opt.itimer.ns = 100;
		retval = PAPI_set_opt( PAPI_DEF_ITIMER_NS, &opt );
		if ( retval != PAPI_OK ) {
			printf("Can't A PAPI_DEF_ITIMER_NS: %s\n",
				PAPI_strerror(retval));
		}
		retval = PAPI_set_opt( PAPI_MULTIPLEX, &opt );
		if ( retval != PAPI_OK ) {
			printf("Can't A PAPI_MULTIPLEX: %s\n",
				PAPI_strerror(retval));
		}
		printf("For %s monitoring %d events --> Multiplexing (Max HW counters = %d)\n", papify_action[0].actor_id, papify_action[0].num_counters, maxNumberHwCounters);
	}
	else{
		printf("For %s monitoring %d events --> No multiplexing\n", papify_action[0].actor_id, papify_action[0].num_counters);
	}

	for (i = 0; i < papify_action[0].num_counters; i++) {
		retval = PAPI_get_event_info(papify_action[0].papify_eventCodeSet[i], &info);
		if ( retval != PAPI_OK )
			test_fail( __FILE__, __LINE__, "PAPI_get_event_info", retval );
		retval = PAPI_add_event( papify_eventSet, info.event_code);
		if ( retval != PAPI_OK )
			test_fail( __FILE__, __LINE__, "PAPI_add_event", retval );
	}	

	if(dynamic == 0){
		papify_eventSet_actor[element_id] = papify_eventSet;
		papify_action[0].papify_eventSet = papify_eventSet;
	}
	else{
		papify_PEs_info[element_id].papify_eventSet_ID_original[papify_action[0].papify_eventSet] = papify_eventSet;
	}
	
	
}
/* 
* Cleanup and destroy of every variable created to monitor with Papify (this function should be called when the application is going to finish)
*/
void event_destroy(void) {

	int retval;
	int i;
	int j;

	for(i = 0; i < 500; i++){
		if(papify_PEs_info[i].papify_eventSet_ID != -1){
			retval = PAPI_stop( papify_PEs_info[i].papify_eventSet_ID, counterValuesTemp );
		}
		if(papify_eventSet_actor[i] != -1){
			// cleanup the eventSet
			retval = PAPI_cleanup_eventset(papify_eventSet_actor[i]);
			if ( retval != PAPI_OK )
				test_fail( __FILE__, __LINE__, "PAPI_cleanup_eventSet", retval );

			// destroy the eventSet
			retval = PAPI_destroy_eventset(&papify_eventSet_actor[i]);
			if ( retval != PAPI_OK )
				test_fail( __FILE__, __LINE__, "PAPI_destroy_eventSet", retval );
		}
		for(j = 0; j < 50; j++){
			if(papify_PEs_info[i].papify_eventSet_ID_original[j] != -1){
				// cleanup the eventSet
				retval = PAPI_cleanup_eventset(papify_PEs_info[i].papify_eventSet_ID_original[j]);
				if ( retval != PAPI_OK )
					test_fail( __FILE__, __LINE__, "PAPI_cleanup_eventSet", retval );
	
				// destroy the eventSet
				retval = PAPI_destroy_eventset(&papify_PEs_info[i].papify_eventSet_ID_original[j]);
				if ( retval != PAPI_OK )
				test_fail( __FILE__, __LINE__, "PAPI_destroy_eventSet", retval );
			
			}
		}
	}
	printf("event_destroy done \n");
}

/* 
* Initialize PAPI library and get the init time (this function should be called before any other monitoring function)
* It also stores the value of time_zero, which should be the starting point of the program
*/
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
	}
}

/* 
* This function translates the name of the events to be monitored to the PAPI codes associated to each event
*/
void event_init_event_code_set(papify_action_s* papify_action, int code_set_size, char* all_events_name) {
	
	int i = 0;
	int event_code = 0;
	int retval;
	char* all_events_name_local = malloc(strlen(all_events_name)+1);;
	snprintf(all_events_name_local, (strlen(all_events_name)+1) * sizeof(char), "%s", all_events_name);
	char* event_name;

	event_name = strtok(all_events_name_local, ",");

	for(i = 0; i < code_set_size; i++){
		
		retval = PAPI_event_name_to_code(event_name, &event_code);
		if ( retval != PAPI_OK ) {
			printf("Translation of event %s not found\n", event_name);
	  		test_fail( __FILE__, __LINE__, 
			  "Translation of event not found\n", retval );
		}

		event_name = strtok(NULL, ",");
		papify_action[0].papify_eventCodeSet[i] = event_code;
	}
}

/* 
* Initialize PAPI library and multiplex functionalities
*/
void event_init_multiplex(void) {

	int retval;
	int i;
	int j;

	for(i = 0; i < 500; i++){
		papify_eventSet_actor[i] = -1;
		papify_PEs_info[i].papify_eventSet_ID = -1;
		for(j = 0; j < 50; j++){
			papify_PEs_info[i].papify_eventSet_ID_original[j] = -1;
		}
	}

	// library initialization
	retval = PAPI_library_init( PAPI_VER_CURRENT );
	if ( retval != PAPI_VER_CURRENT )
		test_fail( __FILE__, __LINE__, "PAPI_library_init", retval );

	// place for initialization in case one makes use of threads
	retval = PAPI_thread_init((unsigned long (*)(void))(pthread_self));
	if ( retval != PAPI_OK )
		test_fail( __FILE__, __LINE__, "PAPI_thread_init", retval );

	// multiplex initialization
	init_multiplex(  );

	printf("event_init done \n");
	time_zero = PAPI_get_real_usec();

	if (pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
	}
}

/* 
* Create the .csv file associated to each function and prints the header of the file
*/
void event_init_output_file(papify_action_s* papify_action, char* actorName, char* all_events_name) {
	
	char file_name[256];
	snprintf(file_name, sizeof file_name, "%s%s%s", "papify-output/papify_output_", actorName, ".csv");

	char output_string[1024];
	snprintf(output_string, sizeof output_string, "%s%s%s", "PE,Actor,tini,tend,", all_events_name, "\n");

	papify_action->papify_output_file = fopen(file_name,"w");
	fprintf(papify_action->papify_output_file, "%s", output_string);
	fclose(papify_action->papify_output_file);
}

/* 
* This function initializes all the variables of the papify_action_s
*/
void event_init_papify_actions(papify_action_s* papify_action, char* componentName, char* actorName, int num_events) {

	papify_action->counterValues = malloc(sizeof(long long) * num_events);
	papify_action->counterValuesStart = malloc(sizeof(long long) * num_events);
	papify_action->counterValuesStop = malloc(sizeof(long long) * num_events);

	papify_action[0].actor_id = malloc(strlen(actorName)+1);
	snprintf(papify_action[0].actor_id, (strlen(actorName)+1) * sizeof(char), "%s", actorName);

	papify_action[0].component_id = malloc(strlen(componentName)+1);
	snprintf(papify_action[0].component_id, (strlen(componentName)+1) * sizeof(char), "%s", componentName);

	papify_action[0].PE_id = malloc(150 * sizeof(char));

	papify_action[0].num_counters = num_events;

	papify_action->papify_eventCodeSet = malloc(sizeof(int) * num_events);
	papify_action->papify_eventSet = PAPI_NULL;
	papify_action->papify_output_file = malloc(sizeof(FILE) * 1);
}

/* 
* Launch the monitoring of the eventSet. This eventSet will be counting from the beginning and the actual values will
* be computed by differences with event_start and event_stop functions
*/
void event_launch(papify_action_s* papify_action, int PE_id){

	int retval;
	retval = PAPI_start( papify_PEs_info[PE_id].papify_eventSet_ID );
	if ( retval != PAPI_OK )
		test_fail( __FILE__, __LINE__, "PAPI_start",retval );

	retval = PAPI_read( papify_PEs_info[PE_id].papify_eventSet_ID, papify_action->counterValuesStart );
	if ( retval != PAPI_OK )
	test_fail( __FILE__, __LINE__, "PAPI_read",retval );
}

/* 
* Read all the .csv files and generates a new one with a profiling associated to each actor
*/
void event_profiling() {
	
	DIR* FD;
	struct dirent* in_file;
	FILE *common_file;
	FILE *entry_file;
	char buffer[BUFSIZ];
	char input_file_name[256];
	unsigned long long averageValues[50];
	char* cellValue;
	char* actorName;
	int totalCells = 0;
	int i;
	int counterCells = 0;
	int totalExecutions = 0;
	unsigned long long initTime;
	unsigned long long endTime;
	unsigned long long executionTime;
	int maxNumberHwCounters;

	maxNumberHwCounters = PAPI_get_opt( PAPI_MAX_HWCTRS, NULL );
    	/* Opening common file for writing */
    	common_file = fopen("papify-output/papify_output_profiling.csv", "w");
    	if (common_file == NULL){
        	fprintf(stderr, "Error : Failed to open common_file - %s\n", strerror(errno));
        	return;
    	}
	fprintf(common_file, "Application profiling -- Actor,NB of events being monitored,NB of executions,Average values\n");
    	/* Scanning the in directory */
    	if (NULL == (FD = opendir ("./papify-output"))){
        	fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        	fclose(common_file);
        	return;
    	}
    	while ((in_file = readdir(FD)) != NULL){
		/* Avoid opening wrong files
	 	*/
		if (!strcmp (in_file->d_name, "."))
	    		continue;
		if (!strcmp (in_file->d_name, ".."))    
	    		continue;
		if (!strcmp (in_file->d_name, "papify_output_profiling.csv"))    
	    		continue;

		/* Open directory entry file for common operation */
		snprintf(input_file_name, sizeof input_file_name, "%s%s", "papify-output/", in_file->d_name);
		entry_file = fopen(input_file_name, "r");
		if (entry_file == NULL){
	    		fprintf(stderr, "Error : Failed to open entry file - %s\n", strerror(errno));
	    		fclose(common_file);
	    		return;
		}
		/* Doing some struf with entry_file : */
		/* For example use fgets */
		if (fgets(buffer, BUFSIZ, entry_file) != NULL){
            		cellValue = strtok(buffer, ","); // PE
            		cellValue = strtok(NULL, ","); // Actor
			fprintf(common_file, "\n%s,totalEvents,totalExecutions", cellValue);
			totalCells = 0;
            		cellValue = strtok(NULL, ","); // tini
            		cellValue = strtok(NULL, ","); // tfin
            		cellValue = strtok(NULL, ","); // First event
			while(cellValue != NULL){
				averageValues[totalCells] = 0;
				totalCells++;
				fprintf(common_file, ",%s", cellValue);
            			cellValue = strtok(NULL, ",");				
			}
		}
		totalExecutions = 0;
		executionTime = 0;
		counterCells = 0;
		while (fgets(buffer, BUFSIZ, entry_file) != NULL){
			totalExecutions++;
			counterCells = 0;
			buffer[strlen(buffer)-1] = 0;
			cellValue = strtok(buffer, ","); // PE
            		actorName = strtok(NULL, ","); // Actor
            		cellValue = strtok(NULL, ","); // tini
            		initTime = strtoull(cellValue, NULL, 10);
            		cellValue = strtok(NULL, ","); // tfin
            		endTime = strtoull(cellValue, NULL, 10);
			executionTime = executionTime + (endTime - initTime);
            		cellValue = strtok(NULL, ",");
			while(cellValue != NULL){
				averageValues[counterCells] = averageValues[counterCells] + strtoull(cellValue, NULL, 10);
				counterCells++;
            			cellValue = strtok(NULL, ",");
			}
		}
		if(maxNumberHwCounters<totalCells){
			printf("WARNING: %s monitoring needed multiplexing --> Some values could have been corrupted by outliers\n", actorName);
			printf("TIP: please, consider monitoring up to %d events at the same time to avoid multiplexing\n", maxNumberHwCounters);

		}
		fprintf(common_file, "%s", actorName);
		fprintf(common_file, ",%d", totalCells);
		fprintf(common_file, ",%d", totalExecutions);
		fprintf(common_file, ",%llu", executionTime/totalExecutions);
		for(i = 0; i < totalCells; i++){
			fprintf(common_file, ",%llu", averageValues[i]/totalExecutions);			
		}
		/* When you finish with the file, close it */
		fprintf(common_file, "\n");
		fclose(entry_file);
    	}
    	/* Don't forget to close common file before leaving */
    	fclose(common_file);

    	return;
}

/* 
* Read the current values of the event counters and stores them as the starting point
*/
void event_start(papify_action_s* papify_action, int PE_id){

	int retval;
	if(papify_PEs_info[PE_id].papify_eventSet_ID_original[papify_action[0].papify_eventSet] == papify_PEs_info[PE_id].papify_eventSet_ID && papify_PEs_info[PE_id].papify_eventSet_ID != -1){
		snprintf(papify_action[0].PE_id, (strlen(papify_PEs_info[PE_id].PE_id)+1) * sizeof(char), "%s", papify_PEs_info[PE_id].PE_id);
		if(papify_action[0].num_counters != 0){
			retval = PAPI_read( papify_PEs_info[PE_id].papify_eventSet_ID, papify_action->counterValuesStart );
		if ( retval != PAPI_OK )
			test_fail( __FILE__, __LINE__, "PAPI_start",retval );
		}
	}
	else if(papify_PEs_info[PE_id].papify_eventSet_ID_original[papify_action[0].papify_eventSet] == -1){
		if(papify_PEs_info[PE_id].papify_eventSet_ID != -1){
			retval = PAPI_stop( papify_PEs_info[PE_id].papify_eventSet_ID, counterValuesTemp );
			if ( retval != PAPI_OK )
				test_fail( __FILE__, __LINE__, "PAPI_stop",retval );
		}
		snprintf(papify_action[0].PE_id, (strlen(papify_PEs_info[PE_id].PE_id)+1) * sizeof(char), "%s", papify_PEs_info[PE_id].PE_id);
		event_create_eventSet(papify_action, PE_id, 1);
		papify_PEs_info[PE_id].papify_eventSet_ID = papify_PEs_info[PE_id].papify_eventSet_ID_original[papify_action[0].papify_eventSet];
		event_launch(papify_action, PE_id);
	}
	else{
		if(papify_PEs_info[PE_id].papify_eventSet_ID != -1){
			retval = PAPI_stop( papify_PEs_info[PE_id].papify_eventSet_ID, counterValuesTemp );
			if ( retval != PAPI_OK )
				test_fail( __FILE__, __LINE__, "PAPI_stop",retval );
		}
		snprintf(papify_action[0].PE_id, (strlen(papify_PEs_info[PE_id].PE_id)+1) * sizeof(char), "%s", papify_PEs_info[PE_id].PE_id);
		papify_PEs_info[PE_id].papify_eventSet_ID = papify_PEs_info[PE_id].papify_eventSet_ID_original[papify_action[0].papify_eventSet];
		event_launch(papify_action, PE_id);	
	}
}

/* 
* This function stores the starting point of the timing monitoring using PAPI_get_real_usec() function
*/
void event_start_papify_timing(papify_action_s* papify_action, int PE_id){
	
	snprintf(papify_action[0].PE_id, (strlen(papify_PEs_info[PE_id].PE_id)+1) * sizeof(char), "%s", papify_PEs_info[PE_id].PE_id);
	papify_action[0].time_init_action = PAPI_get_real_usec() - time_zero;
}

/* 
* Read the current values of the event counters and stores them as the ending point.
* After that, the total value is computed by differences and stored as the total value
*/
void event_stop(papify_action_s* papify_action, int PE_id) {

	int retval, i;
	if(papify_action[0].num_counters != 0){
		retval = PAPI_read( papify_PEs_info[PE_id].papify_eventSet_ID, papify_action->counterValuesStop );
	for(i = 0; i < papify_action[0].num_counters; i++){
		//if(papify_action[0].counterValuesStop[i] > papify_action[0].counterValuesStart[i]){
			papify_action[0].counterValues[i] = papify_action[0].counterValuesStop[i] - papify_action[0].counterValuesStart[i];
		//}
		/*else{
			papify_action[0].counterValues[i] = papify_action[0].counterValuesStart[i] - papify_action[0].counterValuesStop[i];
		}*/
	}
	if ( retval != PAPI_OK )
		test_fail( __FILE__, __LINE__, "PAPI_read",retval );
	}
}

/* 
* This function stores the ending point of the timing monitoring using PAPI_get_real_usec() function
*/
void event_stop_papify_timing(papify_action_s* papify_action, int PE_id){
	
	snprintf(papify_action[0].PE_id, (strlen(papify_PEs_info[PE_id].PE_id)+1) * sizeof(char), "%s", papify_PEs_info[PE_id].PE_id);
	papify_action[0].time_end_action = PAPI_get_real_usec() - time_zero;
}

/* 
* This function writes all the monitoring information as a new line of the .csv file
*/
void event_write_file(papify_action_s* papify_action){

	char output_file_name[250];
	int i;

	snprintf(output_file_name, sizeof output_file_name, "%s%s%s", "papify-output/papify_output_", papify_action[0].actor_id, ".csv");
	papify_action[0].papify_output_file = fopen(output_file_name,"a+");
	fprintf(papify_action[0].papify_output_file, "%s,%s,%llu,%llu", papify_action[0].PE_id, papify_action[0].actor_id, papify_action[0].time_init_action, papify_action[0].time_end_action);

	for(i = 0; i < papify_action[0].num_counters; i++){
		fprintf(papify_action[0].papify_output_file, ",%llu", papify_action[0].counterValues[i]);
	}
	fprintf(papify_action[0].papify_output_file, "\n");
	fclose(papify_action[0].papify_output_file);
}

/* 
* Initialize multiplex functionalities
*/

void init_multiplex( void ) {

	int retval;
	const PAPI_hw_info_t *hw_info;
	const PAPI_component_info_t *cmpinfo;

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

	if ((strstr(cmpinfo->name, "perfctr.c")) && (hw_info != NULL ) && strcmp(hw_info->model_string, "POWER6") == 0) {
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

/*  
* Print the structure to check whether it is configured correctly
*/
void structure_test(papify_action_s *someAction, int eventCodeSetSize, int *eventCodeSet){
	int i;
	printf("Action name: %s\n", someAction->actor_id);
	printf("Event Code Set:\n");
	for(i=0; i<eventCodeSetSize; i++){
		printf("\t-%d\n", eventCodeSet[i]);
	}
}
 
/* 
* Test function to check where the monitoring is failing
*/
void test_fail( char *file, int line, char *call, int retval ) {

	int line_pad;
	char buf[128];

	line_pad = (int) (50 - strlen(file));
	if (line_pad < 0) {
		line_pad = 0;
	}

	memset(buf, '\0', sizeof(buf));

	fprintf(stdout, "%-*s FAILED\nLine # %d\n", line_pad, file, line);

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


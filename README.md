# Papify
### author: Daniel Madroñal (daniel.madronal@upm.es)

Extra layer to unify PAPI library usage for several components

The user can instrument its code using this library based on four stages:

#### Initialization

`event_init_multiplex(); // Initialize PAPI and Papify libraries`

#### Configuration

`configure_papify_PE(char* PEName, int PE_id); // Configure each PE setting its name and its id
configure_papify_actor(papify_action_s* papify_action, char* componentName, char* actorName, int num_events, char* all_events_name, int actor_id); // Configure each actor setting its PAPI component, its name, the number of events being monitored, the events themselves and its id`

#### Monitoring

`event_start(papify_action_s* papify_action, int PE_id); // Start monitoring the events
event_start_papify_timing(papify_action_s* papify_action, int PE_id); // Start timing
do_work(); // User function
event_stop_papify_timing(papify_action_s* papify_action, int PE_id); // Stop monitoring the events 
event_stop(papify_action_s* papify_action, int PE_id); // Stop timing
void event_write_file(papify_action_s* papify_action); // Save results`

#### Ending

`event_destroy(); // Cleanup and destroy variables
event_profiling(); // Generate a profiling based on the different files generated while monitoring`

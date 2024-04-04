#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/time.h>

#include "museumsim.h"
#include "log.h"

static __thread uint32_t rand_seed;

static int test_mode = 0;
static pthread_mutex_t simulation_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Start time of the simulation
 */
struct timeval start_time;

enum static_config_attr {
	/* Total number of visitors which will arrive */
	NUM_VISITORS,

	/* Total number of tour guides which will arrive */
	NUM_GUIDES,

	/* Probability of no visitor delay (as a percentage) */
	VISITOR_CLUSTER_PROBABILITY,

	/* Time before visitor arrival if delayed (in microseconds) */
	VISITOR_ARRIVAL_DELAY,

	/* Random seed used for visitors */
	VISITOR_RANDOM_SEED,

	/* Probability of tour guide delay (as a percentage) */
	GUIDE_CLUSTER_PROBABILITY,

	/* Time before tour guide arrival if delayed (in microseconds) */
	GUIDE_ARRIVAL_DELAY,

	/* Random seed used for guides */
	GUIDE_RANDOM_SEED,

	/* How long the visitor will take during a tour */
	VISITOR_TOUR_DURATION,

	/* Number of values in the enum (must be last) */
	NUM_OPTIONS
};

static size_t options[NUM_OPTIONS] = {
	10,		/* num_visitors */
	1,		/* num_guides */
	100,		/* visitor_cluster_probability */
	1000000,	/* visitor_arrival_delay */
	1,		/* visitor_random_seed */
	100,		/* guide_cluster_probability */
	1000000,	/* guide_arrival_delay */
	1,		/* guide_random_seed */
	2000000		/* visitor_tour_duration*/
};

static size_t test_options[10][NUM_OPTIONS] = {
	{10, 1},
	{20, 2},
	{30, 3},
	{40, 4},
	{50, 5},
	{60, 6},
	{70, 7},
	{80, 8},
	{90, 9},
	{100, 10}
};

struct environment_initializer {
	const char *name;
	enum static_config_attr option;
};

static struct environment_initializer debug_configurables[] = {
	{"num_visitors",		NUM_VISITORS},
	{"num_guides",			NUM_GUIDES},
	{"visitor_cluster_probability",	VISITOR_CLUSTER_PROBABILITY},
	{"visitor_random_seed",		VISITOR_RANDOM_SEED},
	{"visitor_arrival_delay",	VISITOR_ARRIVAL_DELAY},
	{"guide_cluster_probability",	GUIDE_CLUSTER_PROBABILITY},
	{"guide_arrival_delay",		GUIDE_ARRIVAL_DELAY},
	{"guide_random_seed",		GUIDE_RANDOM_SEED},
	{"visitor_tour_duration",	VISITOR_TOUR_DURATION}
};

/**
 * Parses the environment arguments for the program into the options array and
 * fills in the program start time.
 */
static void initialize_static()
{
	for (size_t i = 0; i < sizeof(debug_configurables)/sizeof(debug_configurables[0]); ++i) {
		struct environment_initializer ei = debug_configurables[i];
		const char *envp = getenv(ei.name);

		if (envp) {
			info("Setting option %s to %s\n", ei.name, envp);
			options[ei.option] = atol(envp);
		}
	}

	gettimeofday(&start_time, NULL);
}

/**
 * Parses the test harness data and configures any randomness.
 */
static void initialize_test_static(size_t input_options[static NUM_OPTIONS])
{
	memcpy(options, input_options, sizeof(options));

	for (size_t i = 0; i < sizeof(options)/sizeof(options[0]); ++i) {
		if (options[i] != 0) {
			info("Setting option %s to %ld\n", debug_configurables[i].name, options[i]);
		}
	}

	gettimeofday(&start_time, NULL);
}

static uint32_t thread_rand(uint32_t *state)
{
	uint32_t x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return *state = x;
}

/**
 * Microsleeps for a small, random amount of time in test mode.
 */
static void test_microsleep()
{
	if (!test_mode) {
		return;
	}

	int should_sleep = thread_rand(&rand_seed) % 2;
	int sleep_duration = thread_rand(&rand_seed) % 10000;

	if (should_sleep) {
		usleep(sleep_duration);
	}
}

/**
 * Get the time elapsed (in seconds) since the simulation start.
 */
static long elapsed_time()
{
	struct timeval now;
	gettimeofday(&now, NULL);

	return
		((now.tv_sec - start_time.tv_sec) * 1000000 +
		(now.tv_usec - start_time.tv_usec)) / 1000000;
}

//
// Simulation functions
//

static struct visitor_sim {
	int already_arrived;
	int allowed_to_enter;
	int allowed_to_leave;
} *visitors;

static struct guide_sim {
	int already_arrived;
	int already_left;
	int allowed_to_enter;
	int allowed_to_leave;
} *guides;

static int visitors_waiting;
static int guides_waiting;
static int visitors_inside;
static bool guide_may_enter;
static bool museum_empty;
static bool visitor_group_may_enter;
static int visitors_in_group;
static bool guide_entered;

static void init_simulation_state()
{
	visitors		= calloc(options[NUM_VISITORS], sizeof(struct visitor_sim));
	guides			= calloc(options[NUM_GUIDES], sizeof(struct guide_sim));
	visitors_waiting = 0;
 	guides_waiting = 0;
	visitors_inside = 0;
	visitors_in_group = 0;
 	guide_may_enter = false;
 	museum_empty = true;
	visitor_group_may_enter = false;
	guide_entered = false;

	info("Starting run\n\n");
}

static void free_simulation_state()
{
	if (visitors_waiting > 0) {
		error("%d visitor(s) still waiting outside after run\n", visitors_waiting);
	}
	if (!museum_empty) {
		error("A guide and/or visitor(s) still inside after run\n");
	}
	for (int i = 0; i < options[NUM_VISITORS]; ++i) {
		if (!visitors[i].already_arrived) {
			error("Visitor %d never arrived during run\n", i);
		}
	}
	for (int i = 0; i < options[NUM_GUIDES]; ++i) {
		if (!guides[i].already_arrived) {
			error("Guide %d never arrived during run\n", i);
		}
	}

	free(visitors);
	free(guides);

	info("Finishing run\n\n");
}

/**
 * Visitor arrival routine. Called after the visitor has arrived. Prints
 * arrival status and updates the simulation state
 */
void visitor_arrives(int id)
{
	pthread_mutex_lock(&simulation_lock);
	{
		printf("Visitor %d arrives at time %ld.\n", id, elapsed_time());

		visitors_waiting++;
		if(visitors_waiting >= VISITORS_PER_GUIDE && guides_waiting >= GUIDES_ALLOWED_INSIDE && museum_empty){
			guide_may_enter = true;
		}

		if (visitors[id].already_arrived) {
			error("Visitor %d arrived twice\n", id);
		}

		visitors[id].already_arrived = 1;
		visitors[id].allowed_to_enter = 1;
		visitors[id].allowed_to_leave = 0;
	}
	pthread_mutex_unlock(&simulation_lock);

	test_microsleep();
}

/**
 * Visitor touring routine. Called when the visitor tours the museum. Prints
 * touring status, then sleeps to simulate tour duration.
 */
void visitor_tours(int id)
{
	pthread_mutex_lock(&simulation_lock);
	{
		printf("Visitor %d tours the museum at time %ld.\n", id, elapsed_time());

		if(!visitor_group_may_enter){
			error("Visitor %d tried to tour without a group of ten visitors and/or without a guide, or the museum is not empty\n", id);
		}

		visitors_in_group++;
		visitors_waiting--;

		if(visitors_in_group == 10){
		   visitor_group_may_enter = false;
		   visitors_in_group = 0;
		}
		
		visitors_inside++;

		if (!visitors[id].allowed_to_enter) {
			error("Visitor %d tried to tour twice\n", id);
		}
		visitors[id].allowed_to_enter = 0;
		visitors[id].allowed_to_leave = 1;

	}
	
	pthread_mutex_unlock(&simulation_lock);

	usleep(options[VISITOR_TOUR_DURATION]);
	test_microsleep();
}

/**
 * Visitor leaving routine. Called when the visitor is leaving. Prints
 * leaving status and updates the simulation state.
 */
void visitor_leaves(int id)
{
	pthread_mutex_lock(&simulation_lock);
	{
		printf("Visitor %d leaves the museum at time %ld.\n", id, elapsed_time());
		visitors_inside--;

		if (!visitors[id].allowed_to_leave) {
			error("Visitor %d left twice or never toured\n", id);
		}

		visitors[id].allowed_to_leave = 0;
	}
	pthread_mutex_unlock(&simulation_lock);

	test_microsleep();
}

/**
 * Guide arrival routine. Called after the guide has arrived. Prints arrival
 * status and updates the simulation state.
 */
void guide_arrives(int id)
{
	pthread_mutex_lock(&simulation_lock);
	{
		printf("Guide %d arrives at time %ld.\n", id, elapsed_time());
		guides[id].allowed_to_enter = 1;
		guides_waiting++;

		if(visitors_waiting >= VISITORS_PER_GUIDE && guides_waiting >= GUIDES_ALLOWED_INSIDE && museum_empty){
			guide_may_enter = true;
		}
		

		if (guides[id].already_arrived) {
			error("Guide %d arrived twice\n", id);
		}		

		guides[id].already_arrived = 1;
	}
	pthread_mutex_unlock(&simulation_lock);

	test_microsleep();
}

/**
 * Guide enter routine. Called after the guide has entered. Prints entering
 * status and updates the simulation state.
 */
void guide_enters(int id)
{
	pthread_mutex_lock(&simulation_lock);
	{
		printf("Guide %d enters the museum at time %ld.\n", id, elapsed_time());
		if (!guide_may_enter) {
			error("Guide %d entered the museum, but was not allowed to enter yet\n", id);
		}
		if (!guides[id].allowed_to_enter) {
			error("Guide %d entered the museum without arriving first, or entered twice\n", id);
		}
		guides_waiting--;		
		visitor_group_may_enter = true;
		guide_may_enter = false;
		museum_empty = false;
		guides[id].allowed_to_leave = 0;		
		guides[id].allowed_to_enter = 0;

	}
	pthread_mutex_unlock(&simulation_lock);

	test_microsleep();
}


/**
 * Guide leaving routine. Called after the guide leaves. Prints leaving status
 * and updates the simulation state.
 */
void guide_leaves(int id)
{
	pthread_mutex_lock(&simulation_lock);
	{
		printf("Guide %d leaves the museum at time %ld.\n", id, elapsed_time());

		if (visitors_inside > 0) {
			error("Guide %d attempted to leave, but visitors were still inside\n", id);
		}
		if (guides[id].already_left) {
			error("Guide %d left twice\n", id);
		}

		guides[id].already_left = 1;
		museum_empty = true;
		guide_entered = false;
		if(visitors_waiting >= VISITORS_PER_GUIDE && guides_waiting >= GUIDES_ALLOWED_INSIDE && museum_empty){
			guide_may_enter = true;
		}
	}
	pthread_mutex_unlock(&simulation_lock);

	test_microsleep();
}


//
// Thread spawning
//


/**
 * Visitor thread routine.
 */
static void *visitor_thread(void *argument)
{
	uintptr_t visitor_id = (uintptr_t) argument;

	rand_seed = visitor_id + 1;
	visitor(visitor_id);

	return NULL;
}

/**
 * Guide thread routine.
 */
static void *guide_thread(void *argument)
{
	uintptr_t guide_id = (uintptr_t) argument;

	rand_seed = guide_id + 1;
	guide(guide_id);

	return NULL;
}

/**
 * Spawn visitors according to the arrival mechanism, wait for all visitors to
 * finish executing, then exit.
 */
static void *visitor_arrival_thread(void *argument)
{
	pthread_t *threads = calloc(options[NUM_VISITORS], sizeof(pthread_t));

	rand_seed = options[VISITOR_RANDOM_SEED];

	for (size_t i = 0; i < options[NUM_VISITORS]; ++i) {
		pthread_create(&threads[i], NULL, visitor_thread, (void *) i);

		if ((thread_rand(&rand_seed) % 100) + 1 > options[VISITOR_CLUSTER_PROBABILITY]) {
			usleep(options[VISITOR_ARRIVAL_DELAY]);
		}
	}

	for (size_t i = 0; i < options[NUM_VISITORS]; ++i) {
		pthread_join(threads[i], NULL);
	}

	free(threads);

	return NULL;
}

/**
 * Spawn tour guides according to the arrival mechanism, wait for all guides
 * to finish executing, then exit.
 */
static void *guide_arrival_thread(void *argument)
{
	pthread_t *threads = calloc(options[NUM_GUIDES], sizeof(pthread_t));

	rand_seed = options[GUIDE_RANDOM_SEED];

	for (size_t i = 0; i < options[NUM_GUIDES]; ++i) {
		pthread_create(&threads[i], NULL, guide_thread, (void *) i);

		if ((thread_rand(&rand_seed) % 100) + 1 > options[GUIDE_CLUSTER_PROBABILITY]) {
			usleep(options[GUIDE_ARRIVAL_DELAY]);
		}
	}

	for (size_t i = 0; i < options[NUM_GUIDES]; ++i) {
		pthread_join(threads[i], NULL);
	}

	free(threads);

	return NULL;
}

static void run_simulation()
{
	pthread_t guide_spawner, visitor_spawner;

	init_simulation_state();
	museum_init(options[NUM_GUIDES], options[NUM_VISITORS]);

	pthread_create(&visitor_spawner, NULL, visitor_arrival_thread, NULL);
	pthread_create(&guide_spawner, NULL, guide_arrival_thread, NULL);

	pthread_join(visitor_spawner, NULL);
	pthread_join(guide_spawner, NULL);

	museum_destroy();
	// free_simulation_state();
}

static void run_debug()
{
	initialize_static();
	run_simulation();
}

static void run_test()
{
	test_mode = 1;

	for (size_t i = 0; i < sizeof(test_options)/sizeof(test_options[0]); ++i) {
		initialize_test_static(test_options[i]);
		run_simulation();
	}
}

int main(int argc, char *argv[])
{
	atexit(free_simulation_state);
	if (argc == 2 && strcmp(argv[1], "test") == 0) {
		run_test();
	} else {
		run_debug();
	}

	return 0;
}

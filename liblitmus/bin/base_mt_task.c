/* based_mt_task.c -- A basic multi-threaded real-time task skeleton. 
 *
 * This (by itself useless) task demos how to setup a multi-threaded LITMUS^RT
 * real-time task. Familiarity with the single threaded example (base_task.c)
 * is assumed.
 *
 * Currently, liblitmus still lacks automated support for real-time
 * tasks, but internaly it is thread-safe, and thus can be used together
 * with pthreads.
 */

#include <stdio.h>
#include <stdlib.h>

/* Include gettid() */
#include <sys/types.h>

/* Include threading support. */
#include <pthread.h>

/* Include the LITMUS^RT API.*/
#include "litmus.h"

#define PERIOD		100
#define EXEC_COST	 10

/* Let's create 10 threads in the example, 
 * for a total utilization of 1.
 */
#define NUM_THREADS      10

/* The information passed to each thread. Could be anything. */
struct thread_context {
	int id;
};

// global variable to keep count of threads which are yet to obtain their pid
int remaining_threads_count = NUM_THREADS;

// constraints are set if its value is 1
int constraints_set = -1;

// condtion variable
pthread_cond_t count_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t constraints_cond = PTHREAD_COND_INITIALIZER;

// lock variable
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t constraints_lock = PTHREAD_MUTEX_INITIALIZER;


// Constraint Task
struct thread_context set_ctx;
pthread_t      set_ctx_task;

// Global Data Structure to store thread pids.
pid_t thread_pid[NUM_THREADS];
pid_t main_thread_pid;

/* The real-time thread program. Doesn't have to be the same for
 * all threads. Here, we only have one that will invoke job().
 */
void* rt_thread(void *tcontext);

// Thread to set constraints
void* set_constraint_thread(void* context);

/* Declare the periodically invoked job. 
 * Returns 1 -> task should exit.
 *         0 -> task should continue.
 */
int job(int thread_id);


/* Catch errors.
 */
#define CALL( exp ) do { \
		int ret; \
		ret = exp; \
		if (ret != 0) \
			fprintf(stderr, "%s failed: %m\n", #exp);\
		else \
			fprintf(stderr, "%s ok.\n", #exp); \
	} while (0)


/* Basic setup is the same as in the single-threaded example. However, 
 * we do some thread initiliazation first before invoking the job.
 */
int main(int argc, char** argv)
{
	int i;
	struct thread_context ctx[NUM_THREADS];
	pthread_t             task[NUM_THREADS];

	/* The task is in background mode upon startup. */		


	/*****
	 * 1) Command line paramter parsing would be done here.
	 */


       
	/*****
	 * 2) Work environment (e.g., global data structures, file data, etc.) would
	 *    be setup here.
	 */



	/*****
	 * 3) Initialize LITMUS^RT.
	 *    Task parameters will be specified per thread.
	 */
	init_litmus();

	// For Dependent Tasks
	// Obtain the main thread pid
	main_thread_pid = gettid();
	printf("Main thread PID: %d \n", main_thread_pid);

	// Syscall to intialize task pid at kernel
	CALL(init_dep_task(main_thread_pid));

	// Create the set constraint thread to specify the constraints.
	set_ctx.id = NUM_THREADS;
	pthread_create(&set_ctx_task, NULL, set_constraint_thread, (void *) (set_ctx_task));

	/*****
	 * 4) Launch threads.
	 */
	for (i = 0; i < NUM_THREADS; i++) {
		ctx[i].id = i;
		pthread_create(task + i, NULL, rt_thread, (void *) (ctx + i));
	}


	/*****
	 * 5) Wait for RT threads to terminate.
	 */
	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(task[i], NULL);

//	pthread_join(set_ctx_task, NULL);

	/***** 
	 * 6) Clean up, maybe print results and stats, and exit.
	 */

	// Syscall for exiting dependepent task
	CALL(exit_dep_task(main_thread_pid));

	return 0;
}



/* A real-time thread is very similar to the main function of a single-threaded
 * real-time app. Notice, that init_rt_thread() is called to initialized per-thread
 * data structures of the LITMUS^RT user space libary.
 */
void* rt_thread(void *tcontext)
{
	struct thread_context *ctx = (struct thread_context *) tcontext;
	int do_exit;

	/* Make presence visible. */
	printf("RT Thread %d active.\n", ctx->id);

	/*****
	 * 1) Initialize real-time settings.
	 */
	CALL( init_rt_thread() );
	CALL( sporadic_global_dependent(EXEC_COST, PERIOD) );

	// For Dependent Tasks
	pthread_mutex_lock(&count_lock);
		// Syscall to obtain the pid
		thread_pid[ctx->id] = gettid();
		printf("Thread id: %d PID: %d \n",ctx->id, thread_pid[ctx->id]);

		// Syscall to link main thread task to the subtask at kernel
		CALL(set_main_task_pid(thread_pid[ctx->id], main_thread_pid));

		// Syscall to add the subtask to dependent task list at kernel
		CALL(init_dep_subtask(thread_pid[ctx->id]));

	pthread_mutex_unlock(&count_lock);

	// Signal the set constraints thread to specify constraints
	if (--remaining_threads_count == 0) {
		pthread_cond_signal(&count_cond);
	}

	// Wait for the set constraints thread to specify the constraints and dependencies
	pthread_mutex_lock(&constraints_lock);
	while (constraints_set < 0) {
		pthread_cond_wait(&constraints_cond, &constraints_lock);
	}
	pthread_mutex_unlock(&constraints_lock);

	/*****
	 * 2) Transition to real-time mode.
	 */
	CALL( task_mode(LITMUS_RT_TASK) );

	/* The task is now executing as a real-time task if the call didn't fail.
	 */

	/*****
	 * 3) Invoke real-time jobs.
	 */
	do {
		/* Wait until the next job is released. */
		sleep_next_period();
		/* Invoke job. */
		do_exit = job(ctx->id);
	} while (!do_exit);



	/*****
	 * 4) Transition to background mode.
	 */
	CALL( task_mode(BACKGROUND_TASK) );


	return NULL;
}

// Specify constraints:
void* set_constraint_thread(void *context) {

	// Wait for the threads to obtain their pids
	pthread_mutex_lock(&count_lock);
		while (remaining_threads_count > 0) {
			pthread_cond_wait(&count_cond, &count_lock);
		}
	pthread_mutex_unlock(&count_lock);

//	pthread_mutex_lock(&constraints_lock);

	// Specify Constraints
	CALL(add_parent_to_subtask_in_main_task(thread_pid[3], thread_pid[4], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[3], thread_pid[5], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[3], thread_pid[7], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[4], thread_pid[8], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[4], thread_pid[2], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[5], thread_pid[2], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[7], thread_pid[9], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[8], thread_pid[6], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[2], thread_pid[1], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[2], thread_pid[0], main_thread_pid));
	CALL(add_parent_to_subtask_in_main_task(thread_pid[9], thread_pid[0], main_thread_pid));

	// Signal the threads to resume their operation
	constraints_set = 1;
	pthread_cond_broadcast(&constraints_cond);

//	pthread_mutex_unlock(&constraints_lock);
	return NULL;
}


int job(int thread_id)
{
	/* Do real-time calculation. */
	printf("\nExecuting job: (%d, %d).\n", thread_id, thread_pid[thread_id]);

	/* Don't exit. */
	// return 0;

	return 1;
}

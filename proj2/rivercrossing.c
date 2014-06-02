#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <time.h>

// Person types
#define TYPE_HACKERS		0
#define TYPE_SERFS			1

// Actions
#define ACTION_STARTED		0
#define ACTION_WAITING		1
#define ACTION_BOARDING		2
#define ACTION_LANDING		3
#define ACTION_MEMBER		4
#define ACTION_CAPTAIN		5
#define ACTION_FINISHED		6

// Semaphors identifers and macro for auto choosing right type (Hackers/Serfs)
#define SEM_DATA			0

#define _SEM_WAITING		type == TYPE_HACKERS ? SEM_WAITING_HACKERS : SEM_WAITING_SERFS
#define SEM_WAITING_SERFS	1
#define	SEM_WAITING_HACKERS	2

#define SEM_BOARDING		3
#define	SEM_TRAVEL			4

#define _SEM_LANDING		type == TYPE_HACKERS ? SEM_LANDING_HACKERS : SEM_LANDING_SERFS
#define SEM_LANDING_SERFS	5
#define SEM_LANDING_HACKERS	6

#define SEM_DONE			7

// Macro for easy using semaphores
#define SEM_ON(id)		semop(sem_id, &waitOp[ id ], 1);
#define SEM_OPEN(id)	semop(sem_id, &signalOp[ id ], 1);

// Macros for accessing shared memory and macro for auto choosing right type (Hackers/Serfs)
#define _COUNTER_WAITING		type == TYPE_HACKERS ? COUNTER_WAITING_HACKERS : COUNTER_WAITING_SERFS
#define COUNTER_WAITING_SERFS	0
#define COUNTER_WAITING_HACKERS	1

#define _COUNTER_SHIP			type == TYPE_HACKERS ? COUNTER_SHIP_HACKERS : COUNTER_SHIP_SERFS
#define COUNTER_SHIP_SERFS		2
#define COUNTER_SHIP_HACKERS	3

#define _COUNTER_FINISH			type == TYPE_HACKERS ? COUNTER_FINISH_HACKERS : COUNTER_FINISH_SERFS
#define	COUNTER_FINISH_SERFS	4
#define	COUNTER_FINISH_HACKERS	5

#define	COUNTER_CAPTAIN_ID		6
#define	COUNTER_OUTPUT			7

// Return codes
#define RETURN_USAGE_ERROR	1
#define RETURN_FORK_ERROR	2
#define RETURN_ERROR		-1
#define RETURN_OK			0
 
// Semaphores structures
struct sembuf waitOp[8] =
{
    { .sem_num = 0, .sem_op = -1, .sem_flg = 0 }, 
    { .sem_num = 1, .sem_op = -1, .sem_flg = 0 }, 
    { .sem_num = 2, .sem_op = -1, .sem_flg = 0 }, 
    { .sem_num = 3, .sem_op = -1, .sem_flg = 0 }, 
    { .sem_num = 4, .sem_op = -1, .sem_flg = 0 }, 
    { .sem_num = 5, .sem_op = -1, .sem_flg = 0 }, 
    { .sem_num = 6, .sem_op = -1, .sem_flg = 0 }, 
    { .sem_num = 7, .sem_op = -1, .sem_flg = 0 }, 

};

struct sembuf signalOp[8] =
{
    { .sem_num = 0, .sem_op = 1, .sem_flg = 0 },
    { .sem_num = 1, .sem_op = 1, .sem_flg = 0 },
    { .sem_num = 2, .sem_op = 1, .sem_flg = 0 },
    { .sem_num = 3, .sem_op = 1, .sem_flg = 0 },
    { .sem_num = 4, .sem_op = 1, .sem_flg = 0 },
    { .sem_num = 5, .sem_op = 1, .sem_flg = 0 },
    { .sem_num = 6, .sem_op = 1, .sem_flg = 0 },
    { .sem_num = 7, .sem_op = 1, .sem_flg = 0 },
};

// Structure for program arguments
typedef struct
{
	int P;
	int h_max_wait;
	int s_max_wait;
	int voyage_max_wait;
} arguments;

// Function prototypes
void output(int* data, int type, int id, int action);
int random_number(int min, int max);
void do_actions(int* data, int index, int type, arguments *args, int sem_id);
void fork_persons(int type, arguments* args, int shm_id, int sem_id);
void show_help();
int check_arguments(arguments* args);
int throw_error(int code);
void term();


int sem_id;
int shm_id;
FILE *fp;

/**
 *	Main function 
 *
 *	@param	argc	number of arguments
 *	@param	argv	arguments array
 * 	@return	exit code
 */
int main(int argc, char* argv[])
{
	// Handle sigterm for correct return code, because we send sigterm to all processes if is problem with forking for cleaning
	struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);
	

	// Arguments
	if(argc < 5 && !(argc == 2 && strcmp(argv[1],"--help") == 0))
	{
		return throw_error(RETURN_USAGE_ERROR);
	}
	else if(argc == 2 && strcmp(argv[1],"--help") == 0)
	{ // help
		show_help();
		return 0;
	}
	arguments args = { atoi(argv[1]), atoi(argv[2]),  atoi(argv[3]), atoi(argv[4])};

	if(check_arguments(&args) == RETURN_ERROR)
	{ // Check arguments
		return throw_error(RETURN_USAGE_ERROR);
	}

	// Generate shared memory and semaphores keys
    key_t sem_key = ftok("./rivercrossing.c", 1);
    key_t shm_key = ftok("./rivercrossing.c", 2);

	// Create semaphore id and shared memory id
    sem_id = semget(sem_key, 8, IPC_CREAT | IPC_EXCL | 0644);
    shm_id = shmget(shm_key, sizeof(int)*(8), IPC_CREAT | IPC_EXCL | 0644);

	// Inicialize semaphores
    semctl(sem_id, 0, SETVAL, 1);
    semctl(sem_id, 1, SETVAL, 1);
    semctl(sem_id, 2, SETVAL, 1);
    semctl(sem_id, 3, SETVAL, 0);
    semctl(sem_id, 4, SETVAL, 0);
    semctl(sem_id, 5, SETVAL, 0);
    semctl(sem_id, 6, SETVAL, 0);
    semctl(sem_id, 7, SETVAL, 0);

	int status1 = 0, status2 = 0;

	// Wipe file
	fp = fopen("rivercrossing.out","w");
	

    // Fork on child process for generating hackers
    pid_t pid = fork();
    if (pid == 0)
    {
		fork_persons(TYPE_HACKERS,&args,shm_id,sem_id);
        exit(0);
    }
    else if (pid > 0)
    {
		int pid2;
		pid2 = fork();
		// Fork on second child process for generating serfs
		if(pid2 == 0)
		{
			fork_persons(TYPE_SERFS,&args,shm_id,sem_id);
			exit(0);
		}
		else if(pid2 < 0)
		{
			status2 = 2;
		}
        // Wait on generating processes
        
        waitpid(0, &status1, 0);
        if(status1 != 0)
        {
			kill(-1*getpid(),SIGTERM);
		}
        waitpid(0, &status2, 0);
        if(status2 != 0)
		{
			kill(-1*getpid(),SIGTERM);
		}
    }
    else
    {
		status2 = 2;
	}
    
    // Clean semaphores
    for(int i = 0; i < 8; i++)
    {
		semctl(sem_id, i, IPC_RMID);
	}
	
    // Clean shared memory
    shmctl(shm_id, IPC_RMID, NULL);

	// Check childs exit code
    if(status1 != 0 || status2 != 0)
    {
		return throw_error(RETURN_FORK_ERROR);
	}
    
    return 0;
}


/**
 *	Check parsed app arguments on correct values
 *
 *	@param	args	arguments structure to be checked
 *	@return	RETURN_OK/RETURN_ERROR
 */
int check_arguments(arguments* args)
{
	if(args->P < 0 || args->P % 2 != 0)
	{ // Check P argument
		return RETURN_ERROR;
	}

	if(args->h_max_wait < 0 || args->h_max_wait > 5000)
	{ // Check H argument
		return RETURN_ERROR;
	}

	if(args->s_max_wait < 0 || args->s_max_wait > 5000)
	{ // Check S argument
		return RETURN_ERROR;
	}

	if(args->voyage_max_wait < 0 || args->voyage_max_wait > 5000)
	{ // Check R argument
		return RETURN_ERROR;
	}

	return RETURN_OK;
}


/**
 *	Print error on stderr and return code for exit()
 *	
 *	@param	code	Code of error to show
 * 	@return exit application code
 */
int throw_error(int code)
{
	if(code == RETURN_USAGE_ERROR)
	{
		fprintf(stderr,"Zadane spatne argumenty. Podivejte se --help\n");
		return RETURN_USAGE_ERROR;
	}
	else if(code == RETURN_FORK_ERROR)
	{
		fprintf(stderr,"Chyba pri forkovani procesu!\n");
		return RETURN_FORK_ERROR;
	}
	else
	{
		fprintf(stderr,"Unkown error\n");
		return 93; // selected via roll of the dices
	}
}


/**
 * 	Print help on stdout
 */
void show_help()
{
	printf("Projektu 2 do predmetu IOS na FIT VUT\n");
	printf("Aplikace implementuje synchronizacni problem River Crossing Problem za pouziti sdilene pameti a semaforu. Byl zvolen postup za pouziti sys-v.\n");
	printf("Argumenty:\n");
	printf("   --help\t Vypis teto napovedy pouziti\n");
	printf("   A B C D\t Vsechny hodnoty jsou celociselne\n");
	printf("\t\t A - Pocet clenu kazdeho druhu. Musi byt delitelne 2 s nulovym zbytkem.\n");
	printf("\t\t B - Maximalni doba uspani mezi generovani procesu 1. druhu.\n");
	printf("\t\t C - Maximalni doba uspani mezi generovani procesu 2. druhu.\n");
	printf("\t\t D - Maximalni doba 1 plavby lodi.\n\n");
	printf("\t Vytvoril: Stanislav Nechutny - xnechu01\n");
}


/**
 *	Print right formated output to file. Semaphore for data must be closed
 *
 *	@param	data	Pointer on shared memory with datas
 * 	@param	type	Type of person - TYPE_HACKERS/TYPE_SERFS
 * 	@param	id		Person id
 * 	@param	action	What action print - ACTION_* macro
 */
void output(int* data, int type, int id, int action)
{
	char* type_s;
	char* action_s;
	int with_count = 0;

	// Stringify type
	if(type == TYPE_HACKERS)
	{
		type_s = "hacker";
	}
	else
	{
		type_s = "serf";
	}

	// Stringify action
	if(action == ACTION_BOARDING)
	{
		action_s = "boarding";
		with_count = 1;
	}
	else if(action == ACTION_CAPTAIN)
	{
		action_s = "captain";
	}
	else if(action == ACTION_FINISHED)
	{
		action_s = "finished";
	}
	else if (action == ACTION_LANDING)
	{
		action_s = "landing";
		with_count = 1;
	}
	else if(action == ACTION_MEMBER)
	{
		action_s = "member";
	}
	else if(action == ACTION_STARTED)
	{
		action_s = "started";
	}
	else if(action == ACTION_WAITING)
	{
		action_s = "waiting for boarding";
		with_count = 1;
	}

	

	// Print and choose right format
	if(with_count)
	{
		fprintf(fp,"%i: %s: %d: %s: %d: %d\n",++data[ COUNTER_OUTPUT ],type_s,id+1,action_s,data[ COUNTER_WAITING_HACKERS ],data[ COUNTER_WAITING_SERFS] );
	}
	else
	{
		fprintf(fp,"%i: %s: %d: %s\n",++data[ COUNTER_OUTPUT ],type_s,id+1,action_s);
	}
	fflush(fp);
}


/**
 *	Return random number from range min (including) to max (including)
 * 	IMPORTANT: must run srand(time(NULL)); before first call
 *
 *	@param	min	Minimal value of range
 *	@param	max	Maximal value of range
 *	@return	random number from range
 */
int random_number(int min, int max)
{
	int result = rand() % (max-min+1);
	return result+min;
}


/**
 *	Control proccess waiting, boarding, controlling shift, landing etc.
 * 	Just magic function, can make ice cream too...
 *	
 *	@param	data	pointer on shared memory
 *	@param	index	number of process
 *	@param	type	type of process - TYPE_HACKERS/TYPE_SERFS
 *	@param	args	structure of parsed app arguments
 * 	@param	sem_id	semaphore id
 *
 */
void do_actions(int* data, int index, int type, arguments *args, int sem_id)
{
	int is_capitan = 0;
	
			SEM_ON(_SEM_WAITING); // waiting on boarding
				SEM_ON(SEM_DATA);
				 // on pier
				data[_COUNTER_WAITING]++;
				output(data, type, index, ACTION_WAITING);
				
				if(data[ _COUNTER_SHIP ]+data[ _COUNTER_WAITING ] <= 1)
				{ // Allow boarding for next if is enought space
					SEM_OPEN(_SEM_WAITING);
				}
				else if( (data[ COUNTER_SHIP_HACKERS ]+data[ COUNTER_WAITING_HACKERS ]) == 2 && (data[ COUNTER_SHIP_SERFS ]+data[ COUNTER_WAITING_SERFS ]) == 2)
				{ // Start boarding if are all on pier
					is_capitan = 1;
					SEM_OPEN(SEM_BOARDING);
				}
				SEM_OPEN(SEM_DATA);

				SEM_ON(SEM_BOARDING); // start boarding
					SEM_ON(SEM_DATA);

						// print information about boarding
						output(data, type, index, ACTION_BOARDING);
						
						if(is_capitan)
						{ // set as captain if is first
							data[COUNTER_CAPTAIN_ID] = index+args->P*type;
							output(data, type, index, ACTION_CAPTAIN);
						}
						else
						{ // or just as member
							output(data, type, index, ACTION_MEMBER);
						}

						// add to ship counter
						data[ _COUNTER_WAITING ]--;
						data[ _COUNTER_SHIP ]++;

						
						
						//	ugly goto for returning ship. This is my first goto and i hate myself for it!
						//	http://xkcd.com/292/
						target:
						
						if(data[COUNTER_SHIP_HACKERS]+data[COUNTER_SHIP_SERFS] < 4)
						{ // Allow boarding if is space in ship
							SEM_OPEN(SEM_BOARDING);
						}
						if(data[COUNTER_SHIP_HACKERS]+data[COUNTER_SHIP_SERFS] == 4)
						{ // Ship is full so start travel
							SEM_OPEN(SEM_TRAVEL);
						}
						SEM_OPEN(SEM_DATA);

						if(is_capitan)
						{ //Captain sleep and then unlock other passagers
							SEM_ON(SEM_TRAVEL);
							usleep(random_number(0,args->voyage_max_wait)*1000);
							SEM_OPEN(SEM_LANDING_HACKERS);
							SEM_OPEN(SEM_LANDING_SERFS);
						}
						else
						{ // passagers are waiting on travel end via semaphore
							SEM_ON(_SEM_LANDING);
							SEM_OPEN(_SEM_LANDING);
						}

						// travel ended so start landing
						SEM_ON(SEM_DATA);
							if(data[ _COUNTER_SHIP ] == 2 || data[COUNTER_SHIP_HACKERS]+data[COUNTER_FINISH_HACKERS] == args->P)
							{ // They can land if it's last travel, or are first of their type
								data[ _COUNTER_SHIP ]--;
								
								if(is_capitan)
								{ // If is captain landing, then he must free captain id for other member
									data[ COUNTER_CAPTAIN_ID ] = -1;
									is_capitan = 0;
								}
								// landing output
								output(data, type, index, ACTION_LANDING);
								
								data[ _COUNTER_FINISH ]++;
								// finished output
								output(data, type, index, ACTION_FINISHED);
								
								if(data[COUNTER_SHIP_HACKERS]+data[COUNTER_FINISH_HACKERS] == args->P)
								{ // all are on second pier, so open SEM_DONE
									SEM_OPEN(SEM_DONE);
								}
								SEM_OPEN(SEM_DATA);

								// are on second pier so wait on SEM_DONE
								SEM_ON(SEM_DONE);
									SEM_OPEN(SEM_DONE);
							}
							else
							{ // Wait on ship for return
								if(data[ COUNTER_CAPTAIN_ID ] == -1)
								{ // Captain left ship - get this place
									data[ COUNTER_CAPTAIN_ID ] = index;
									is_capitan = 1;
								}

								if(data[ COUNTER_SHIP_HACKERS ] == 1 && data[ COUNTER_SHIP_SERFS ] == 1)
								{ // 2 members on ship so return on first pier
									SEM_OPEN(SEM_TRAVEL);
								}

								SEM_OPEN(SEM_DATA);

								if(is_capitan)
								{ // Captain is returning - sleep, unlock waiting ang jump to black hole
									SEM_ON(SEM_TRAVEL);
									usleep(random_number(0,args->voyage_max_wait)*1000);
									SEM_OPEN(SEM_BOARDING);
									SEM_OPEN(SEM_WAITING_HACKERS);
									SEM_OPEN(SEM_WAITING_SERFS);
									SEM_ON(SEM_DATA);
									is_capitan = 0;
									data[ COUNTER_CAPTAIN_ID ] = -1; 
									goto target;
								}
								else
								{ // travel complete - jump back in flow
									SEM_ON(SEM_BOARDING);
									SEM_OPEN(SEM_BOARDING);
									SEM_ON(SEM_DATA);
									goto target;
								}
							}	
}


/**
 *	Function for forking processes of given type
 *
 *	@param	type	what type to fork - TYPE_HACKERS/TYPE_SERFS
 *	@param	args	parsed app arguments
 *	@param	shm_id	key for shared memory
 * 	@param	sem_id	key for semaphore
 */
void fork_persons(int type, arguments* args, int shm_id, int sem_id)
{ 
	srand(time(NULL));
	// prepare sleep time
	int sleeping = args->h_max_wait;
	if(type == TYPE_SERFS)
	{
		sleeping = args->s_max_wait;
	}

	// alloc array for pids for correct waiting
	int* pid = malloc(sizeof(int)*args->P);
	
	for(int i = 0; i < args->P; i++)
	{ // fork loop
		// sleep
		usleep(random_number(0,sleeping)*1000);
		SEM_ON(SEM_DATA);
		pid[i] = fork();	
		if(pid[i] == 0)
		{ // fork
			srand(time(NULL));
			
			// get shared memory pointer
			int *data = shmat(shm_id, NULL, 0);

			output(data,type, i, ACTION_STARTED);
			SEM_OPEN(SEM_DATA);

			// jump to magic function, that can find solution for every problem
			do_actions(data, i, type, args, sem_id);
			
			// unmap shared memory, free arays and die
			shmdt(data);
			free(pid);
			
			exit(0);
		}
		else if(pid[i] < 0)
		{ // fork error
			free(pid);
			fclose(fp);
			exit(RETURN_FORK_ERROR);
		}

	// wait on all forks
	}
	for(int i = 0; i < args->P; i++)
	{
		waitpid(pid[i],NULL,0);
		
	}
	// free allovcated memory
	free(pid);
}

/**
 * 	Handle SIGTERM - Send to all childs and parents if is fork error, so show error and exit with right code
 *	Kill it with fire before it lays eggs
 */
void term()
{
	 // Clean semaphores
    for(int i = 0; i < 8; i++)
    {
		semctl(sem_id, i, IPC_RMID);
	}
	
    // Clean shared memory
    shmctl(shm_id, IPC_RMID, NULL);

	fclose(fp);
	
    exit(throw_error(RETURN_FORK_ERROR));
}

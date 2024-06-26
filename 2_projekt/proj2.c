//IOS project 2, author: Igor Lacko (xlackoi00)
//proj2.c, main file for project 2

#include "proj2.h"

//output file
FILE *out = NULL;

//shared variables
long int *A = NULL;
long int *finished_skiers = NULL;

//shared semaphores
sem_t *mutex = NULL;
sem_t *boarding = NULL;
sem_t *last_stop = NULL;

//shared structures
bus_stop *stops = NULL;
skibus *bus = NULL;



//validates if the arguments don't exceed their capacity (or are lower than their capacity)
void CapacityCheck(long int *args){
    long int upper_bound[] = {19999, 10, 100, 10000, 1000};
    long int lower_bound[] = {1, 1, 10, 0, 0};
    for(int i = 0; i < 5; i++){
        if(args[i] > upper_bound[i] || args[i] < lower_bound[i])
            ErrorExit("Error: Argument out of bounds!\n");
    }
}

//prints out the error message it gets as an argument to stderr and exits the program
void ErrorExit(const char *error_message){
    fprintf(stderr, "%s", error_message);
    exit(1);
}

//initializes the needed shared variables to their starting values
void SharedMemoryInit(long int *args){

    //allocating shared semaphores
    mutex = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    boarding = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    last_stop = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);

    //shared variables
    finished_skiers = mmap(NULL, sizeof(long int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    A = mmap(NULL, sizeof(long int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);

    //shared structures
    stops = mmap(NULL, args[1] * sizeof(bus_stop), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    bus = mmap(NULL, sizeof(skibus), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
    
    //validation 
    if((A == MAP_FAILED) || (mutex == MAP_FAILED) || (last_stop == MAP_FAILED) || (stops == MAP_FAILED) || 
    (bus == MAP_FAILED) || (boarding == MAP_FAILED) || (finished_skiers == MAP_FAILED)){
        ErrorExit("Error: Couldn't allocate shared memory!\n");
    }

    //value initialization
    *A = 1; *finished_skiers = 0; 
    sem_init(mutex, 1, 1); sem_init(boarding, 1, 0); sem_init(last_stop, 1, 0); 
    //allocating the queue semaphore in the bus stop structure
    for(long int i = 0; i < args[1]; i++){
        stops[i].n_waiting = 0; stops[i].id = i + 1; 
        if((stops[i].queue = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0)) == NULL){
            ErrorExit("Error: semaphore allocation failed!\n");
        }
        else sem_init(stops[i].queue, 1,0);
    }

    //checking proj2.out 
    if((out = fopen("proj2.out", "w")) == NULL) ErrorExit("Error: Couldn't open proj2.out!\n");
    setbuf(out, NULL);
    
}

//unmaps the allocated shared memory, destroys all semaphores and closes output 
void SharedMemoryFree(long int *args){
    //semaphore destruction
    sem_destroy(mutex);
    sem_destroy(boarding);
    for(long int i = 0; i < args[1]; i++){
        sem_destroy(stops[i].queue);
        munmap(stops[i].queue, sizeof(sem_t));
    }

    //shared variables unmapping
    munmap(A, sizeof(long int));
    munmap(finished_skiers, sizeof(long int));

    //semaphore unmapping
    munmap(mutex, sizeof(sem_t));
    munmap(boarding, sizeof(sem_t));

    //structure unmapping
    munmap(stops, args[1] * sizeof(bus_stop));
    munmap(bus, sizeof(skibus));

    //closing file
    fclose(out); 
}

//lets all skiers finish by unlocking all their semaphores
void ProcKill(long int *args){

    //for each skier
    for(int i = 0; i < args[0]; i++){ 

        //for each stop
        for(int j = 0; j < args[1]; j++){
            sem_post(stops[j].queue);
        }

        sem_post(mutex);
        sem_post(last_stop);
    }

}


//controls the life of the skier process
void Skier(long int *args, skier L){
    
    //random number generator seed
    srand(time(NULL) * getpid()); 
    
    sem_wait(mutex);

    fprintf(out, "%ld: L %ld: started\n", *A, L.id);
    (*A) ++;

    sem_post(mutex);

    usleep(rand() % (args[3] + 1)); 

    sem_wait(mutex);
    
    fprintf(out, "%ld: L %ld: arrived to %ld\n", *A, L.id, L.starting_stop);
    (*A) ++;
    stops[L.starting_stop - 1].n_waiting ++;

    sem_post(mutex);

    sem_wait(stops[L.starting_stop - 1].queue);

    sem_wait(mutex);

    fprintf(out, "%ld: L %ld: boarding\n", *A, L.id);
    (*A) ++;
    stops[L.starting_stop - 1].n_waiting --; bus->n_passengers ++;
    sem_post(boarding);

    sem_post(mutex);

    //waiting for the bus to reach it's destination
    sem_wait(last_stop);

    sem_wait(mutex);

    fprintf(out, "%ld: L %ld: going to ski\n", *A, L.id);
    (*A) ++;
    (*finished_skiers) ++;
    bus->n_passengers --;
    sem_post(boarding);

    sem_post(mutex);

    fclose(out);
    exit(0); //end of the skier process

}

//controls the life of the skibus process
void Skibus(long int *args){
    srand(time(NULL) * getpid()); //random number generator seed

    //initializing the shared bus variable representing the skibus

    bus -> capacity = args[2]; bus -> current_stop = 1; bus -> last_stop = args[1] + 1; bus -> n_passengers = 0;
    

    sem_wait(mutex);

    fprintf(out, "%ld: BUS: started\n", *A);
    (*A) ++;

    sem_post(mutex);

    //time to get to the first stop
    usleep(rand() % (args[4] + 1));

    while(true){


        //if we haven't reached the end yet
        if(bus->current_stop != bus->last_stop){
            sem_wait(mutex);
            
            fprintf(out, "%ld: BUS: arrived to %ld\n", *A, bus->current_stop);
            (*A) ++;
            
            sem_post(mutex);

            if((bus->capacity == bus->n_passengers) || (stops[bus->current_stop - 1].n_waiting == 0)){
                sem_wait(mutex);
                fprintf(out, "%ld: BUS: leaving %ld\n", *A, bus->current_stop);
                (*A) ++;
                bus->current_stop ++;
                sem_post(mutex);
                
                usleep(rand() % (args[4] + 1));
                continue;
            }
            else{
                //save the number of free seats so there isn't conflict in the for loop 
                long int free_seats = bus->capacity - bus->n_passengers;

                for(long int i = 0; i < free_seats; i++){
                    sem_post(stops[bus->current_stop - 1].queue); //unlock a spot in the stop's queue
                    sem_wait(boarding); //wait for the skier to board the bus

                    //if nobody else is on the bus stop we can leave
                    if(stops[bus->current_stop - 1].n_waiting == 0){ 
                        break;
                    }
                }
                sem_wait(mutex);
                fprintf(out, "%ld: BUS: leaving %ld\n", *A, bus->current_stop);
                (*A) ++;
                bus->current_stop ++;
                sem_post(mutex);
                
                usleep(rand() % (args[4] + 1)); 
                continue;
            }


        }
        else{ //bus has reached the final
            sem_wait(mutex);

            fprintf(out, "%ld: BUS: arrived to final\n", *A);
            (*A) ++;

            sem_post(mutex);

            //if nobody is on the bus
            if(bus->n_passengers == 0){
                sem_wait(mutex);

                fprintf(out, "%ld: BUS: leaving final\n", *A);
                (*A) ++;
                bus->current_stop = 1;

                sem_post(mutex);

                usleep(rand() % (args[4] + 1));
                continue;
            }

            else{ //wait for the skiers to leave the bus

                //save the number of skiers to exit the bus
                long int passengers_at_end = bus->n_passengers; 

                for(long int i = 0; i < passengers_at_end; i++){
                    sem_post(last_stop); //unlock the door for a skier
                    sem_wait(boarding); //wait for the skier to signal to the bus that he got out
                }

                //bus leaving final
                sem_wait(mutex);

                fprintf(out, "%ld: BUS: leaving final\n", *A);
                (*A) ++;

                sem_post(mutex);

                //check if anyone is still waiting

                //bus can finish
                if(*finished_skiers == args[0]){ 
                    sem_wait(mutex);

                    fprintf(out, "%ld: BUS: finish\n", *A);
                    (*A) ++;

                    sem_post(mutex);
                    break;
                }

                //the bus has to go another round
                else{
                    usleep(rand() % (args[4] + 1));
                    bus->current_stop = 1;
                    continue;
                }
            }

        }

    }
    fclose(out);
    exit(0);
}

int main(int argc, char **argv){
    //parsing arguments
    if(argc != 6){ //invalid argument count
        ErrorExit("Error: Invalid argument count!\n");
    }
    

    char *stringpart = NULL;
    long int arg_nums[5]; //array containing the arguments of the program
    
    
    for(int i = 0; i < 5; i++){ //argument initialization
        arg_nums[i] = strtol(argv[i + 1], &stringpart, 10); //arg array is {L, Z, K, TL, TB}
        if(strlen(stringpart)){ //checking the format of the arguments
            ErrorExit("Error: Invalid argument format!\n");
        }
    }
    
    CapacityCheck(arg_nums); //checking the capacity of the arguments 
    
    //shared memory initialization
    SharedMemoryInit(arg_nums);

    //random number generator seed
    srand(time(NULL) * getpid());


    //creating the skibus process
    pid_t skibus = fork();
    if(skibus < 0){ //fork error
        SharedMemoryFree(arg_nums);
        ErrorExit("Error: Couldn't create the skibus process!\n");
    }
        
    //we are in the main/parent process
    else if(skibus > 0){ 
    
        //creating the skiers
        for(long int i = 0; i < arg_nums[0]; i++){
            
            //intiialize the skier structure
            skier L = {i + 1, (rand() % arg_nums[1]) + 1};
            
            //create a child process
            pid_t process = fork();
            
            if(process < 0){ //fork error
                ProcKill(arg_nums);
                kill(skibus, SIGKILL);
                while(wait(NULL) > 0) continue;
                SharedMemoryFree(arg_nums);
                ErrorExit("Error: Couldn't create the skier process!\n");

            }
            else if(process == 0) //we are in the child process
                Skier(arg_nums, L);
        }
    }

    //we are in the skibus process
    else Skibus(arg_nums);

    //wait for all children to finish
    while(wait(NULL) > 0);
    SharedMemoryFree(arg_nums);
    exit(0); 
}



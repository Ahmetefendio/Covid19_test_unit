#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h> 

// The maximum number of people and healthstaff threads.
#define MAX_PeopleS 150
#define max_healthstaff 8

void *People(void *num);
void *healthstaff(void *num);

//Defining the semaphores.

//Limit the number of people allowed to enter the waiting room at one time. (50)
sem_t waitingRoom;

//Limits the number of people allowed to enter the test room at one time. (24)
sem_t testRoom;

//Healthstaff is waiting until first person comes.
sem_t alertStaff;

//This semaphore holds people threads until test is done.
sem_t waitDuringTest;

//This semaphore holds healthstaff until second and last people comes to room to start the covid test.
sem_t fillingRoom;

//This semaphore allows us to make a queue to avoid conglomerating.
sem_t enteringQueue;

// Flag to stop the healthstaff threads when all Peoples have been tested.
int allDone = 0;

//Counter for counting every three people.
int count = 0;


int main(int argc, char *argv[])
{
    pthread_t hstid[max_healthstaff];
    pthread_t tid[MAX_PeopleS];
    int i, x, numPeoples, numChairs; int Number[MAX_PeopleS];

    numPeoples = 99;
    numChairs = 50;

    for (i = 0; i < MAX_PeopleS; i++) {
        Number[i] = i;
    }
    // Initialize the semaphores with initial values...
    sem_init(&waitingRoom, 0, numChairs);
    sem_init(&testRoom, 0, 24);
    sem_init(&alertStaff, 0, 0);
    sem_init(&waitDuringTest, 0, 0);
    sem_init(&fillingRoom, 0, 0);
    sem_init(&enteringQueue, 0, 1);

   //Create 8 healthstaffs.
    for (i = 0; i < max_healthstaff; i++){
        pthread_create(&hstid[i], NULL, healthstaff, (void *)&Number[i+1]);
	//usleep(1000);
    }
    

    // Create the peoples.
    for (i = 0; i < numPeoples; i++) {
        pthread_create(&tid[i], NULL, People, (void *)&Number[i+1]);
    }

    // Join each of the threads to wait for them to finish.
    for (i = 0; i < numPeoples; i++) {
        pthread_join(tid[i],NULL);
    }

    // When all of the Peoples are tested, kill the healthstaff threads.

    allDone = 1;

    for (i = 0; i < max_healthstaff; i++) {
        sem_post(&alertStaff); // Alert the healthstaffs so they will exit.
    }
    
    // Join each of the threads to wait for them to finish.
    for (i = 0; i < max_healthstaff; i++) {
        pthread_join(hstid[i], NULL);
    }
    printf("Hospital is closing.\n");  

    return 0;
}

void *People(void *number) {
     int pNum = *(int *)number;
     
      // Wait for space to open up in the waiting room.
     sem_wait(&waitingRoom);
     printf("+ People %d is entering the hospital and waiting room. \n", pNum);

     // Wait for the testroom to become free.
     sem_wait(&testRoom); // this semaphore allows 24 people which is total capacity of test rooms.

     
     //This part is responsible for making queue to avoid conglomerating.
     sem_wait(&enteringQueue);
     usleep(10000);
     sem_post(&enteringQueue);


     sem_post(&waitingRoom);// Allow a new person to enter the waiting room.

     
     if(count % 3 == 0){ // If the room is empty.
        printf("People %d alerting the staff and filling the form.\n", pNum);
        sem_post(&alertStaff); // Alerting the healthstaff
	count++;
     }
     else{ // If room has one or two people in it.
	printf("People %d filling the form.\n", pNum);
	sem_post(&fillingRoom);
        count++;
     }
     sem_wait(&waitDuringTest); // Wait until the test is over.
     usleep(10000); // elapsed time when people are leaving.
     printf("- People %d leaving the hospital.\n", pNum);
     sem_post(&testRoom);// Allow a new person to enter the test room.
     
     }

void *healthstaff(void *number1){
    int hsNum=*(int *)number1;
    while (!allDone) { 

    // Waiting until someone arrives and alerts healthstaff.
    sem_wait(&alertStaff);

    if (!allDone)
    {
	printf("Covid 19 test unit %d: [X |  |  ]\n",hsNum);
	printf("\tLast 2 people, Please pay attention to your social distance and use a mask\n");

	sem_wait(&fillingRoom); // waiting second people
	printf("Covid 19 test unit %d: [X | X |  ]\n",hsNum);
	printf("\tLast people, Please pay attention to your social distance and use a mask\n");

	sem_wait(&fillingRoom); // waiting last people
	printf("Covid 19 test unit %d: [X | X | X]\n",hsNum);
        printf("\tCovid test unit %d 's medical staff apply the covid test.\n\n",hsNum);

        sleep(5); // elapsed time during the test.

        sem_post(&waitDuringTest); // When the test is over, three people leaves the room.
        sem_post(&waitDuringTest);
        sem_post(&waitDuringTest);
	
        printf("The healthstaff is ventilating the room %d \n",hsNum);
    }
    
   }
}

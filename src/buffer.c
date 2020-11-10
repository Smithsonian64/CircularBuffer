/**
 * Michael Smith
 * CS 474 Project 2
 * 11/9/2020
 */
#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

//create the semaphores
sem_t empty;
sem_t full;
sem_t mutex;

//method to create shared memory
void * create_shared_memory(size_t size) {

    int protection = PROT_READ | PROT_WRITE;
    int visibility = MAP_SHARED | MAP_ANONYMOUS;
    return mmap(NULL, size, protection, visibility, -1, 0);

}

//the producer thread
void * producerThread(void * buffer) {
    
    //create the file, buffer, counter and character variables
    int * buf = (int *)buffer;	
    int counter = 0;
    FILE *filePointer;
    char ch;

    //open the file
    filePointer = fopen("mytest.dat", "r");

    //return if file was not opened
    if (filePointer == NULL) {
        printf("File is not available \n");
    } else {

	//repeat until break
        while (1) {

	    //wait for empty and mutex sem posts, skips initially
            sem_wait(&empty);
            sem_wait(&mutex);

	    //repeat while counter < 15
            while(counter < 15) {
                //read a character, write it to buffer and increment counter
		ch = fgetc(filePointer);
		//if character is EOF, break
                if(ch == EOF) break;
                buf[counter++] = ch;
                printf("writing %c to buffer\n", ch);
            }
            if(ch == EOF) break;

	    //when buffer is full give control to consumer and wait
            printf("Buffer full transferring to consumer\n");
            buf[15] = 1;
            counter = 0;

            sem_post(&mutex);
            sem_post(&full);

        }
        buf[counter] = EOF;
    }
    //EOF was read, close file, write EOF to buffer, give control to consumer and terminate thread.
    fclose(filePointer);
    printf("File completely read, wrote EOF and transferred control to consumer\n");
    sem_post(&mutex);
    sem_post(&full);


}

//the consumer thread
void * consumerThread(void * buffer) {
    
	//initialize counter and buffer variables
	int counter;
	int * buf = (int *)buffer;

    while(1) {

	//wait for the producer to finish filling the buffer
        sem_wait(&full);
        sem_wait(&mutex);

	//while counter is < 15
        while(counter < 15) {
            
	    //read one character from the buffer, print it, and increment coutner
	    //if character at buffer index counter is EOF the break
	    if(buf[counter] == EOF) break;
            printf("Reading %c from buffer, value = %d\n", (char)buf[counter], (int)buf[counter]);
	    counter++;
            sleep(1);
        }
        if(buf[counter] == EOF) break;


	//buffer read, give control back to producer and wait
        printf("Buffer read, transferring control to producer\n");

        counter = 0;
        sem_post(&mutex);
        sem_post(&empty);

    }
    //EOF was read, terminate thread
    printf("read EOF, exiting\n");
}

int main(void) {

    //create shared memory buffer
    void * buffer = create_shared_memory(15);

    //initialize semaphores
    sem_init(&empty, 1, 1);
    sem_init(&full, 1, 0);
    sem_init(&mutex, 1, 1);

    //create threads
    pthread_t producer, consumer;
    
    //initialize threads
    pthread_create(&consumer,NULL,consumerThread,buffer);
    pthread_create(&producer,NULL,producerThread,buffer);

    //join threads
    pthread_join(producer,NULL);
    pthread_join(consumer,NULL);

    //release semaphores
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);

    //release shared memory
    shmdt(buffer);

    return 0;

}


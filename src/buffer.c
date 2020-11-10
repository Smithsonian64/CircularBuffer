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

sem_t empty;
sem_t full;
sem_t mutex;

void * create_shared_memory(size_t size) {

    int protection = PROT_READ | PROT_WRITE;
    int visibility = MAP_SHARED | MAP_ANONYMOUS;
    return mmap(NULL, size, protection, visibility, -1, 0);

}

void * producerThread(void * buffer) {
    
    int * buf = (int *)buffer;	

    int counter = 0;
    FILE *filePointer;
    char ch;
    filePointer = fopen("mytest.dat", "r");

    if (filePointer == NULL) {
        printf("File is not available \n");
    } else {
        while (1) {

            sem_wait(&empty);
            sem_wait(&mutex);

            while(counter < 15) {
                ch = fgetc(filePointer);
                if(ch == EOF) break;
                buf[counter++] = ch;
                printf("writing %c to buffer\n", ch);
            }
            if(ch == EOF) break;

            printf("Buffer full transferring to consumer\n");
            buf[15] = 1;
            counter = 0;

            sem_post(&mutex);
            sem_post(&full);

        }
        buf[counter] = EOF;
    }
    fclose(filePointer);
    printf("File completely read, wrote EOF and transferred control to consumer\n");
    sem_post(&mutex);
    sem_post(&full);


}

void * consumerThread(void * buffer) {
    
	int counter;
	int * buf = (int *)buffer;

    while(1) {

        sem_wait(&full);
        sem_wait(&mutex);

        while(counter < 15) {
            printf("Reading %c from buffer\n", (char)buf[counter++]);
            if(buf[counter] == EOF) break;
            sleep(1);
        }
        if(buf[counter] == EOF) break;

        printf("Buffer read, transferring control to producer\n");

        counter = 0;
        sem_post(&mutex);
        sem_post(&empty);

    }
    printf("read EOF, exiting\n");
}

int main(void) {

    void * buffer = create_shared_memory(15);

    sem_init(&empty, 1, 1);
    sem_init(&full, 1, 0);
    sem_init(&mutex, 1, 1);

    pthread_t producer, consumer;

    pthread_create(&consumer,NULL,consumerThread,buffer);
    pthread_create(&producer,NULL,producerThread,buffer);

    pthread_join(producer,NULL);
    pthread_join(consumer,NULL);

    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);

    shmdt(buffer);

    return 0;

}


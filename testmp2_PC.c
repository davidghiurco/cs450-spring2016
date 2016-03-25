#include"types.h"
#include"user.h"

#define BUFFER_SIZE 15
#define MAX_COUNT 25
#define NULL 0
int mutex;
int buffer[BUFFER_SIZE];

void buffer_init() {//initializes the buffer array
	int i;
	for (i=0; i<BUFFER_SIZE; i++) {
		buffer[i] = 0;
	}
}

void producer(void* arg1) {
  int i,j;
	int counter=1;
  for(i = 0; i < MAX_COUNT; i++)
	{
    mtx_lock(mutex);
    	sleep(1);
    	for(j=0; j< BUFFER_SIZE; j++, counter++){
      	if(buffer[j]==0){
      		buffer[j]=counter;
      		printf(1,"Producer put %d in buffer[%d]\n",buffer[j],j);
      	}
    	}
    mtx_unlock(mutex);
    sleep(1);
  }
  printf(1,"**************Producer Finished***************\n");
  exit();
}

void consumer(void* arg){
  int i,j;
  for (i = 0; i < MAX_COUNT; i++) {
    mtx_lock(mutex); // LOCKED Producer cannot interfere
    sleep(1);
    for(j=0;j<BUFFER_SIZE;j++){
      if(buffer[j]!=0){
        printf(1,"Consumer got %d from buffer[%d]\n",buffer[j],j);
        buffer[j]=0;
      }
    }//end for
     mtx_unlock(mutex);// UNLOCKED Producer can interfere
     sleep(1);
  }
  printf(1,"**************Consumer Finished***************\n");
  exit();
}

int main(){
  buffer_init();
  printf(1,"********** Main Started **********\n");
  mutex = mtx_create(0);
  uint* stack1 = (uint*) malloc(512);
  uint* stack2 = (uint*) malloc(512);

  uint* returnstack1 = (uint*) 0;
  uint* returnstack2 = (uint*) 0;

  thread_create(*producer, (void*)stack1, NULL);
  thread_create(*consumer, (void*)stack2, NULL);

  sleep(50);

	thread_join((void**) &returnstack1);
  thread_join((void**)&returnstack2);

	free(stack1);
  free(stack2);
  printf(1,"****************Main Finished*****************\n");
  exit();
  return 0;
}

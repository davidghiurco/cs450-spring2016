#include"types.h"
#include"user.h"
#define NULL 0


uint* stack1;
uint* stack2;
int shared_boolean = 0;
int mutex;



// stackwitching + arg passing testing
void consumer(void* arg){
  int i=0;
  while(i<50) {
    mtx_lock(mutex);
    while(shared_boolean != 0)
    {
      //printf(1,"****************\n");
      printf(1,"consumed %d from shared_boolean\n" ,shared_boolean);
      //printf(1,"****************\n");
      shared_boolean=0;
    }
     mtx_unlock(mutex);
     sleep(1);
     i++;
  }
  printf(1,"*******************Consumer Exit*****************\n");
    exit();

}

void producer(void* arg){
  int i;
  for (i=0; i < 50; i++) {
    mtx_lock(mutex);
      while(shared_boolean == 0)
      {
        shared_boolean = 1;
        printf(1,"Producer Produced %d in the shared_boolean\n",shared_boolean);
      }
    mtx_unlock(mutex);
    sleep(1);
  }
    printf(1,"*******************Producer Exit*****************\n");
    exit();

}

int main(int argc, char** argv){
printf(1,"******************Main Started*************************\n");
  mutex = mtx_create(0);
  //parent();
  stack1 = (uint*) malloc(512);
  stack2 = (uint*) malloc(512);


  uint* returnstack1 = (uint*)0;
  uint* returnstack2 = (uint*)0;

  // printf(1,"producer thread created\n");
  thread_create(*producer, (void*)stack1, NULL);
  // printf(1,"consumer thread created\n");
  thread_create(*consumer, (void*)stack2, NULL);

  sleep(50);

  thread_join((void **) &returnstack1);
  thread_join((void **) &returnstack2);

  free(returnstack1);
  free(returnstack2);

  printf(1, "*******************Main Finished*****************\n");
  exit();
  return 0;
}

 // sleep(50);

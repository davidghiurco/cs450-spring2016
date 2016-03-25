#include "types.h"
#include "user.h"
#include "syscall.h"


#define NUM_ELEMENTS 100
int mutex;

void producer(void *arg){

    int *buffer = (int*) arg;
    int i;
    for(i=0; i < NUM_ELEMENTS; i++){
        mtx_lock(mutex);
        buffer[i]= i*5;
        printf(1,"Producer put %d\n", buffer[i]);
        mtx_unlock(mutex);
        if(i==(NUM_ELEMENTS/2)){
            sleep(100);
        }
    }
    exit();
}

void consumer(void *arg){
    int *buffer = (int*) arg;
    int i;
    mtx_lock(mutex);
    printf(1,"Consumer has:[");
    for(i=0 ; i<NUM_ELEMENTS; i++){
        if(buffer[i]!=-1){
            printf(1,"%d,", buffer[i]);
            buffer[i]=-1;
        }
    }
    printf(1,"]\n");
    mtx_unlock(mutex);
    exit();
}

int main(int argc,char *argv[]){

    mutex = mtx_create(0);

    int *buffer= (int*) malloc(NUM_ELEMENTS * sizeof(int));
    memset(buffer, -1, NUM_ELEMENTS * sizeof(int*));

    uint *stack1= (uint *) malloc(1024);
    //uint *stack2=(uint *)malloc(1024);
    void *return_stack1;
    //void *return_stack2;

    thread_create(*producer,(void *)stack1,(void *)buffer);
    thread_join((void**)&return_stack1);

    thread_create(*consumer,(void *)stack1,(void *)buffer);
    thread_join((void**)&return_stack1);

    free(return_stack1);
    //free(return_stack2);
    free(buffer);
    exit();
    return 0;
}

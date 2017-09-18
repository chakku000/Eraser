#include <stdio.h>
#include <pthread.h>

int count=0;

void f(){
}

int main(){
    pthread_t th1;
    pthread_create(&th1,NULL,(void*)f,NULL);
    pthread_join(th1,NULL);
    //printf("count = %d\n",count);
}

#include <stdio.h>
#include <pthread.h>

//pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;


//int count=0;

void f(){}

int main(){
    fprintf(stderr,"main start\n");
    pthread_t th1;
    //pthread_t th2;
   /pthread_create(&th1,NULL,(void *)f,NULL);
    //pthread_create(&th2,NULL,(void *)f,NULL);
    //count++;
    //pthread_join(th1,NULL);
    //pthread_mutex_destroy(&mu);
    //fprintf(stderr,"main end (%d)\n",count);
    fprintf(stderr,"main end\n");
}

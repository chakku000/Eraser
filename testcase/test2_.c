#include <stdio.h>
#include <pthread.h>

int count = 0;

void f(){
    count++;
}

void g(){
    count++;
}

int main(){
    pthread_t th1,th2;
    pthread_create(&th1,NULL,(void*)f,NULL);
    pthread_create(&th2,NULL,(void*)g,NULL);
    pthread_join(th1,NULL);
    pthread_join(th2,NULL);
    printf("%p\n",&count);
    printf("%d\n",count);
    return 0;
}

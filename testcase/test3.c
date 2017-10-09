/*
 * test3
 * thread,lockを作成する.
 * ロックをしないで制御
 * 競合あり
 */
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
int count = 0;

void f(){
    //printf("%p\n",&count);
    count += 1;
}

void g(){
    count += 1;
}

int main(){
    pthread_t th1,th2;
    pthread_mutex_init(&mu,NULL);
    pthread_create(&th1,NULL,(void*)f,NULL);
    pthread_create(&th2,NULL,(void*)g,NULL);
    pthread_join(th1,NULL);
    pthread_join(th2,NULL);
    //printf("%d\n",count);
    return 0;
}

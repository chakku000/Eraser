/*
 * test3
 * thread,lockを作成する.
 * ロックを用いて制御
 * 競合なし
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;

int *ar;

void f(){
    pthread_mutex_lock(&mu);
    ar = malloc(sizeof(int)*10);
    pthread_mutex_unlock(&mu);
}

void g(){
    pthread_mutex_lock(&mu);
    ar[0] = 1;
    pthread_mutex_unlock(&mu);
}

int main(){
    pthread_t th1,th2;
    pthread_create(&th1,NULL,(void*)f,NULL);
    pthread_join(th1,NULL);
    pthread_create(&th2,NULL,(void*)g,NULL);
    pthread_join(th2,NULL);
    return 0;
}

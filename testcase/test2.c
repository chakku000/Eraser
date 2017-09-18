/*
 * test2
 * thread,lockを作成する.
 * ロックを用いて制御
 * 競合
 */
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;

int count = 0;

void f(){
    count++;
}

int main(){
    pthread_t th;
    pthread_create(&th,NULL,(void*)f,NULL);
    pthread_join(th,NULL);
    return 0;
}

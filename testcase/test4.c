/*
 * test3
 * thread,lockを作成する.
 * ロックを用いて制御
 * 競合なし
 */
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;

int count = 0;

void f(){
    pthread_mutex_lock(&mu);
    count++;
    pthread_mutex_unlock(&mu);
}

int main(){
    pthread_t th1,th2;
    pthread_create(&th1,NULL,(void*)f,NULL);
    pthread_create(&th2,NULL,(void*)f,NULL);
    pthread_join(th1,NULL);
    pthread_join(th2,NULL);
    return 0;
}

/*
 * test1
 * thread,lockを作成するがなにもしない
 * 競合なし
 */
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;

void f(){}

int main(){
    pthread_t th;
    pthread_create(&th,NULL,(void*)f,NULL);
    pthread_join(th,NULL);
    return 0;
}

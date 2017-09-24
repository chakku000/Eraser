/*
 * test1
 * threadを作成するがなにもしない
 * 競合なし
 */
#include <stdio.h>
#include <pthread.h>

void f(){}

int main(){
    pthread_t th;
    pthread_create(&th,NULL,(void*)f,NULL);
    pthread_join(th,NULL);
    return 0;
}

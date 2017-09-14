#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mu;

int count=0;

int main(){
    pthread_mutex_lock(&mu);
    count++;
    pthread_mutex_unlock(&mu);
}

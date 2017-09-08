#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mu;

int count=0;

int main(){
    puts("\n========= User Program Start ==========");
    pthread_mutex_lock(&mu);
    count++;
    printf("%d\n",count);
    pthread_mutex_unlock(&mu);
    puts("========= User Program End   ==========\n");
}

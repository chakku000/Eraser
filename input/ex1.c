#include <stdio.h>
#include <pthread.h>

#define NOLOCK

const size_t loop_max = 65535;

pthread_mutex_t mu1 = PTHREAD_MUTEX_INITIALIZER;

int counter_lock=0;

void count_with_lock(){
    printf("---------- New Thread Count Start ----------\n");
    pthread_mutex_lock(&mu1);
    counter_lock++;
    pthread_mutex_unlock(&mu1);
    printf("---------- New Thread Count End ----------\n");
}

int main(){
    puts("========== User Program Start ==========");
    pthread_t thread1;

    pthread_create(&thread1,NULL,(void*)count_with_lock,NULL);

    pthread_join(thread1,NULL);
    printf("with lock after thread1 : %d\n",counter_lock);

    pthread_mutex_t mu2 = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mu2);
    counter_lock++;
    pthread_mutex_unlock(&mu2);
    printf("with lock after thread0 : %d\n",counter_lock);

    pthread_mutex_destroy(&mu1);
    pthread_mutex_destroy(&mu2);
    puts("========== Done ==========");
    return 0;
}

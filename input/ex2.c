#include <stdio.h>
#include <pthread.h>

int main(){
    puts("\n========== User Program Start ==========");
    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&m);
    pthread_mutex_unlock(&m);
    puts("========== Done ==========\n");
}

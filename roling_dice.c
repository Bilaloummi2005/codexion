#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>


void* rolling_dice(){
    int value = (rand() % 6) + 1;
    int *result = malloc(sizeof(int));
    *result = value;
    printf("%p\n", result);
    printf("value = %d\n", value);
    return result;
}


int main(int ac, char* av[])
{
    srand(time(NULL));
    int *res;
    pthread_t th[4];
    int i = 0;
    for (i = 0; i < 4; i++){
        if (pthread_create(th + i, NULL, &rolling_dice, NULL))
        {
            return 1;
        }
    }
    for (i = 0; i < 4; i++){
        if (pthread_join(th[i], (void **) &res))
            return 2;
    }
    printf("%p\n", res);
    printf("resault %d:", *res);
    free(res);
    return 0;
}
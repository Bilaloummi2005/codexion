#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>  
#include <string.h>

#define RED   "\x1b[31m"
#define GRN   "\x1b[32m"
#define YEL   "\x1b[33m"
#define BLU   "\x1b[34m"
#define RESET "\x1b[0m"

typedef struct s_simulation {
    int         stop;           // flag: 1 = simulation should end
    pthread_mutex_t stop_mutex; // protects the stop flag
    pthread_mutex_t log_mutex;  // serializes all printf output
} t_simulation;

typedef struct s_waiter
{
    int  id;
    long deadline;  // last_compile + time_to_burnout
} t_waiter;

typedef struct s_dongle
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    long last_release;
    int             queue_size;         // how many are waiting
    int             *queue;  // waiting coder IDs in order
    t_waiter *edf_q;
    int edf_size;
} t_dongle;

typedef struct s_args
{
    long time_to_burnout;
    long time_to_compile;
    long time_to_debug;
    long time_to_refactor;
    long number_of_compiles_required;
    long dongle_cooldown;
    long n_coders;
    char* scheduler;
    int burned_out;
    int completed;
    pthread_mutex_t log_mutex;
    pthread_mutex_t burned_mutex;
} t_args;

typedef struct s_thread
{
    int id;
    int compile_count;
    long start_time;
    long last_compile;
    long deadline;
    pthread_mutex_t state_mutex;
    t_args *requirements;
    t_dongle *right_dongle;
    t_dongle *left_dongle;
} t_thread;

long get_time()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void log_state2(t_thread *data, char *msg, char *clr)
{
    long t;
    int id;
    pthread_mutex_lock(&data->requirements->log_mutex);
    t = get_time() - data->start_time;
    id = data->id;
    printf("%s%ld %d %s\n" RESET, clr, t, id, msg);
    pthread_mutex_unlock(&data->requirements->log_mutex);
}
void log_state(t_thread *data, char *msg, char *clr)
{
    pthread_mutex_lock(&data->requirements->log_mutex);
    pthread_mutex_lock(&data->requirements->burned_mutex);
    if (data->requirements->burned_out)
    {
        pthread_mutex_unlock(&data->requirements->burned_mutex);
        pthread_mutex_unlock(&data->requirements->log_mutex);
        return;
    }
    long t = get_time() - data->start_time;
    printf("%s%ld %d %s\n" RESET, clr, t, data->id, msg);
    pthread_mutex_unlock(&data->requirements->log_mutex);
    pthread_mutex_unlock(&data->requirements->burned_mutex);
}

void remove_from_queue(t_dongle *dongle)
{
    int i;
    i = 0;
    while (i < dongle->queue_size - 1)
    {
        dongle->queue[i] = dongle->queue[i + 1];
        i++;
    }
    dongle->queue_size--;
}

// scan queue — is anyone more urgent than me?
int someone_more_urgent(t_dongle *dongle, t_thread *data)
{
    for (int i = 0; i < dongle->edf_size; i++) {
        if (dongle->edf_q[i].id != data->id &&
            dongle->edf_q[i].deadline < data->deadline)
            return 1;
    }
    return 0;
}

// update my deadline in the queue after a compile reset
void update_deadline_in_queue(t_dongle *dongle, int id, long deadline)
{
    for (int i = 0; i < dongle->edf_size; i++) {
        if (dongle->edf_q[i].id == id) {
            dongle->edf_q[i].deadline = deadline;
            return;
        }
    }
}

// remove by id instead of by position
void remove_from_edf_queue(t_dongle *dongle, int id)
{
    for (int i = 0; i < dongle->edf_size; i++) {
        if (dongle->edf_q[i].id == id) {
            while (i < dongle->edf_size - 1) {
                dongle->edf_q[i] = dongle->edf_q[i + 1];
                i++;
            }
            dongle->edf_size--;
            return;
        }
    }
}

int is_burned_out(t_args *req)
{
    int val;
    pthread_mutex_lock(&req->burned_mutex);
    val = req->burned_out;
    pthread_mutex_unlock(&req->burned_mutex);
    return val;
}

void set_burned_out(t_args *req)
{
    pthread_mutex_lock(&req->burned_mutex);
    req->burned_out = 1;
    pthread_mutex_unlock(&req->burned_mutex);
}

int is_completed(t_args *req)
{
    int val;
    pthread_mutex_lock(&req->burned_mutex);
    val = req->completed;
    pthread_mutex_unlock(&req->burned_mutex);
    return val;
}

void set_completed(t_args *req)
{
    pthread_mutex_lock(&req->burned_mutex);
    req->completed = 1;
    pthread_mutex_unlock(&req->burned_mutex);
}

// Swap two waiters in the heap
void heap_swap(t_waiter *a, t_waiter *b)
{
    t_waiter tmp = *a;
    *a = *b;
    *b = tmp;
}

// Bubble up: after inserting at the end
void heap_bubble_up(t_waiter *heap, int index)
{
    int parent;
    while (index > 0)
    {
        parent = (index - 1) / 2;
        if (heap[index].deadline < heap[parent].deadline
            || (heap[index].deadline == heap[parent].deadline
                && heap[index].id < heap[parent].id))
        {
            heap_swap(&heap[index], &heap[parent]);
            index = parent;
        }
        else
            break;
    }
}

// Bubble down: after removing the root
void heap_bubble_down(t_waiter *heap, int size, int index)
{
    int smallest;
    int left;
    int right;

    while (1)
    {
        smallest = index;
        left = 2 * index + 1;
        right = 2 * index + 2;
        if (left < size
            && (heap[left].deadline < heap[smallest].deadline
                || (heap[left].deadline == heap[smallest].deadline
                    && heap[left].id < heap[smallest].id)))
            smallest = left;
        if (right < size
            && (heap[right].deadline < heap[smallest].deadline
                || (heap[right].deadline == heap[smallest].deadline
                    && heap[right].id < heap[smallest].id)))
            smallest = right;
        if (smallest != index)
        {
            heap_swap(&heap[index], &heap[smallest]);
            index = smallest;
        }
        else
            break;
    }
}

// Insert a waiter into the heap
void heap_push(t_dongle *dongle, int id, long deadline)
{
    dongle->edf_q[dongle->edf_size].id = id;
    dongle->edf_q[dongle->edf_size].deadline = deadline;
    dongle->edf_size++;
    heap_bubble_up(dongle->edf_q, dongle->edf_size - 1);
}

// Remove the root (the most urgent waiter)
t_waiter heap_pop(t_dongle *dongle)
{
    t_waiter top = dongle->edf_q[0];
    dongle->edf_size--;
    dongle->edf_q[0] = dongle->edf_q[dongle->edf_size];
    heap_bubble_down(dongle->edf_q, dongle->edf_size, 0);
    return top;
}

// Find a waiter by ID and update their deadline, then fix heap
void heap_update(t_dongle *dongle, int id, long new_deadline)
{
    int i = 0;
    while (i < dongle->edf_size)
    {
        if (dongle->edf_q[i].id == id)
        {
            dongle->edf_q[i].deadline = new_deadline;
            heap_bubble_up(dongle->edf_q, i);
            heap_bubble_down(dongle->edf_q, dongle->edf_size, i);
            return;
        }
        i++;
    }
}

// Remove a specific waiter by ID (for when they leave without winning)
void heap_remove_by_id(t_dongle *dongle, int id)
{
    int i = 0;
    while (i < dongle->edf_size)
    {
        if (dongle->edf_q[i].id == id)
        {
            dongle->edf_size--;
            dongle->edf_q[i] = dongle->edf_q[dongle->edf_size];
            heap_bubble_up(dongle->edf_q, i);
            heap_bubble_down(dongle->edf_q, dongle->edf_size, i);
            return;
        }
        i++;
    }
}

int lock_dongle(t_dongle *first_dongle, t_dongle *second_dongle, t_thread *data)
{
    struct timespec ts;
    int dongle_cooldown = data->requirements->dongle_cooldown;
    int use_edf = (strcmp(data->requirements->scheduler, "edf") == 0);

    // ── FIRST DONGLE ──────────────────────────────────────────
    pthread_mutex_lock(&first_dongle->mutex);

    if (use_edf) {
        // add to edf queue
        heap_push(first_dongle, data->id, data->deadline);
    } else {
        // add to fifo queue
        first_dongle->queue[first_dongle->queue_size] = data->id;
        first_dongle->queue_size++;
    }

    // wait until i am the most urgent (EDF) or first in line (FIFO)
    while (1) {
        if (is_burned_out(data->requirements)) {
            if (use_edf) {
                heap_remove_by_id(first_dongle, data->id);  // when leaving due to burnout
                pthread_cond_broadcast(&first_dongle->cond);
            } else {
                remove_from_queue(first_dongle);
                pthread_cond_broadcast(&first_dongle->cond);
            }
            pthread_mutex_unlock(&first_dongle->mutex);
            return 0;
        }
        if (use_edf) {
            // update my deadline before checking
            data->deadline = data->last_compile + data->requirements->time_to_burnout;
            heap_update(first_dongle, data->id, data->deadline);
            if (first_dongle->edf_q[0].id == data->id &&
                get_time() - first_dongle->last_release >= dongle_cooldown)
                break;
        } else {
            if (first_dongle->queue[0] == data->id &&
                get_time() - first_dongle->last_release >= dongle_cooldown)
                break;
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 1000 * 1000;
        if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
        pthread_cond_timedwait(&first_dongle->cond, &first_dongle->mutex, &ts);
    }

    // remove from whichever queue
    if (use_edf){
        heap_pop(first_dongle);  // when you're the winner (you're at root)
    }else
        remove_from_queue(first_dongle);

    log_state(data, "has taken a dongle", YEL);

    if (data->requirements->n_coders == 1){
        pthread_mutex_unlock(&first_dongle->mutex);
        return 0;
    }

    // ── SECOND DONGLE ─────────────────────────────────────────
    pthread_mutex_lock(&second_dongle->mutex);

    if (use_edf) {
        // add to edf queue
        heap_push(second_dongle, data->id, data->deadline);
    } else {
        second_dongle->queue[second_dongle->queue_size] = data->id;
        second_dongle->queue_size++;
    }

    while (1) {
        if (is_burned_out(data->requirements)) {
            if (use_edf) {
                heap_remove_by_id(second_dongle, data->id);  // when leaving due to burnout
                pthread_cond_broadcast(&second_dongle->cond);
            } else {
                remove_from_queue(second_dongle);
                pthread_cond_broadcast(&second_dongle->cond);
            }
            pthread_mutex_unlock(&first_dongle->mutex);
            pthread_mutex_unlock(&second_dongle->mutex);
            return 0;
        }
        if (use_edf) {
            data->deadline = data->last_compile + data->requirements->time_to_burnout;
            heap_update(second_dongle, data->id, data->deadline);
            if (second_dongle->edf_q[0].id == data->id &&
                get_time() - second_dongle->last_release >= dongle_cooldown)
                break;
        } else {
            if (second_dongle->queue[0] == data->id &&
                get_time() - second_dongle->last_release >= dongle_cooldown)
                break;
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += 1000 * 1000;
        if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }
        pthread_cond_timedwait(&second_dongle->cond, &second_dongle->mutex, &ts);
    }

    if (use_edf){
        heap_pop(second_dongle);  // when you're the winner (you're at root)
    }else
        remove_from_queue(second_dongle);

    log_state(data, "has taken a dongle", YEL);
    return 1;
}

void realise_dongle(t_dongle *first_dongle, t_dongle *second_dongle){
    first_dongle->last_release = get_time();
    pthread_cond_broadcast(&first_dongle->cond);
    pthread_mutex_unlock(&first_dongle->mutex);

    second_dongle->last_release = get_time();
    pthread_cond_broadcast(&second_dongle->cond);
    pthread_mutex_unlock(&second_dongle->mutex);
}

void* routine(void *arg)
{
    t_thread *data = (t_thread *)arg;
    int *turns = &data->compile_count;
    int time_to_burnout = data->requirements->time_to_burnout;
    int time_to_compile = data->requirements->time_to_compile;
    int time_to_debug = data->requirements->time_to_debug;
    int time_to_refactor = data->requirements->time_to_refactor;
    int dongle_cooldown = data->requirements->dongle_cooldown;

    while(!is_burned_out(data->requirements) && !is_completed(data->requirements)){
        // add_to_queue(data);
        if (data->id % 2){
            if (!lock_dongle(data->right_dongle, data->left_dongle, data))
                return 0;
        }else{
            if (!lock_dongle(data->left_dongle, data->right_dongle, data))
                return 0;
        }
        
        if(is_burned_out(data->requirements))
        {
             if (data->id % 2)
            realise_dongle(data->left_dongle, data->right_dongle);
        else
            realise_dongle(data->right_dongle, data->left_dongle);
        return (NULL);
        
        }

        pthread_mutex_lock(&data->state_mutex);
        data->last_compile = get_time();
        pthread_mutex_unlock(&data->state_mutex);
        log_state(data, "is compiling", GRN);
        usleep(time_to_compile*1000);
        
        if (data->id % 2)
            realise_dongle(data->left_dongle, data->right_dongle);
        else
            realise_dongle(data->right_dongle, data->left_dongle);

        pthread_mutex_lock(&data->state_mutex);
        data->compile_count++;
        pthread_mutex_unlock(&data->state_mutex);

        if(is_burned_out(data->requirements))
            return (NULL);

        log_state(data, "is debugging", RED);
        usleep(time_to_debug*1000);
        
        if(is_burned_out(data->requirements))
            return (NULL);

        log_state(data, "is refactoring", BLU);
        usleep(time_to_refactor * 1000);
        
        if(is_burned_out(data->requirements))
            return (NULL);
    }
    return NULL;
}

void* monitor(void *arg){
    t_thread *coders = (t_thread *)arg;   // correct cast
    int n = coders[0].requirements->n_coders;
    long time_to_burnout = coders[0].requirements->time_to_burnout;
    long required = coders[0].requirements->number_of_compiles_required; 
    int done;
    long last_compile;
    int count;
    while (1){
        usleep(1000);
        for (int i=0; i < n; i++){
            pthread_mutex_lock(&coders[i].state_mutex);
            last_compile = coders[i].last_compile;
            pthread_mutex_unlock(&coders[i].state_mutex);
            if (get_time() - last_compile > time_to_burnout){
                log_state(&coders[i], "burned out", "");
                set_burned_out(coders[i].requirements);
                // coders[i].requirements->burned_out = 1;
                return NULL;
            }
        }
        done = 1;
        for (int i=0; i < n; i++){
            pthread_mutex_lock(&coders[i].state_mutex);
            count = coders[i].compile_count;
            pthread_mutex_unlock(&coders[i].state_mutex);
            if (count < required)
                done = 0;
        }
        if (done){
            set_completed(coders[0].requirements);
            // coders[0].requirements->completed = 1;
            return NULL;
        }
    }
}


int main(int argc, char* argv[]) {
    if (argc < 9)
    {
        fprintf(stderr, "Usage: %s n burnout compile debug refactor turns cooldown scheduler\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    t_args requirements;
    requirements.n_coders = n;
    requirements.time_to_burnout = atoi(argv[2]);
    requirements.time_to_compile = atoi(argv[3]);
    requirements.time_to_debug = atoi(argv[4]);
    requirements.time_to_refactor = atoi(argv[5]);
    requirements.number_of_compiles_required = atoi(argv[6]);
    requirements.dongle_cooldown = atoi(argv[7]);
    requirements.scheduler = argv[8];
    requirements.burned_out = 0;
    requirements.completed = 0;
    pthread_mutex_init(&requirements.log_mutex, NULL);
    pthread_mutex_init(&requirements.burned_mutex, NULL);
    pthread_t t[n];
    t_thread data[n];
    t_dongle mutex[n];
    pthread_t m;

    for (int i = 0; i < n; i++){
        mutex[i].last_release = get_time() - requirements.dongle_cooldown;
        mutex[i].queue_size = 0;
        mutex[i].edf_size = 0;
        mutex[i].edf_q = malloc(2 * sizeof(t_waiter));
        mutex[i].queue = malloc(2 * sizeof(int));
        pthread_cond_init(&mutex[i].cond, NULL);
        pthread_mutex_init(&mutex[i].mutex, NULL);
    }
    

    for (int i = 0; i < n; i++){
        // printf("thread %d started execution:\n", i);
        pthread_mutex_init(&data[i].state_mutex, NULL);
        data[i].requirements = &requirements;
        data[i].last_compile = get_time();
        data[i].deadline = data[i].last_compile + data[i].requirements->time_to_burnout;
        data[i].id = i + 1;
        data[i].compile_count = 0;
        data[i].left_dongle = &mutex[i];
        data[i].right_dongle = &mutex[(i + 1) % n];
        data[i].start_time = get_time();
        if (pthread_create(&t[i], NULL, &routine, &data[i])) 
            return 1;
    }
    if (pthread_create(&m, NULL, &monitor, &data))
        return 1;
    if (pthread_join(m, NULL)) 
        return 1;

    for (int i = 0; i < n; i++){
        if (pthread_join(t[i], NULL)) {
            return 1;
        }
        // printf("thread %d finished it execution\n", i + 1);
    }
    for (int i = 0; i < n; i++){
        pthread_mutex_destroy(&mutex[i].mutex);
        free(mutex[i].queue);
        free(mutex[i].edf_q);
        pthread_cond_destroy(&mutex[i].cond);
    }
    return 0;
}
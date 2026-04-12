#ifndef CODEXION_H
# define CODEXION_H

# define _POSIX_C_SOURCE 199309L
# define _XOPEN_SOURCE 500
# define _POSIX_C_SOURCE 199309L
# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# include <pthread.h>
# include <time.h>
# include <sys/time.h>
# include <string.h>

# define RED   "\x1b[31m"
# define GRN   "\x1b[32m"
# define YEL   "\x1b[33m"
# define BLU   "\x1b[34m"
# define RESET "\x1b[0m"

/* ═══════════════════════════ STRUCTURES ═══════════════════════════ */

typedef struct s_waiter
{
	int		id;
	long	deadline;
}	t_waiter;

typedef struct s_args
{
	long			time_to_burnout;
	long			time_to_compile;
	long			time_to_debug;
	long			time_to_refactor;
	long			number_of_compiles_required;
	long			dongle_cooldown;
	long			n_coders;
	char			*scheduler;
	int				burned_out;
	int				completed;
	pthread_mutex_t	log_mutex;
	pthread_mutex_t	burned_mutex;
}	t_args;

typedef struct s_dongle
{
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	long			last_release;
	int				queue_size;
	int				*queue;
	t_waiter		*edf_q;
	int				edf_size;
}	t_dongle;

typedef struct s_thread
{
	int				id;
	int				compile_count;
	long			start_time;
	long			last_compile;
	long			deadline;
	pthread_mutex_t	state_mutex;
	t_args			*requirements;
	t_dongle		*right_dongle;
	t_dongle		*left_dongle;
}	t_thread;

/* ═══════════════════════════ TIME UTILS ═══════════════════════════ */

long	get_time(void);

/* ════════════════════════════ LOGGING ═════════════════════════════ */

void	log_state(t_thread *data, char *msg, char *clr);

/* ══════════════════════════ STATE FLAGS ═══════════════════════════ */

int		is_burned_out(t_args *req);
void	set_burned_out(t_args *req);
int		is_completed(t_args *req);
void	set_completed(t_args *req);

/* ══════════════════════════ FIFO QUEUE ════════════════════════════ */

void	remove_from_queue(t_dongle *dongle);

/* ═══════════════════════════ EDF HEAP ═════════════════════════════ */

void		heap_swap(t_waiter *a, t_waiter *b);
void		heap_bubble_up(t_waiter *heap, int index);
void		heap_bubble_down(t_waiter *heap, int size, int index);
void		heap_push(t_dongle *dongle, int id, long deadline);
t_waiter	heap_pop(t_dongle *dongle);
void		heap_update(t_dongle *dongle, int id, long new_deadline);
void		heap_remove_by_id(t_dongle *dongle, int id);

/* ══════════════════════════ DONGLE OPS ════════════════════════════ */

int		lock_dongle(t_dongle *first, t_dongle *second, t_thread *data);
void	realise_dongle(t_dongle *first, t_dongle *second);

/* ════════════════════════════ THREADS ═════════════════════════════ */

void	*routine(void *arg);
void	*monitor(void *arg);

#endif
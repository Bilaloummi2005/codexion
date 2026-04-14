#include "codexion.h"

int	init_dongles(t_dongle *dongles, int n, long cooldown)
{
	int	i;

	i = 0;
	while (i < n)
	{
		dongles[i].last_release = get_time() - cooldown;
		dongles[i].queue_size = 0;
		dongles[i].edf_size = 0;
		dongles[i].edf_q = malloc(2 * sizeof(t_waiter));
		if (!dongles[i].edf_q)
			return (0);
		dongles[i].queue = malloc(2 * sizeof(int));
		if (!dongles[i].queue)
		{
			free(dongles[i].edf_q);
			return (0);
		}
		pthread_cond_init(&dongles[i].cond, NULL);
		pthread_mutex_init(&dongles[i].mutex, NULL);
		i++;
	}
	return (1);
}

void	init_coder(t_thread *coder, int index, t_args *req, t_dongle *dongles)
{
	pthread_mutex_init(&coder->state_mutex, NULL);
	coder->requirements = req;
	coder->last_compile = get_time();
	coder->deadline = coder->last_compile + req->time_to_burnout;
	coder->id = index + 1;
	coder->compile_count = 0;
	coder->left_dongle = &dongles[index];
	coder->right_dongle = &dongles[(index + 1) % req->n_coders];
	coder->start_time = get_time();
}

void	cleanup(t_dongle *dongles, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		pthread_mutex_destroy(&dongles[i].mutex);
		pthread_cond_destroy(&dongles[i].cond);
		free(dongles[i].queue);
		free(dongles[i].edf_q);
		i++;
	}
}

int	parse_args(t_args *req, int argc, char **argv)
{
	if (argc != 9)
	{
		fprintf(stderr, "Usage: %s n burnout compile debug refactor turns cooldown scheduler\n", argv[0]);
		return (0);
	}
	req->n_coders = atoi(argv[1]);
	req->time_to_burnout = atoi(argv[2]);
	req->time_to_compile = atoi(argv[3]);
	req->time_to_debug = atoi(argv[4]);
	req->time_to_refactor = atoi(argv[5]);
	req->number_of_compiles_required = atoi(argv[6]);
	req->dongle_cooldown = atoi(argv[7]);
	req->scheduler = argv[8];
	req->burned_out = 0;
	req->completed = 0;
	pthread_mutex_init(&req->log_mutex, NULL);
	pthread_mutex_init(&req->burned_mutex, NULL);
	return (1);
}

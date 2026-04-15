#include "codexion.h"

static int	run(t_args *req){
	int			i;
	int			n = req->n_coders;
	pthread_t	threads[n];
	t_thread	coders[n];
	t_dongle	dongles[n];
	pthread_t	mon;

	if (!init_dongles(dongles, n, req->dongle_cooldown))
		return (1);
	i = 0;
	while (i < n){
		init_coder(&coders[i], i, req, dongles);
		if (pthread_create(&threads[i], NULL, &routine, &coders[i]))
			return (1);
		i++;
	}
	if (pthread_create(&mon, NULL, &monitor, &coders))
		return (1);
	if (pthread_join(mon, NULL))
		return (1);
	i = 0;
	while (i < n){
		pthread_join(threads[i], NULL);
		i++;
	}
	cleanup(dongles, n);
	return (0);
}

int	main(int argc, char *argv[])
{
	t_args		req;

	if (!parse_args(&req, argc, argv))
		return (1);
	return (run(&req));
}

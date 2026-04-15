#include "codexion.h"

static int	parse_long(const char *str, long *out){
	int	    i;
	int	    len;
	long	result;
    int     long_max_digit;
    char    *long_max_str;

	if (!str || !*str || str[0] == '-'||)
		return (0);
    long_max_str = "9223372036854775807";
    long_max_digit = 19;
	i = 0;
	len = strlen(str);
	if (len == 0 || len > long_max_digit)
		return (0);
	if (len == long_max_digit && strcmp(str, long_max_str) > 0)
		return (0);
	result = 0;
	while (str[i]){
		if (str[i] < '0' || str[i] > '9')
			return (0);
		result = result * 10 + (str[i] - '0');
		i++;
	}
	*out = result;
	return (1);
}

int	parse_args(t_args *req, int argc, char **argv)
{
	if (argc != 9)
	{
		fprintf(stderr, "Usage: %s n burnout compile debug refactor turns cooldown scheduler\n", argv[0]);
		return (0);
	}
	if (!parse_long(argv[1], &req->n_coders)
		|| !parse_long(argv[2], &req->time_to_burnout)
		|| !parse_long(argv[3], &req->time_to_compile)
		|| !parse_long(argv[4], &req->time_to_debug)
		|| !parse_long(argv[5], &req->time_to_refactor)
		|| !parse_long(argv[6], &req->number_of_compiles_required)
		|| !parse_long(argv[7], &req->dongle_cooldown)){
		fprintf(stderr, "Error: all numeric arguments must be valid positive integers\n");
		return (0);
	}
	if (req->n_coders < 1 || req->time_to_burnout <= 0
		|| req->time_to_compile <= 0 || req->time_to_debug <= 0
		|| req->time_to_refactor <= 0 || req->number_of_compiles_required < 1
		|| req->dongle_cooldown < 0){
		fprintf(stderr, "Error: n/burnout/compile/debug/refactor/turns must be > 0, cooldown >= 0\n");
		return (0);
	}
	if (strcmp(argv[8], "edf") != 0 && strcmp(argv[8], "fifo") != 0){
		fprintf(stderr, "Error: scheduler must be 'edf' or 'fifo'\n");
		return (0);
	}
	req->scheduler = argv[8];
	req->burned_out = 0;
	req->completed = 0;
	pthread_mutex_init(&req->log_mutex, NULL);
	pthread_mutex_init(&req->burned_mutex, NULL);
	return (1);
}
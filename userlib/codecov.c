#include "codecov.h"

const static char *cov_file = "/sys/kernel/debug/codecov_entry";
static int cov_fd;

int cov_register(void)
{
	int err;

	cov_fd = open(cov_file, O_RDONLY);
	if (cov_fd == -1)
		return -1;
	/* TODO: for now, we just open the file descriptor */

	return 0;
}

int cov_unregister(void)
{
	close(cov_fd);
	return 0;
}

int checkpoint_add(char *name, char *func, unsigned long offset)
{
	struct checkpoint tmp;

	memset(&tmp, 0, sizeof(tmp));
	tmp.name = name;
	tmp.func = func;
	tmp.offset = offset;
	tmp.name_len = strlen(name);
	tmp.func_len = strlen(func);

	int err = ioctl(cov_fd, COV_ADD_CP, (unsigned long)&tmp);

	return (err == -1) ? -1 : 0;
}

int checkpoint_del(char *name)
{
	struct checkpoint tmp;
	int err;

	memset(&tmp, 0, sizeof(tmp));
	tmp.name = name;
	tmp.name_len = strlen(name);

	err = ioctl(cov_fd, COV_DEL_CP, (unsigned long)&tmp);

	return (err == -1) ? -1 : 0;
}

int get_numhit(unsigned long *num_hit)
{
	return ioctl(cov_fd, COV_COUNT_HIT, (unsigned long)num_hit);
}

int get_numtotal(unsigned long *num_total)
{
	return ioctl(cov_fd, COV_COUNT_CP, (unsigned long)num_total);
}

int get_coverage(double *percent)
{
	int err;
	unsigned long num_hit, num_total;

	err = get_numhit(&num_hit);
	if (err == -1)
		return -1;
	err = get_numtotal(&num_total);
	if (err == -1)
		return -1;

	*percent = (double)num_hit/(double)num_total;
	return 0;
}

int checkpoint_restart(void)
{
	int err = ioctl(cov_fd, COV_RESTART_CP, (unsigned long)0);
	return (err == -1) ? -1 : 0;
}

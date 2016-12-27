#include "codecov.h"

const static char *cov_file = "/sys/kernel/debug/codecov_entry";
static int cov_fd;
static char *log_buffer;

/*
 * cov_register: register current process
 * @id: the sample id for the testing file, 0 if do not care
 * @is_test_case: is this process for test case
 */
int cov_register(unsigned long id, int is_test_case)
{
	int err = 0;
	unsigned long arg_tmp[2];

	log_buffer = malloc(THREAD_BUFFER_SIZE);
	if (!log_buffer)
		return -1;

	cov_fd = open(cov_file, O_RDONLY);
	if (cov_fd == -1) {
		free(log_buffer);
		return -1;
	}

	arg_tmp[0] = id;
	arg_tmp[1] = (unsigned long)is_test_case;
	err = ioctl(cov_fd, COV_REGISTER, (unsigned long)arg_tmp);
	if (err == -1) {
		free(log_buffer);
		close(cov_fd);
		cov_fd = -1;
	}

	return err;
}

int cov_check(void)
{
	return ioctl(cov_fd, COV_CHECK, 0);
}

int cov_unregister(void)
{
	ioctl(cov_fd, COV_UNREGISTER, 0);
	close(cov_fd);
	free(log_buffer);
	return 0;
}

/*
 * checkpoint_add: add a new probe point `func`
 * if return EINVAL, that means
 *	@name too long
 *	@func too long
 *	the register_*probe failed, Documentations/kprobes.txt for more detail
 *	@level: the total level value of the function, different from
 *		get_next_unhit_cp / get_next_unhit_func level
 */
int checkpoint_add(char *name, char *func, unsigned long offset,
		   unsigned long level)
{
	struct checkpoint tmp;

	memset(&tmp, 0, sizeof(tmp));
	tmp.name = name;
	tmp.func = func;
	tmp.offset = offset;
	tmp.name_len = strlen(name);
	tmp.func_len = strlen(func);
	tmp.level = level;

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

/* get kernel log msg buffer, len usually equal THREAD_BUFFER_SIZE */
int cov_get_buffer(char *buffer, size_t len)
{
	struct buffer_user bu;
	bu.buffer = buffer;
	bu.len = len;

	return ioctl(cov_fd, COV_GET_BUFFER, (unsigned long)&bu);
}

int cov_buffer_print(void)
{
	int err = cov_get_buffer(log_buffer, THREAD_BUFFER_SIZE);
	if (err == -1)
		return -1;

	printf("%s\n", log_buffer);
	return 0;
}

int get_cp_status(char *name, int option, unsigned long *value)
{
	unsigned long arg_u[4];
	arg_u[0] = (unsigned long)name;
	arg_u[1] = strlen(name);
	arg_u[2] = option;
	arg_u[3] = (unsigned long)value;

	return ioctl(cov_fd, COV_GET_CP_STATUS, (unsigned long)arg_u);
}

int get_next_unhit_func(char *buf, size_t len, size_t skip, unsigned long level)
{
	unsigned long arg_tmp[4];
	arg_tmp[0] = (unsigned long)buf;
	arg_tmp[1] = len;
	arg_tmp[2] = skip;
	arg_tmp[3] = level;

	return ioctl(cov_fd, COV_NEXT_UNHIT_FUNC, (unsigned long)arg_tmp);
}

int get_next_unhit_cp(char *buf, size_t len, size_t skip, unsigned long level)
{
	unsigned long arg_tmp[4];
	arg_tmp[0] = (unsigned long)buf;
	arg_tmp[1] = len;
	arg_tmp[2] = skip;
	arg_tmp[3] = level;

	return ioctl(cov_fd, COV_NEXT_UNHIT_CP, (unsigned long)arg_tmp);
}

int get_path_map(char *buf, size_t *len)
{
	unsigned long arg_tmp[2];
	arg_tmp[0] = (unsigned long)buf;
	arg_tmp[1] = (unsigned long)len;

	return ioctl(cov_fd, COV_PATH_MAP, (unsigned long)arg_tmp);
}

int cov_path_count(unsigned long *count)
{
	return ioctl(cov_fd, COV_PATH_COUNT, count);
}

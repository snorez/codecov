#ifndef CODECOV_H
#define CODECOV_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/user.h>

#define COV_IOC_MAGIC		'z'
#define COV_COUNT_HIT		_IOWR(COV_IOC_MAGIC, 0, unsigned long)
#define COV_COUNT_CP		_IOWR(COV_IOC_MAGIC, 1, unsigned long)
#define COV_ADD_CP		_IOWR(COV_IOC_MAGIC, 2, unsigned long)
#define COV_DEL_CP		_IOWR(COV_IOC_MAGIC, 3, unsigned long)
#define COV_RESTART_CP		_IOWR(COV_IOC_MAGIC, 4, unsigned long)
#define COV_REGISTER		_IOWR(COV_IOC_MAGIC, 5, unsigned long)
#define COV_UNREGISTER		_IOWR(COV_IOC_MAGIC, 6, unsigned long)
#define COV_GET_BUFFER		_IOWR(COV_IOC_MAGIC, 7, unsigned long)
#define COV_PATH_COUNT		_IOWR(COV_IOC_MAGIC, 8, unsigned long)
#define COV_NEXT_UNHIT_FUNC	_IOWR(COV_IOC_MAGIC, 9, unsigned long)
#define COV_NEXT_UNHIT_CP	_IOWR(COV_IOC_MAGIC, 10, unsigned long)
#define COV_PATH_MAP		_IOWR(COV_IOC_MAGIC, 11, unsigned long)

struct checkpoint {
	size_t name_len;
	char *name;
	size_t func_len;
	char *func;
	unsigned long offset;
};

#define THREAD_BUFFER_SIZE	PAGE_SIZE*8
struct buffer_user {
	char *buffer;
	size_t len;
};

struct path_map_user {
	char *buffer;
	size_t *len;
};

extern int checkpoint_add(char *name, char *func, unsigned long offset);
extern int checkpoint_del(char *name);
extern int get_numhit(unsigned long *num_hit);
extern int get_numtotal(unsigned long *num_total);
extern int get_coverage(double *percent);
extern int checkpoint_restart(void);
extern int cov_register(unsigned long id);
extern int cov_unregister(void);
extern int cov_get_buffer(char *buffer, size_t len);
extern int cov_path_count(unsigned long *count);
extern int get_next_unhit_func(char *buf, size_t len);
extern int get_next_unhit_cp(char *buf, size_t len);
extern int get_path_map(char *buf, size_t *len);

#endif

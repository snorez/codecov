#ifndef CODECOV_H
#define CODECOV_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ioctl.h>

#define COV_IOC_MAGIC		'z'
#define COV_COUNT_HIT		_IOWR(COV_IOC_MAGIC, 0, unsigned long)
#define COV_COUNT_CP		_IOWR(COV_IOC_MAGIC, 1, unsigned long)
#define COV_ADD_CP		_IOWR(COV_IOC_MAGIC, 2, unsigned long)
#define COV_DEL_CP		_IOWR(COV_IOC_MAGIC, 3, unsigned long)
#define COV_RESTART_CP		_IOWR(COV_IOC_MAGIC, 4, unsigned long)

struct checkpoint {
	size_t name_len;
	char *name;
	size_t func_len;
	char *func;
	unsigned long offset;
};

extern int checkpoint_add(char *name, char *func, unsigned long offset);
extern int checkpoint_del(char *name);
extern int get_numhit(unsigned long *num_hit);
extern int get_numtotal(unsigned long *num_total);
extern int get_coverage(double *percent);
extern int checkpoint_restart(void);
extern int cov_register(void);
extern int cov_unregister(void);

#endif

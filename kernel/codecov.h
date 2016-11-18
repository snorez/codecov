#ifndef CODECOV_H
#define CODECOV_H

struct checkpoint;

/* some cmds we need
 * XXX: we should use _IOR _IOW _IOWR macros to generate these cmds
 */
#define COV_IOC_MAGIC		'z'
#define COV_COUNT_HIT		_IOWR(COV_IOC_MAGIC, 0, unsigned long)
#define COV_COUNT_CP		_IOWR(COV_IOC_MAGIC, 1, unsigned long)
#define COV_ADD_CP		_IOWR(COV_IOC_MAGIC, 2, unsigned long)
#define COV_DEL_CP		_IOWR(COV_IOC_MAGIC, 3, unsigned long)
#define COV_RESTART_CP		_IOWR(COV_IOC_MAGIC, 4, unsigned long)

#endif

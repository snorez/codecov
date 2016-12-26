#ifndef CODECOV_H
#define CODECOV_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kprobes.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include "./cov_thread.h"
#include "./checkpoint.h"
#include "./thread_buffer.h"

/* some cmds we need
 * XXX: we should use _IOR _IOW _IOWR macros to generate these cmds
 */
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
#define COV_CHECK		_IOWR(COV_IOC_MAGIC, 12, unsigned long)

#endif

#ifndef COVERR_H
#define COVERR_H

#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>

extern struct list_head coverr_root;

struct coverr {
	struct list_head siblings;	/* for multiple threads */
	struct task_struct *task;	/* be careful here if process terminated */
	char *buffer;
};

extern void coverr_init(void);
extern int coverr_add(struct task_struct *task);
extern void coverr_del(struct task_struct *task);
extern void coverr_exit(void);

#endif

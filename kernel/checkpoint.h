#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/types.h>
#include <linux/rwlock.h>
#include <linux/kallsyms.h>

#define NAME_LEN_MAX	KSYM_NAME_LEN
#define FUNC_LEN_MAX	KSYM_NAME_LEN
#define RETPROBE_MAXACTIVE	16


struct checkpoint {
	struct list_head siblings;
	struct list_head caller;	/* root of who called this probed point */

	unsigned long hit;		/* numbers been hit */
	char *name;			/* checkpoint's specific name */
	struct kprobe *this_kprobe;	/* used inside a function */
	struct kretprobe *this_retprobe;/* against this_kprobe, used on func */
};
extern struct list_head cproot;
extern rwlock_t cproot_rwlock;

struct checkpoint_caller {
	struct list_head caller_list;
	char name[KSYM_NAME_LEN];	/* caller function name */
	unsigned long address;		/* specific */
	unsigned long sample_id;	/* identify sample trigger this first */
};

struct checkpoint_user {
	size_t name_len;
	char __user *name;
	size_t func_len;
	char __user *func;
	unsigned long offset;
};

extern int cp_default_kp_prehdl(struct kprobe *kp, struct pt_regs *reg);
extern int cp_default_ret_hdl(struct kretprobe_instance *ri,
			      struct pt_regs *regs);
extern int cp_default_ret_entryhdl(struct kretprobe_instance *ri,
				   struct pt_regs *regs);

extern void checkpoint_init(void);
extern int checkpoint_add(char *name, char *func, unsigned long offset);
extern void checkpoint_del(char *name);
extern void checkpoint_restart(void);
extern void checkpoint_exit(void);
extern unsigned long checkpoint_get_numhit(void);
extern unsigned long checkpoint_count(void);
unsigned long path_count(void);

#endif

#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/types.h>
#include <linux/rwlock.h>
#include <linux/kallsyms.h>
#include <asm/uaccess.h>
#include <asm/insn.h>
#include <linux/atomic.h>
#include "./cov_thread.h"
#include "./config.h"

#define NAME_LEN_MAX	KSYM_NAME_LEN
#define FUNC_LEN_MAX	KSYM_NAME_LEN
#define RETPROBE_MAXACTIVE	16

enum jxx_1 {
	JO_OPC0		= 0x70,	//Jbs
	JNO_OPC0,		//Jbs
	JB_OPC0,		//Jbs
	JNB_OPC0,		//Jbs
	JE_OPC0,		//Jbs
	JNE_OPC0,		//Jbs
	JBE_OPC0,		//Jbs
	JA_OPC0,		//Jbs
	JS_OPC0,		//Jbs
	JNS_OPC0,		//Jbs
	JPE_OPC0,		//Jbs
	JPO_OPC0,		//Jbs
	JL_OPC0,		//Jbs
	JGE_OPC0,		//Jbs
	JLE_OPC0,		//Jbs
	JG_OPC0		= 0x7f,	//Jbs
	JCXZ_OPC	= 0xe3,	//Jbs jump if ecx/rcx = 0
	//JMP_OPC	= 0xe9,	//Jvds
	//JMPS_OPC	= 0xeb,	//Jvds
	//JMPF_OPC	= 0xea, //invalid in 64-bit mode
};

enum jxx_2 {
	TWO_OPC		= 0x0f,
	JO_OPC1		= 0x80,	//Jvds
	JNO_OPC1,		//Jvds
	JB_OPC1,		//Jvds
	JNB_OPC1,		//Jvds
	JE_OPC1,		//Jvds
	JNE_OPC1,		//Jvds
	JBE_OPC1,		//Jvds
	JA_OPC1,		//Jvds
	JS_OPC1,		//Jvds
	JNS_OPC1,		//Jvds
	JPE_OPC1,		//Jvds
	JPO_OPC1,		//Jvds
	JL_OPC1,		//Jvds
	JGE_OPC1,		//Jvds
	JLE_OPC1,		//Jvds
	JG_OPC1		= 0x8f,	//Jvds
};

#define CP_CALLER_MAX	64
struct checkpoint_caller {
	atomic_t in_use;		/* use test_and_set_bit set this value */
	char name[KSYM_NAME_LEN];	/* caller function name */
	atomic64_t address;		/* specific */
	atomic64_t sample_id;		/* identify sample trigger this first */
};

struct checkpoint {
	struct list_head siblings;
	struct checkpoint_caller caller[CP_CALLER_MAX];
					/* root of who called this probed point */

	char *name;			/* checkpoint's specific name */
	unsigned long level;
	atomic_t hit;			/* numbers been hit */
	atomic_t enabled;
	struct kprobe *this_kprobe;	/* used inside a function */
	struct kretprobe *this_retprobe;/* against this_kprobe, used on func */
};
extern struct list_head cproot;

struct checkpoint_user {
	size_t name_len;
	char __user *name;
	size_t func_len;
	char __user *func;
	unsigned long offset;
	unsigned long level;
};

enum status_opt { STATUS_HIT, STATUS_LEVEL, STATUS_ENABLED, };

extern int cp_default_kp_prehdl(struct kprobe *kp, struct pt_regs *reg);
extern int cp_default_ret_hdl(struct kretprobe_instance *ri,
			      struct pt_regs *regs);
extern int cp_default_ret_entryhdl(struct kretprobe_instance *ri,
				   struct pt_regs *regs);

extern void checkpoint_init(void);
extern int checkpoint_add(char *name, char *func, unsigned long offset,
			  unsigned long level);
extern int checkpoint_xstate(unsigned long name, unsigned long len,
			     unsigned long enable, unsigned long subpath);
extern void checkpoint_del(char *name);
extern void checkpoint_restart(void);
extern void checkpoint_exit(void);
extern unsigned long checkpoint_get_numhit(void);
extern unsigned long checkpoint_count(void);
unsigned long path_count(void);
extern unsigned long get_cp_status(char *name, enum status_opt option);
extern int get_next_unhit_func(char __user *buf, size_t len, size_t skip,
			       unsigned long level);
extern int get_next_unhit_cp(char __user *buf, size_t len, size_t skip,
			     unsigned long level);
extern int get_path_map(char __user *buf, size_t __user *len);

#endif

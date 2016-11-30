#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include "checkpoint.h"
#include "cov_thread.h"
#include "./thread_buffer.h"

/*
 * these probe handler should do this, increase the `hit` field of the
 * checkpoint structure
 */

static int address_in_caller(struct checkpoint *cp, unsigned long address)
{
	struct checkpoint_caller *tmp;

	list_for_each_entry(tmp, &cp->caller, caller_list) {
		if (address == tmp->address)
			return 1;
	}
	return 0;
}

/*
 * add the @address to cp->caller list, the cproot_rwlock has already been held
 * check if the address is already in the list, if not
 * alloc a new structure checkpoint_caller
 * search the task_list_root to get/set the sample_id,
 * lookup_symbol_name to set the function name
 */
static int checkpoint_caller_add(struct checkpoint *cp, unsigned long address)
{
	/*
	 * TODO:
	 * check if the address already exists in cp->caller
	 * add a structure checkpoint_caller
	 */
	struct checkpoint_caller *new;
	struct task_struct *task = current;
	struct cov_thread *ct = NULL, *tmp;
	int (*get_symbol_via_address)(unsigned long, char *);
	int err;

	if (address_in_caller(cp, address))
		return -1;

	new = kmalloc(sizeof(*new), GFP_KERNEL);
	if (unlikely(!new))
		return -2;
	memset(new, 0, sizeof(*new));

	read_lock(&task_list_rwlock);
	list_for_each_entry(tmp, &task_list_root, list) {
		if (tmp->task == task) {
			ct = tmp;
			break;
		}
	}
	if (unlikely(!ct)) {
		/* may never happen */
		read_unlock(&task_list_rwlock);
		kfree(new);
		return -2;
	} else
		new->sample_id = ct->sample_id;
	read_unlock(&task_list_rwlock);

	get_symbol_via_address =
			(void *)kallsyms_lookup_name("lookup_symbol_name");
	err = get_symbol_via_address(address, new->name);
	if (unlikely(err))
		memset(new->name, 0, KSYM_NAME_LEN);
	new->address = address;
	list_add_tail(&new->caller_list, &cp->caller);
	return 0;
}

/*
 * TODO
 * for point in functions. do not care now
 */
int cp_default_kp_prehdl(struct kprobe *kp, struct pt_regs *reg)
{
	struct checkpoint *tmp;

	if (!task_in_list(current))
		return 0;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_kprobe == kp) {
			if (unlikely(!tmp->hit))
				ctbuf_print("NEW PATH: %s\n", tmp->name);
			tmp->hit++;
			if (unlikely(!tmp->hit))
				tmp->hit = 1;
			break;
		}
	}
	read_unlock(&cproot_rwlock);

	return 0;
}

/*
 * TODO:
 * things we do when the function returns
 */
int cp_default_ret_hdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	if (!task_in_list(current))
		return 0;
	return 0;
}

/*
 * TODO
 * things we do when the function get called
 *
 * if current process has not registered, then return
 * if this probe point have not been called yet, then this is a NEW PATH
 * we inc the `hit` field and log the caller address,
 * if this probe hit several times, then we just check the caller address
 * and add it to cp->caller list
 */
int cp_default_ret_entryhdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	/* before the function called. XXX:should always return 0 */
	struct checkpoint *tmp;
	int err;

	if (!task_in_list(current))
		return 0;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_retprobe == ri->rp) {
			tmp->hit++;
			if (unlikely(!tmp->hit))
				tmp->hit = 1;

			/*
			 * XXX: note that ri->ret_addr not the current caller addr.
			 * so we should get the value [sp]
			 */
			err = checkpoint_caller_add(tmp,
						    *(unsigned long *)regs->sp);
			if (unlikely(err == -2))
				ctbuf_print("ERR: checkpoint_caller_add: %s: %p\n",
					    tmp->name,
					    ri->ret_addr);
			else if (!err)
				ctbuf_print("NEW PATH: %s. CALLER_ADDR: %p\n",
					    tmp->name,
					    *(unsigned long *)regs->sp);
			break;
		}
	}
	read_unlock(&cproot_rwlock);

	return 0;
}

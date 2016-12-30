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
	int err = 0;

	read_lock(&cp->caller_rwlock);
	list_for_each_entry(tmp, &cp->caller, caller_list) {
		if (address == tmp->address) {
			err = 1;
			break;
		}
	}
	read_unlock(&cp->caller_rwlock);
	return err;
}

/*
 * add the @address to cp->caller list, the cproot_rwlock has already been held
 * check if the address is already in the list, if not
 * alloc a new structure checkpoint_caller
 * search the task_list_root to get/set the sample_id,
 * lookup_symbol_name to set the function name
 * if @address == 0, then we take the value of current_thread.prev_addr
 *
 * XXX: get a BUG here, when called by cp_default_kp_prehdl, the kernel stuck
 */
static int checkpoint_caller_add(struct checkpoint *cp, unsigned long address)
{
	/*
	 * check if the address already exists in cp->caller
	 * add a structure checkpoint_caller
	 */
	struct checkpoint_caller *new;
	struct task_struct *task = current;
	struct cov_thread *ct = NULL, *tmp;
	int (*get_symbol_via_address)(unsigned long, char *);
	int err;
	unsigned long id;
	int no_name = 0;

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
		return -2;
	} else {
		id = ct->sample_id;
		if (!address) {
			address = ct->prev_addr;
			no_name = 1;
		}
	}
	read_unlock(&task_list_rwlock);

	if (address_in_caller(cp, address))
		return -1;

	/* XXX: why kmalloc would get stuck? */
	new = kzalloc(sizeof(*new), GFP_ATOMIC);
	if (unlikely(!new))
		return -2;
	new->sample_id = id;

	if (!no_name) {
		get_symbol_via_address =
				(void *)kallsyms_lookup_name("lookup_symbol_name");
		err = get_symbol_via_address(address, new->name);
		if (unlikely(err))
			memset(new->name, 0, KSYM_NAME_LEN);
	}
	new->address = address;

	write_lock(&cp->caller_rwlock);
	list_add_tail(&new->caller_list, &cp->caller);
	write_unlock(&cp->caller_rwlock);

	return 0;
}

/*
 * for checkpoints in functions.
 */
int cp_default_kp_prehdl(struct kprobe *kp, struct pt_regs *reg)
{
	struct checkpoint *tmp;
	struct cov_thread *ct;
	int err;
	int effective = 0;

	if (!task_is_test_case(current))
		return 0;

	write_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_kprobe == kp) {
			tmp->hit++;
			if (unlikely(!tmp->hit))
				tmp->hit = 1;

			ctbuf_print("--------> %s\n", tmp->name);

			err = checkpoint_caller_add(tmp, 0);
			if (unlikely(err == -2))
				ctbuf_print("ERR: checkpoint_caller_add: %s\n",
					    tmp->name);
			else if (!err) {
				ctbuf_print("NEW PATH: %s\n", tmp->name);
				effective = 1;
			}
			break;
		}
	}
	write_unlock(&cproot_rwlock);

	write_lock(&task_list_rwlock);
	list_for_each_entry(ct, &task_list_root, list) {
		if (ct->task == current) {
			ct->prev_addr = (unsigned long)kp->addr;
			if (effective)
				ct->is_sample_effective = 1;
			break;
		}
	}
	write_unlock(&task_list_rwlock);

	return 0;
}

/*
 * things we do when the function returns
 */
int cp_default_ret_hdl(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct cov_thread *ct;
	struct checkpoint *tmp;
	if (!task_is_test_case(current))
		return 0;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (ri->rp == tmp->this_retprobe) {
			ctbuf_print("<<<<<<<<< %s\n", tmp->name);
			break;
		}
	}
	read_unlock(&cproot_rwlock);

	write_lock(&task_list_rwlock);
	list_for_each_entry(ct, &task_list_root, list) {
		if (ct->task == current) {
			ct->prev_addr = (unsigned long)ri->ret_addr;
			break;
		}
	}
	write_unlock(&task_list_rwlock);

	return 0;
}

/*
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
	struct cov_thread *ct;
	int err;
	int effective = 0;

	if (!task_is_test_case(current))
		return 0;

	write_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->this_retprobe == ri->rp) {
			tmp->hit++;
			if (unlikely(!tmp->hit))
				tmp->hit = 1;

			ctbuf_print(">>>>>>>>> %s\n", tmp->name);
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
			else if (!err) {
				ctbuf_print("NEW PATH: %s. CALLER_ADDR: %p\n",
					    tmp->name,
					    *(unsigned long *)regs->sp);
				effective = 1;
			}
			break;
		}
	}
	write_unlock(&cproot_rwlock);

	write_lock(&task_list_rwlock);
	list_for_each_entry(ct, &task_list_root, list) {
		if (ct->task == current) {
			ct->prev_addr = regs->ip;
			if (effective)
				ct->is_sample_effective = 1;
			break;
		}
	}
	write_unlock(&task_list_rwlock);

	return 0;
}

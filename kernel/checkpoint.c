#include "checkpoint.h"

struct list_head cproot;	/* all checkpoint we have probed */
rwlock_t cproot_rwlock;

void checkpoint_init(void)
{
	INIT_LIST_HEAD(&cproot);
	rwlock_init(&cproot_rwlock);
}

static int name_in_cplist(char *name)
{
	struct checkpoint *tmp;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if ((strlen(name) == strlen(tmp->name)) &&
			!strcmp(tmp->name, name)) {
			read_unlock(&cproot_rwlock);
			return 1;
		}
	}
	read_unlock(&cproot_rwlock);

	return 0;
}

/*
 * alloc a new `checkpoint` and initialised with the given arguments
 * @name: the name of this checkpoint, if use offset, please name it [name]_offxxxx
 * @function: the name of the function will be probed
 * @offset: the offset of the function will be probed
 *
 * we should check if the name is already exist in the cproot;
 */
int checkpoint_add(char *name, char *func, unsigned long offset)
{
	int err;
	struct checkpoint *new;
	size_t name_len;

	if (name_in_cplist(name))
		return -EEXIST;

	new = kmalloc(sizeof(struct checkpoint), GFP_KERNEL);
	if (unlikely(!new))
		return -ENOMEM;
	memset(new, 0, sizeof(struct checkpoint));

	err = -EINVAL;
	name_len = strlen(name);
	if (unlikely(name_len > NAME_LEN_MAX))
		goto err_free;
	err = -ENOMEM;
	new->name = kmalloc(name_len+1, GFP_KERNEL);
	if (unlikely(!new->name))
		goto err_free;
	memset(new->name, 0, name_len+1);
	memcpy(new->name, name, name_len);

	/*
	 * now deal with the probe.
	 * If the offset not zero, we alloc a kprobe,
	 * otherwise, we alloc a kretprobe
	 */
	if (unlikely(offset)) {
		new->this_kprobe = kmalloc(sizeof(struct kprobe), GFP_KERNEL);
		if (unlikely(!new->this_kprobe))
			goto err_free2;
		memset(new->this_kprobe, 0, sizeof(struct kprobe));

		new->this_kprobe->pre_handler = cp_default_kp_prehdl;
		new->this_kprobe->symbol_name = func;
		new->this_kprobe->offset = offset;

		if ((err = register_kprobe(new->this_kprobe)))
			goto err_free3;
	} else {
		new->this_retprobe = kmalloc(sizeof(struct kretprobe), GFP_KERNEL);
		if (unlikely(!new->this_retprobe))
			goto err_free2;
		memset(new->this_retprobe, 0, sizeof(struct kretprobe));

		new->this_retprobe->entry_handler = cp_default_ret_entryhdl;
		new->this_retprobe->maxactive = RETPROBE_MAXACTIVE;
		new->this_retprobe->handler = cp_default_ret_hdl;
		new->this_retprobe->kp.addr =
				(kprobe_opcode_t *)kallsyms_lookup_name(func);

		if ((err = register_kretprobe(new->this_retprobe)))
			goto err_free3;
	}

	write_lock(&cproot_rwlock);
	list_add_tail(&new->siblings, &cproot);
	INIT_LIST_HEAD(&new->caller);
	write_unlock(&cproot_rwlock);
	return 0;

err_free3:
	kfree(new->this_retprobe);
	kfree(new->this_kprobe);
err_free2:
	kfree(new->name);
err_free:
	kfree(new);
	return err;
}

static void checkpoint_caller_cleanup(struct checkpoint *cp)
{
	struct checkpoint_caller *tmp, *next;

	list_for_each_entry_safe(tmp, next, &cp->caller, caller_list) {
		list_del(&tmp->caller_list);
		kfree(tmp->name);
	}
	INIT_LIST_HEAD(&cp->caller);
}

void checkpoint_del(char *name)
{
	struct checkpoint *tmp;

	/*
	 * XXX:here we do not need list_for_each_entry_safe,
	 * because when we find the target checkpoint, we jump out the loop
	 */
	write_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if ((strlen(name) == strlen(tmp->name)) &&
			!strcmp(tmp->name, name)) {
			list_del(&tmp->siblings);
			checkpoint_caller_cleanup(tmp);

			if (unlikely(tmp->this_kprobe)) {
				unregister_kprobe(tmp->this_kprobe);
				kfree(tmp->this_kprobe);
			} else {
				unregister_kretprobe(tmp->this_retprobe);
				kfree(tmp->this_retprobe);
			}
			kfree(tmp->name);
			kfree(tmp);
			break;
		}
	}
	write_unlock(&cproot_rwlock);
}

void checkpoint_restart(void)
{
	struct checkpoint *tmp, *next;

	/* TODO: need to check cphit_root */

	write_lock(&cproot_rwlock);
	list_for_each_entry_safe(tmp, next, &cproot, siblings) {
		list_del(&tmp->siblings);
		checkpoint_caller_cleanup(tmp);
		if (unlikely(tmp->this_kprobe)) {
			unregister_kprobe(tmp->this_kprobe);
			kfree(tmp->this_kprobe);
		} else {
			unregister_kretprobe(tmp->this_retprobe);
			kfree(tmp->this_retprobe);
		}
		kfree(tmp->name);
		kfree(tmp);
	}
	INIT_LIST_HEAD(&cproot);
	write_unlock(&cproot_rwlock);
}

/* this function actually returns the number of functions been hit */
unsigned long checkpoint_get_numhit(void)
{
	unsigned long num = 0;
	struct checkpoint *tmp;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		if (tmp->hit)
			num++;
	}
	read_unlock(&cproot_rwlock);

	return num;
}

unsigned long checkpoint_count(void)
{
	unsigned long num = 0;
	struct checkpoint *tmp;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings)
		num++;
	read_unlock(&cproot_rwlock);

	return num;
}

unsigned long path_count(void)
{
	unsigned long num = 0;
	struct checkpoint *tmp;
	struct checkpoint_caller *tmp_caller;

	read_lock(&cproot_rwlock);
	list_for_each_entry(tmp, &cproot, siblings) {
		list_for_each_entry(tmp_caller, &tmp->caller, caller_list)
			num++;
	}
	read_unlock(&cproot_rwlock);

	return num;
}

void checkpoint_exit(void)
{
	checkpoint_restart();
}

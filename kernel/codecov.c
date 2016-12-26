#include "./codecov.h"

/*
 * this file do these things:
 *	* provide filesystem interfaces, could use ioctl
 *	* some initialize things
 */

static long cov_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned long res;

	switch (cmd) {
	case COV_COUNT_HIT:
		res = checkpoint_get_numhit();
		if (copy_to_user((unsigned long __user *)arg, &res, sizeof(arg)))
			return -EFAULT;
		return 0;

	case COV_COUNT_CP:
		res = checkpoint_count();
		if (copy_to_user((unsigned long __user *)arg, &res, sizeof(arg)))
			return -EFAULT;
		return 0;

	case COV_ADD_CP: {
		struct checkpoint_user new;
		char *name, *func;

		if (copy_from_user(&new, (unsigned long __user *)arg, sizeof(new)))
			return -EFAULT;
		if ((new.name_len > NAME_LEN_MAX) || (new.func_len > FUNC_LEN_MAX))
			return -EINVAL;

		name = kmalloc(new.name_len+1, GFP_KERNEL);
		if (unlikely(!name))
			return -ENOMEM;
		memset(name, 0, new.name_len+1);
		if (copy_from_user(name, (unsigned long __user *)new.name,
				   new.name_len)) {
			kfree(name);
			return -EFAULT;
		}

		func = kmalloc(new.func_len+1, GFP_KERNEL);
		if (unlikely(!func)) {
			kfree(name);
			return -ENOMEM;
		}
		memset(func, 0, new.func_len+1);
		if (copy_from_user(func, (unsigned long __user *)new.func,
				   new.func_len)) {
			kfree(name);
			kfree(func);
			return -EFAULT;
		}

		res = checkpoint_add(name, func, new.offset, new.level);
		kfree(name);
		kfree(func);
		return res;
	}

	case COV_DEL_CP: {
		struct checkpoint_user new;
		char *name;

		if (copy_from_user(&new, (unsigned long __user *)arg, sizeof(new)))
			return -EFAULT;
		if (new.name_len > NAME_LEN_MAX)
			return -EINVAL;

		name = kmalloc(new.name_len+1, GFP_KERNEL);
		if (unlikely(!name))
			return -ENOMEM;
		memset(name, 0, new.name_len+1);
		if (copy_from_user(name, (unsigned long __user *)new.name,
				   new.name_len)) {
			kfree(name);
			return -EFAULT;
		}

		checkpoint_del(name);
		kfree(name);
		return 0;
	}

	case COV_RESTART_CP:
		checkpoint_restart();
		return 0;

	case COV_REGISTER: {
		unsigned long arg_u[2];
		if (copy_from_user(arg_u, (void __user *)arg, sizeof(arg_u)))
			return -EFAULT;
		return cov_thread_add(arg_u[0], (int)arg_u[1]);
	}

	case COV_UNREGISTER:
		cov_thread_del();
		return 0;

	case COV_CHECK:
		cov_thread_check();
		return 0;

	case COV_GET_BUFFER: {
		struct buffer_user bu;
		if (copy_from_user(&bu, (void __user *)arg, sizeof(bu)))
			return -EFAULT;
		return ctbuf_get(bu.buffer, bu.len);
	}

	case COV_PATH_COUNT:
		res = path_count();
		if (copy_to_user((void __user *)arg, &res, sizeof(arg)))
			return -EFAULT;
		return 0;

	case COV_NEXT_UNHIT_FUNC: {
		unsigned long arg_u[4];
		if (copy_from_user(arg_u, (void __user *)arg, sizeof(arg_u)))
			return -EFAULT;
		return get_next_unhit_func((char __user *)arg_u[0],
					   arg_u[1], arg_u[2], arg_u[3]);
	}

	case COV_NEXT_UNHIT_CP: {
		unsigned long arg_u[4];
		if (copy_from_user(arg_u, (void __user *)arg, sizeof(arg_u)))
			return -EFAULT;
		return get_next_unhit_cp((char __user *)arg_u[0],
					 arg_u[1], arg_u[2], arg_u[3]);
	}

	case COV_PATH_MAP: {
		unsigned long arg_u[2];
		if (copy_from_user(arg_u, (void __user *)arg, sizeof(arg_u)))
			return -EFAULT;
		return get_path_map((char __user *)arg_u[0],
				    (size_t __user *)arg_u[1]);
	}

	default:
		return -ENOTSUPP;
	}
	return 0;
}

static struct dentry *cov_entry;
static struct file_operations cov_ops = {
	.owner = THIS_MODULE,
	/* XXX: since when the ioctl changed to unlocked_ioctl */
	.unlocked_ioctl = cov_ioctl,
};

static int __init codecov_init(void)
{
	/* TODO */
	cov_entry = debugfs_create_file("codecov_entry", S_IRWXU | S_IROTH, NULL,
					NULL, &cov_ops);
	if (unlikely(!cov_entry))
		return -1;

	cov_thread_init();
	checkpoint_init();

	return 0;
}

static void __exit codecov_exit(void)
{
	/* TODO */
	debugfs_remove(cov_entry);
	cov_thread_exit();
	checkpoint_exit();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zerons");
module_init(codecov_init);
module_exit(codecov_exit);

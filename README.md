# Base infomation
this tool is used to test the code coverage in current running kernel. It need
+ kernel kprobe/kretprobe (Documentation/kprobes.txt)
+ debugfs

we don't use kjprobe, because there are so many functions we might want to probe.
If it is a function(offset should be zero), we use kretprobe(entry_handler, ret_handler). If it has an offset, we use kprobe

As the CONFIG_KCOV is supported from kernel 4.6
[commit](https://github.com/torvalds/linux/commit/5c9a8750a6409c63a0f01d51a9024861022f6593)
maybe we could also use this to do these things.
CONFIG_KASAN/CONFIG_KTSAN..

It seems that what we do now and syzkaller are similar.

we provide two global lists, the functions list we probed, and the threads that
registered. If the probed functions triggered, but current thread not registered,
then we do nothing.

# Usage
+ ### kernel module
	+ modify your /etc/fstab, add this line
		debugfs	/sys/kernel/debug	debugfs	mode=0755,noauto	0	0
	+ insmod your target kernel module and this kernel module
+ ### user library
	+ write a program to add checkpoints
	the program(each [child] tests and checkpint_add process) should do
		```c
		cov_register(sample_id);
		...
		cov_unregister();
		```
	+ run your tests, and check coverage
	+ `cov_unregister` doesn't unregister all probed points, rmmod does
	`cov_register` register current process
	then we should use `checkpoint_add`, and then we could exit the current
	process (which is preferred) or do the tests.

# Notice
+ you can't probe a static function yet, cause the symbol couldn't be found

# things should be done
+ add/del checkpoint(path_node)
+ show current code coverage
+ new path notify
+ till code coverage gets to 100%
+ usespace support libs
+ multiple thread support
+ also mutiple thread err message
+ new path should also be something like(A->B C->D, then A->C B->D)
+ some error message should also be provide to user space
+ subpath in functions. only support x86_64

# things doesn't do yet
+ output the path map
+ ...

# things can't not be done
+ kprobes docs show that some functions couldn't be probed, so maybe we should
	check the error code

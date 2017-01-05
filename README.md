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
+ it seems that when a program calls `fork()`, the kernel module would take high
	cpu usage, and the system doesn't reponse
	+ maybe we should disable all the probes, and then enable the target
		function/s, then rerun the test.
		the `checkpoint_xstate` is added to fix this

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
+ subpath in functions. ***only support x86_64***

# things doesn't do yet
+ output the path map
+ there is a bug that could take a high cpu usage and system doesn't response
	I tried to make the handler quicker and shorter, if this still doesn't work
	I will try to use workqueue to see if that could work.
+ ...

# things can't not be done
+ kprobes docs show that some functions couldn't be probed, so maybe we should
	check the error code

# 中文介绍
### 简介
+ 覆盖率检测工具, 用于内核模块的覆盖率检测.
+ 使用kprobes实现.
+ 通过debugfs来实现用户接口.

### 功能(目前所支持的)
+ 添加观测点, 只需指定需要观测的函数即可, x86_64下可自动添加函数内部分支
+ 启用/禁用观测点
+ 调用者记录
+ 路径统计
+ 观测点状态查询, 包括是否触发, 层级, 启用/禁用状态
+ 查询下一个未触发的函数/观测点, 支持跳过, 支持指定等级的查询
+ 获取调用路径图(所有观测点)
+ 过滤不相关进程(以进程为单位)
+ 支持有效进程检测(样本是否触发新路径)
+ 进程修复(未正常退出的进程会造成内核空间有所浪费)
+ 支持某次系统调用的路径输出

### 注意事项
+ 由于使用读写锁会带来一个难以检测的bug, (在kprobe/kretprobe的注册处理函数
	中会用到读写锁), 所以在注册处理函数中避免使用锁, 但是可能产生一些
	出错的情况或者一些多进程情况下的bug(暂时未发现, 请按规则使用)
+ 请用单个进程来添加观测点, 直至使用完成. 期间尽量避免删除观测点.
+ 使用了原子变量来进行一些操作, 对调用者记录以及进程注册都使用了数组
	+ 同步原理:
		每个数组元素中包含一个`atomic_t`成员, 使用
		test_and_set_bit来申请使用这个元素, 使用
		atomic_read来查询这个元素是否被使用. 使用
		test_and_clear_bit来释放对这个元素的使用权.
+ 样本请尽量不要使用多线程, 可能造成一些记录出错. (主要是调用者记录)
+ 调用记录最多64个(CP_CALLER_MAX), 同时在线的进程64个(COV_THREAD_MAX)

### 函数介绍
	主要就是两个结构, 一个是观测点的数据结构, 一个是注册进程的数据结构
	进行较的操作是遍历观测点, 遍历注册的进程, 遍历调用记录
+ `cov_ioctl`, 用户层系统调用接口
	+ 添加接口注意使用相关宏来处理ioctl接口的cmd参数
+ `do_set_jxx_probe`, 对所有的跳转指令(指令下一条指令和跳转到的地址)
	添加观测点, 主要根据intel手册的指令规范进行处理. 同时内核提供
	了指令分析函数(`insn_init`/`insn_get_length`)
+ `do_auto_add`, 对指定函数进行内部跳转分支自动添加观测点处理.
	此函数和`do_set_jxx_probe`均为架构相关代码.
	***移植可考虑对这个函数进行一些预处理操作.***
+ `do_checkpoint_add`, 添加观测点的主要函数
+ `checkpoint_add`, 添加观测点的接口函数, 如果函数添加成功, 即调用
	`do_auto_add`进行自动添加操作.
+ `checkpoint_xstate`, 切换观测点状态(启用/禁用), 可指定
	是否同时禁用/启用函数内部分支
+ `checkpoint_del`/`checkpoint_restart`, 删除观测点/清空观测点, 暂未使用
+ `checkpoint_get_numhit`, 获取所有被触发的观测点数目
+ `checkpoint_count`, 获取观测点的总数
+ `path_count`, 获取触发的路径总数. 注意
	***覆盖率计算使用的是触发的观测点数目除观测点总数***
+ `get_cp_status`, 获取观测点状态
+ `get_next_unhit_func`/`get_next_unhit_cp`, 查询下一个未被触发的函数
+ `get_path_map`, 使用较少, 只是将内核中的数据结构以一定格式传到用户空间
+ `task_is_test_case`, 避免一些不相关进程的干扰
+ `cov_thread_add`, 添加注册进程, 使用get_task_struct来增加进程结构的引用
+ `cov_thread_del`, 删除注册进程, 对应使用put_task_struct
+ `cov_thread_check`, 对cov_thread数组进行遍历修复(使用需特别注意), 最好
	保证当前只有一个进程, 并且没有其他进程要注册
+ `cov_thread_effective`, 进程是否触发了新路径
+ `probe.c`中为几个kprobes注册处理函数. 其中需要注意的上面提到的, 不要
	使用锁, 会出现难以检测的bug.
	其中一个为kprobe的注册函数, 一个为函数返回时的处理函数, 一个为
	函数入口的处理函数. 处理逻辑类似
	检测指令前的调用者是谁, 然后进行调用记录(`checkpoint_caller_add`)
+ `thread_buffer.c`中为每个进程的信息处理.

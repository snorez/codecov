CONFIG_MODULE_SIG=n

# here, we can not make a object name same with the kernel module name
# otherwise, some strange errs would happen when `insmod`
# module license `unspecific` taint kernel # even we use MODULE_LICENSE
# unknown symbol xxx
obj-m += code_cov.o
code_cov-objs := codecov.o checkpoint.o probe.o cov_thread.o thread_buffer.o
KVER := `uname -r`
KDIR = /lib/modules/$(KVER)/build

all:
	make -C $(KDIR) M=`pwd` modules
clean:
	make -C $(KDIR) M=`pwd` clean

#include "../userlib/codecov.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

char *func_to_probe[] = {
	"ovl_is_opaquedir",
	"ovl_dentry_weak_revalidate",
	"ovl_remount",
	"ovl_mount",
	"ovl_fill_super",
	"ovl_mount_v1",
	"ovl_v1_fill_super",
	"ovl_put_super",
	"ovl_d_real",
	"ovl_dentry_release",
	"ovl_dentry_revalidate",
	"ovl_mount_dir_noesc",
	"ovl_mount_dir",
	"ovl_lower_dir",
	"ovl_workdir_create",
	"ovl_show_options",
	"ovl_statfs",
	"ovl_exit",
	"ovl_getattr",
	"ovl_copy_up_truncate",
	"ovl_put_link",
	"ovl_readlink",
	"ovl_follow_link",
	"ovl_dir_getattr",
	"ovl_do_rename",
	"ovl_lock_rename_workdir",
	"ovl_remove_opaque",
	"ovl_whiteout",
	"ovl_clear_empty",
	"ovl_check_empty_and_clear",
	"ovl_rename2",
	"ovl_do_remove",
	"ovl_rmdir",
	"ovl_unlink",
	"ovl_create_over_whiteout",
	"ovl_create_or_link",
	"ovl_create_object",
	"ovl_mknod",
	"ovl_mkdir",
	"ovl_symlink",
	"ovl_create",
	"ovl_link",
	"ovl_dir_fsync",
	"ovl_check_whiteouts",
	"ovl_fill_merge",
	"ovl_dir_open",
	"ovl_dir_read_merged",
	"ovl_dir_release",
	"ovl_dir_llseek",
	"ovl_iterate",
	"ovl_set_timestamps",
	"ovl_dentry_update",
	"ovl_is_whiteout",
	"ovl_path_next",
	"ovl_do_whiteout_v1",
	"ovl_cleanup",
	"ovl_workdir",
	"ovl_is_private_xattr",
	"ovl_is_whiteout_v1",
	"ovl_entry_real",
	"ovl_config_legacy",
	"ovl_copy_up_one",
	"ovl_path_real",
	"ovl_dentry_real",
	"ovl_dir_cache",
	"ovl_prepare_creds",
	"ovl_dentry_version_inc",
	"ovl_copy_up",
	"ovl_lookup",
	"ovl_setxattr",
	"ovl_drop_write",
	"ovl_new_inode",
	"ovl_dentry_upper",
	"ovl_dentry_root_may",
	"ovl_d_select_inode",
	"ovl_dentry_is_opaque",
	"ovl_path_lower",
	"ovl_set_dir_cache",
	"ovl_path_type",
	"ovl_listxattr",
	"ovl_removexattr",
	"ovl_want_write",
	"ovl_setattr",
	"ovl_create_real",
	"ovl_dentry_set_opaque",
	"ovl_is_whiteout_v2",
	"ovl_cleanup_whiteouts",
	"ovl_cache_free",
	"ovl_check_empty_dir",
	"ovl_path_upper",
	"ovl_set_attr",
	"ovl_copy_xattr",
	"ovl_dentry_lower",
	"ovl_permission",
	"ovl_lookup_temp",
	"ovl_path_open",
	"ovl_getxattr",
	"ovl_dentry_version_get",
};

int main(int argc, char *argv[])
{
	int err, i;
	cov_register(0);
	char *pid0_to_read = "/home/zerons/Desktop/ofs/merge/l0.txt";
	char *pid1_to_read = "/home/zerons/Desktop/ofs/merge/u0.txt";

	for (i = 0; i < sizeof(func_to_probe)/sizeof(char *); i++) {
		err = checkpoint_add(func_to_probe[i], func_to_probe[i], 0);
		if (err == -1)
			printf("checkpoint_add %s err: %s\n", func_to_probe[i],
				strerror(errno));
	}
	char *buffer = malloc(THREAD_BUFFER_SIZE);
	if (!buffer) {
		cov_unregister();
		return -1;
	}
	memset(buffer, 0, THREAD_BUFFER_SIZE);

	int pid;
	if ((pid = fork()) < 0) {
		perror("fork");
	} else if (pid == 0) {
		cov_register(1);
		int fd = open(pid1_to_read, O_RDONLY);
		if (fd == -1)
			perror("open pid1_to_read");
		close(fd);

		int err = cov_get_buffer(buffer, THREAD_BUFFER_SIZE);
		if (err != -1)
			printf("pid1: \n%s\n", buffer);
		cov_unregister();
		exit(0);
	}

	sleep(9);
	int fd = open(pid0_to_read, O_RDONLY);
	if (fd == -1)
		perror("open pid0_to_read");
	close(fd);

	err = cov_get_buffer(buffer, THREAD_BUFFER_SIZE);
	if (err != -1)
		printf("pid0: \n%s\n", buffer);

	double percent;
	err = get_coverage(&percent);
	printf("%d\n", err);
	printf("%.2lf\n", percent);

	unsigned long path_count;
	err = cov_path_count(&path_count);
	printf("%d\n", err);
	printf("%ld\n", path_count);

	cov_unregister();
	return 0;
}

#include "../userlib/codecov.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	int err;
	cov_register();

	err = checkpoint_add("ovl_mount", "ovl_mount", 0);
	printf("%d\n", err);
	err = checkpoint_add("ovl_prepare_creds", "ovl_prepare_creds", 0);
	printf("%d\n", err);
	err = checkpoint_add("ovl_path_type", "ovl_path_type", 0);
	printf("%d\n", err);
	err = checkpoint_add("ovl_path_upper", "ovl_path_upper", 0);
	printf("%d\n", err);
	err = checkpoint_add("ovl_path_real", "ovl_path_real", 0);
	printf("%d\n", err);
	err = checkpoint_add("ovl_dentry_upper", "ovl_dentry_upper", 0);
	printf("%d\n", err);
	err = checkpoint_add("ovl_dentry_lower", "ovl_dentry_lower", 0);
	printf("%d\n", err);
	err = checkpoint_add("ovl_dentry_real", "ovl_dentry_real", 0);
	printf("%d\n", err);

	getchar();
	double percent;
	err = get_coverage(&percent);
	printf("%d\n", err);
	printf("%.2lf\n", percent);

	cov_unregister();
	return 0;
}

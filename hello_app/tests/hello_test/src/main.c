#include <version.h> /* KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_PATCHLEVEL */
#include <zephyr/ztest.h>

ZTEST_SUITE(hello_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(hello_suite, test_basic_sanity)
{
	zassert_equal(1, 1, "Sanity check failed");
	zassert_true(IS_ENABLED(CONFIG_PRINTK), "PRINTK must be enabled");
}

ZTEST(hello_suite, test_kernel_version)
{
	zassert_true(KERNEL_VERSION_MAJOR > 0, "KERNEL_VERSION_MAJOR should be > 0, got %d",
		     KERNEL_VERSION_MAJOR);

	TC_PRINT("Zephyr version: %d.%d.%d\n", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR,
		 KERNEL_PATCHLEVEL);
}

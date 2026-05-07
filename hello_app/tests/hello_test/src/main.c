#include "hello_app/src/app_logic.h"
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

ZTEST(hello_suite, test_validate_input)
{
	zassert_equal(validate_input(0), 0, "0 should be valid");
	zassert_equal(validate_input(50), 0, "50 should be valid");
	zassert_equal(validate_input(99), 0, "99 should be valid");
	zassert_equal(validate_input(-1), -1, "-1 should be invalid");
	zassert_equal(validate_input(100), -1, "100 should be invalid");
}

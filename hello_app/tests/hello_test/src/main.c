#include <zephyr/ztest.h>

ZTEST_SUITE(hello_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(hello_suite, test_basic_sanity)
{
	/* Placeholder – demonstrates Twister + Ztest integration on native_sim.
	 * Replace with application-specific unit tests. */
	zassert_equal(1, 1, "Sanity check failed");
	zassert_true(IS_ENABLED(CONFIG_PRINTK), "PRINTK must be enabled");
}

ZTEST(hello_suite, test_kernel_version)
{
	/* Verify Zephyr major version matches SBOM entry (nRF Connect SDK v2.2.0 → Zephyr 3.x) */
	zassert_equal(KERNEL_VERSION_MAJOR, 3, "Zephyr major != 3 (got %d)", KERNEL_VERSION_MAJOR);
}

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("Hello, Zephyr!\n");
	printk("Board:       %s\n", CONFIG_BOARD);
	printk("nRF Connect SDK v2.2.0 / Zephyr RTOS v3.2.x\n");

	while (1) {
		k_sleep(K_SECONDS(1));
		printk("tick\n");
	}

	return 0;
}

#include "app_logic.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	printk("Hello, Zephyr!\n");
	printk("Board:       %s\n", app_board_name());
	printk("Zephyr RTOS v3.2.0\n");

	while (1) {
		k_sleep(K_SECONDS(1));
		printk("tick\n");
	}

	return 0;
}

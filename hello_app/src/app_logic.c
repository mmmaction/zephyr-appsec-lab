#include "hello_app/src/app_logic.h"
#include <zephyr/kernel.h>

int validate_input(int val)
{
	return (val >= 0 && val < 100) ? 0 : -1;
}

const char *app_board_name(void)
{
	return CONFIG_BOARD;
}

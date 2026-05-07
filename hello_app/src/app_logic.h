#ifndef HELLO_APP_SRC_APP_LOGIC_H_
#define HELLO_APP_SRC_APP_LOGIC_H_

/**
 * Validate an input value.
 *
 * @param val Input value to check.
 * @return 0 if valid (0 <= val < 100), -1 otherwise.
 */
int validate_input(int val);

/**
 * Return the board name string.
 *
 * @return CONFIG_BOARD string.
 */
const char *app_board_name(void);

#endif // HELLO_APP_SRC_APP_LOGIC_H_

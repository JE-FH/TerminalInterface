#include "../shared/terminal2.h"
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <utf8proc.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

/*Sets the appropiate flags to disable echo and make input immediately available*/
void terminal2_setup() {
	struct termios tio;

	tcgetattr(STDIN_FILENO, &tio);

	tio.c_lflag = tio.c_lflag & ~(ICANON | ECHO);

	tio.c_cc[VTIME] = 0;
	tio.c_cc[VMIN] = 0;

	int old_fl = fcntl(STDIN_FILENO, F_GETFL); // i forgor why this work 💀

	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

	tcsetattr(STDIN_FILENO, TCSANOW, &tio);
}

/*Resets these flags to the standard*/
void terminal2_reset() {
	struct termios tio;

	tcgetattr(STDIN_FILENO, &tio);

	tio.c_lflag = tio.c_lflag | ICANON | ECHO;

	fcntl(STDIN_FILENO, F_SETFL, 0);

	tcsetattr(STDIN_FILENO, TCSANOW, &tio);

}

bool terminal2_read_input(char* out) {
	return read(STDIN_FILENO, out, 1) > 0;
}

void terminal2_enable_blocking() {
	fcntl(STDIN_FILENO, F_SETFL, 0);
}

void terminal2_disable_blocking() {
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}
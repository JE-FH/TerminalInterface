#include "../shared/terminal2.h"
#include "../queue.h"
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <utf8proc.h>
#include <signal.h>
#include <windows.h>

Queue input_queue = {
	NULL,
	0,
	0,
	0,
	0
};

DWORD old_stdin_mode = -1;
DWORD old_stdout_mode = -1;


HANDLE hStdin = INVALID_HANDLE_VALUE;
HANDLE hStdout = INVALID_HANDLE_VALUE;

void set_stdin_handle() {
	if (hStdin == INVALID_HANDLE_VALUE) {
		hStdin = GetStdHandle(STD_INPUT_HANDLE);
		if (hStdin == INVALID_HANDLE_VALUE) {
			//fatal_error("Could not get stdin handle");
			printf("Could not get stdin handle");
			exit(1);
		}
	}
}

void set_stdout_handle() {
	if (hStdout == INVALID_HANDLE_VALUE) {
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdout == INVALID_HANDLE_VALUE) {
			//fatal_error("Could not get stdout handle");
			printf("Could not get stdout handle");
			exit(1);
		}
	}
}

/*Sets the appropiate flags to disable echo and make input immediately available*/
void terminal2_setup() {
	set_stdin_handle();
	set_stdout_handle();

	if (input_queue.data == NULL) {
		input_queue = Queue_new(1, 256);
	}
	
	if (!GetConsoleMode(hStdin, &old_stdin_mode)) {
		//fatal_error("could not get stdin mode, error code: %x", GetLastError());
		printf("could not get stdin mode, error code: %x", GetLastError());
		exit(1);
	}
	if (!GetConsoleMode(hStdout, &old_stdout_mode)) {
		//fatal_error("Could not get stdout mode, error code: %x", GetLastError());
		printf("Could not get stdout mode, error code: %x", GetLastError());
		exit(1);
	}

	if (SetConsoleMode(hStdin, ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT) == 0) {
		//fatal_error("Could not set console mode, error code: %x", GetLastError());
		printf("Could not set console mode, error code: %x", GetLastError());
		exit(1);
	}
	if (SetConsoleMode(hStdout, ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN | ENABLE_PROCESSED_OUTPUT ) == 0) {
		//fatal_error("Could not set console mode, error code: %x", GetLastError());
		printf("Could not set console mode, error code: %x", GetLastError());
		exit(1);
	}
}

/*Resets these flags to the standard*/
void terminal2_reset() {
	set_stdin_handle();
	set_stdout_handle();

	if (input_queue.data != NULL) {
		Queue_free(&input_queue);
	}

	if (old_stdin_mode != -1) {
		SetConsoleMode(hStdin, old_stdin_mode);
	}
	if (old_stdout_mode != -1) {
		SetConsoleMode(hStdout, old_stdout_mode);
	}
}

bool blocking_enabled = false;
/*
* this version supports surrogates but because the windows function are so impossible to use, i cant make this work
*/
/*
bool terminal2_read_input(char* out) {
	set_stdin_handle();

	if (Queue_dequeue(&input_queue, out)) {
		return true;
	}

	INPUT_RECORD input_record;
	DWORD number_read;
	if (!blocking_enabled) {
		while (true) {
			if (!PeekConsoleInput(hStdin, &input_record, 1, &number_read)) {
			}
				fatal_error("Could not read console input, error code: %x", GetLastError());
			if (number_read == 0) {
				return false;
			}
			if (input_record.EventType != KEY_EVENT || !input_record.Event.KeyEvent.bKeyDown) {
				ReadConsoleInput(hStdin, &input_record, 1, &number_read);
			} else {
				break;
			}
		}
	}

	wchar_t c;
	uint32_t decoded_char;
	ReadConsoleW(hStdin, &c, 1, &number_read, NULL);
	if (number_read > 0) {
		if ((c >= 0 && c <= 0xD7FF) || (c >= 0xE000 && c <= 0xFFFF)) {
			decoded_char = c;
		} else {
			if ((c & 0b1101100000000000) && (~c & 0b0010010000000000)) {
				decoded_char = (((uint32_t)c) & 0b1111111111) << 10;
				ReadConsoleW(hStdin, &c, 1, &number_read, NULL);
				if (number_read > 0) {
					if ((c & 0b1101110000000000) && (~c & 0b0010000000000000)) {
						decoded_char |= c & 0b1111111111;
					} else {
					}
						fatal_error("Got invalid character from console");
				} else {
				}
					fatal_error("Wrongly encoded character received from console");
			} else {
			}
				fatal_error("Got invalid character from console");
		}
		char utf8encoded[4];
		int encoded_len = encode_to_utf8(decoded_char, utf8encoded);
		for (int i = 0; i < encoded_len; i++) {
			Queue_enqueue(&input_queue, &(utf8encoded[i]));
		}
	}

	if (Queue_dequeue(&input_queue, out)) {
		return true;
	} else {
		return false;
	}
}
*/

bool terminal2_read_input(char* out) {
	set_stdin_handle();

	if (Queue_dequeue(&input_queue, out)) {
		return true;
	}

	INPUT_RECORD input_record;
	DWORD number_read;
	if (!blocking_enabled) {
		while (true) {
			if (!PeekConsoleInput(hStdin, &input_record, 1, &number_read)) {
				//TODO: need some kind of fatal error handler
				//fatal_error("Could not read console input, error code: %x", GetLastError());
				printf("Could not read console input, error code: %x", GetLastError());
				exit(1);
			}
			if (number_read == 0) {
				return false;
			}
			if (input_record.EventType != KEY_EVENT || !input_record.Event.KeyEvent.bKeyDown) {
				ReadConsoleInput(hStdin, &input_record, 1, &number_read);
			} else {
				break;
			}
		}
	}

	input_record.EventType = 0;
	while (input_record.EventType != KEY_EVENT) {
		ReadConsoleInput(hStdin, &input_record, 1, &number_read);
	}

	char utf8encoded[4];
	int encoded_len = encode_to_utf8(input_record.Event.KeyEvent.uChar.UnicodeChar, utf8encoded);
	for (int i = 0; i < encoded_len; i++) {
		Queue_enqueue(&input_queue, &(utf8encoded[i]));
	}

	if (Queue_dequeue(&input_queue, out)) {
		return true;
	} else {
		return false;
	}
}

void terminal2_enable_blocking() {
	blocking_enabled = true;
}

void terminal2_disable_blocking() {
	blocking_enabled = false;
}
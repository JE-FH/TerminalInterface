#include <stdlib.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "terminal_interface.h"
#include "queue.h"
#include "Utf8Iterator.h"
#include "shared/terminal2.h"

#define DEFAULT_BG_COLOR TERM_COLOR_BLACK
#define DEFAULT_TEXT_COLOR TERM_COLOR_WHITE

#define ANSI_BRIGHT(x) (x + 60)
#define ANSI_BG(x) (x + 40)
#define ANSI_FG(x) (x + 30)

typedef struct TICharInfo {
	uint32_t code_point;
	TermColor text_color;
	TermColor background_color;
} TICharInfo;

struct TIState {
	int cursor_x;
	int cursor_y;
	uint16_t terminal_width;
	uint16_t terminal_height;
	TermColor current_text_color;
	TermColor current_background_color;
	TICharInfo* back_buffer;
	TICharInfo* front_buffer;
	bool is_front_buffer_undefined;
	Queue input_buffer;
	/*Space enough for atleast one codepoint encoded as utf8 and then some in case of encoding error*/
	uint8_t tmp_input[10];
	uint8_t tmp_input_len;
	/*if false it will only print extended ascii but still encoded using utf8, it will also supply input in extended ascii*/
	bool use_utf8;
};

void set_real_cursor_pos(int x, int y);
void set_real_text_color(TermColor color);
void set_real_back_color(TermColor color);
bool TI_get_real_cursor_pos(TIState* self, int* x, int* y);
void TI_read_remaining_input(TIState* self);
void TI_read_remaining_input_once(TIState* self);
void TI_try_decode_tmp_input(TIState* self);

/*out should have enough space for 4 chars or test len with null out*/
size_t encode_to_utf8(uint32_t c, char* out);
uint32_t decode_utf8_2(char c, char c2);
uint32_t decode_utf8_3(char c, char c2, char c3);
uint32_t decode_utf8_4(char c, char c2, char c3, char c4);
bool is_utf8_seq(char c);

int color_to_ansi(TermColor term_color);

/*Gets the char info at a specific 1-based x and y*/
TICharInfo* TI_get_char_info_safe(TIState* self, TICharInfo* buffer, uint16_t cursor_x, uint16_t cursor_y);


TIState* TI_init(bool use_utf8) {
	TIState* self = calloc(1, sizeof(TIState));
	if (self == NULL) {
		return NULL;
	}
	
	self->cursor_x = 1;
	self->cursor_y = 1;

	self->current_background_color = DEFAULT_BG_COLOR;
	self->current_text_color = DEFAULT_TEXT_COLOR;
	
	self->input_buffer = Queue_new(sizeof(uint32_t), 256);
	self->tmp_input_len = 0;

	self->use_utf8 = use_utf8;

	terminal2_setup();

	printf("\033[?1049h");

	set_real_cursor_pos(1000,1000);

	int w, h;
	if (!TI_get_real_cursor_pos(self, &w, &h)) {
		TI_free(self);
		return NULL;
	}

	self->terminal_width = w;
	self->terminal_height = h;

	self->is_front_buffer_undefined = true;

	self->back_buffer = calloc(self->terminal_height * self->terminal_width, sizeof(TICharInfo));
	if (self->back_buffer == NULL) {
		TI_free(self);
		return NULL;
	}
	self->front_buffer = calloc(self->terminal_height * self->terminal_width, sizeof(TICharInfo));
	if (self->front_buffer == NULL) {
		TI_free(self);
		return NULL;
	}
	TI_clear(self);

	return self;
}

void TI_free(struct TIState* self) {
	if (self == NULL) {
		return;
	}
	if (self->back_buffer != NULL) {
		free(self->back_buffer);
	}
	if (self->front_buffer != NULL) {
		free(self->front_buffer);
	}
	Queue_free(&self->input_buffer);
	free(self);
	terminal2_reset();
	printf("\033[?1049l");

}

void TI_set_cursor_pos(struct TIState* self, int x, int y) {
	self->cursor_x = x;
	self->cursor_y = y;
}

void TI_set_text_color(struct TIState* self, TermColor color) {
	self->current_text_color = color;
}

void TI_set_background_color(struct TIState* self, TermColor color) {
	self->current_background_color = color;
}

TermColor TI_get_text_color(struct TIState* self) {
	return self->current_text_color;
}

TermColor TI_get_background_color(struct TIState* self) {
	return self->current_background_color;
}

void TI_get_cursor_pos(struct TIState* self, int* x, int* y) {
	if (x != NULL) {
		*x = self->cursor_x;
	}
	if (y != NULL) {
		*y = self->cursor_y;
	}
}

void TI_get_size(struct TIState* self, int* w, int* h) {
	if (w != NULL) {
		*w = self->terminal_width;
	}
	if (h != NULL) {
		*h = self->terminal_height;
	}
}

void TI_write_no_wrap_ss(struct TIState* self, const char* data, size_t len) {
	if (self->use_utf8) {
		Utf8Iterator data_it = Utf8Iterator_new(data, len);
		uint32_t c;
		while (Utf8Iterator_has_next(&data_it)) {
			Utf8Iterator_next(&data_it, &c);
			TI_write_single(self, c, self->current_text_color, self->current_background_color);
		}
	} else {
		for (size_t i = 0; i < len; i++) {
			char c = data[i];
			TI_write_single(self, data[i], self->current_text_color, self->current_background_color);
		}
	}
}

void TI_write_no_wrap_sz(struct TIState* self, const char* data) {
	TI_write_no_wrap_ss(self, data, strlen(data));
}

void TI_write_single(struct TIState* self, uint32_t code_point, TermColor text_color, TermColor background_color) {
	TICharInfo* info = TI_get_char_info_safe(self, self->back_buffer, self->cursor_x, self->cursor_y);
	if (info != NULL) {
		info->background_color = background_color;
		info->text_color = text_color;
		/*from https://en.wikipedia.org/wiki/Control_character*/
		if (code_point > 255) {
			self->cursor_x = self->cursor_x;
		}
		if (code_point > 0x1F && code_point != 0x7F && (code_point < 0x80 || code_point > 0x9F)) {
			info->code_point = code_point;
		} else {
			info->code_point = ' ';
		}
	}
	self->cursor_x += 1;
}

void TI_update_display(struct TIState* self) {
	bool skipped_one = true;
	TermColor real_text_color = -1;
	TermColor real_back_color = -1;
	for (uint16_t y = 1; y <= self->terminal_height; y++) {
		for (uint16_t x = 1; x <= self->terminal_width; x++) {
			TICharInfo* back_info = TI_get_char_info_safe(self, self->back_buffer, x, y);
			TICharInfo* front_info = TI_get_char_info_safe(self, self->front_buffer, x, y);
			if (self->is_front_buffer_undefined ||
				back_info->background_color != front_info->background_color ||
				back_info->code_point != front_info->code_point ||
				back_info->text_color != front_info->text_color
			) {
				if (skipped_one) {
					set_real_cursor_pos(x, y);
					skipped_one = false;
				}

				if (back_info->code_point != ' ' && real_text_color != back_info->text_color) {
					set_real_text_color(back_info->text_color);
					real_text_color = back_info->text_color;
				}

				if (real_back_color != back_info->background_color) {
					set_real_back_color(back_info->background_color);
					real_back_color = back_info->background_color;
				}

				if (!self->use_utf8 && back_info->code_point > 255) {
					putchar('?');
				} else {
					char encoded[4];
					size_t len = encode_to_utf8(back_info->code_point, encoded);
					for (size_t i = 0; i < len; i++) {
						putchar(encoded[i]);
					}
				}

				*front_info = *back_info;
			} else {
				skipped_one = true;
			}
		}
	}

	set_real_cursor_pos(self->cursor_x, self->cursor_y);
	fflush(stdout);

	self->is_front_buffer_undefined = false;
}

uint32_t TI_read_key(struct TIState* self) {
	terminal2_enable_blocking();

	while (true) {
		uint32_t next_char;
		if (Queue_dequeue(&self->input_buffer, &next_char)) {
			return next_char;
		}
		TI_read_remaining_input_once(self);
	}
	
	terminal2_disable_blocking();
}

uint32_t TI_peek_key(struct TIState* self) {
	TI_read_remaining_input(self);
	uint32_t next_char;
	if (Queue_dequeue(&self->input_buffer, &next_char)) {
		return next_char;
	
	}
	return 0xFFFFFFFF;
}

void TI_clear(struct TIState* self) {
	for (size_t i = 0; i < self->terminal_width * self->terminal_height; i++) {
		self->back_buffer[i].background_color = self->current_background_color;
		self->back_buffer[i].text_color = self->current_text_color;
		self->back_buffer[i].code_point = ' ';
	}
}

void TI_scroll_down(struct TIState* self, unsigned int amount) {
	if (amount >= self->terminal_height) {
		TI_clear(self);
	}
	if (amount == 0) {
		return;
	}
	memmove(
		TI_get_char_info_safe(self, self->back_buffer, 1, 1), 
		TI_get_char_info_safe(self, self->back_buffer, 1, amount + 1),
		sizeof(TICharInfo) * self->terminal_width * (self->terminal_height - amount)
	);
	for (uint16_t y = self->terminal_height - amount + 1; y <= self->terminal_height; y++) {
		for (uint16_t x = 1; x <= self->terminal_width; x++) {
			TICharInfo* info = TI_get_char_info_safe(self, self->back_buffer, x, y);
			info->code_point = ' ';
			info->background_color = self->current_background_color;
			info->text_color = self->current_text_color;
		}
	}
}

void TI_scroll_up(struct TIState* self, unsigned int amount) {
	if (amount >= self->terminal_height) {
		TI_clear(self);
	}
	if (amount == 0) {
		return;
	}
	//Not actually implemented yet haha
}

void TI_clear_current_line(struct TIState* self) {
	for (uint16_t x = 1; x <= self->terminal_width; x++) { 
		TICharInfo* info = TI_get_char_info_safe(self, self->back_buffer, x, self->cursor_y);
		info->background_color = self->current_background_color;
		info->text_color = self->current_text_color;
		info->code_point = ' ';
	}
}

void set_real_cursor_pos(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

void set_real_text_color(TermColor color) {
	printf("\033[%dm", ANSI_FG(color_to_ansi(color)));
}

void set_real_back_color(TermColor color) {
	printf("\033[%dm", ANSI_BG(color_to_ansi(color)));
}

bool TI_get_real_cursor_pos(TIState* self, int* x, int* y) {
	TI_read_remaining_input(self);
	terminal2_enable_blocking();
	printf("\033[6n"); 
	int res = scanf("\033[%i;%iR", y, x);
	terminal2_disable_blocking();

	//If res is not 2, then we could not read the cursor pos
	return res == 2;
}

void TI_read_remaining_input_once(TIState* self) {
	char c;
	terminal2_read_input(&c);
	if (self->tmp_input_len >= sizeof(self->tmp_input)) {
		//TODO: Here some kind of error callback must happen so that this library can be relied on to not exit your application
		printf("nej nej nej\n");
		exit(1);
	}
	self->tmp_input[self->tmp_input_len++] = c;
	TI_try_decode_tmp_input(self);
}

void TI_read_remaining_input(TIState* self) {
	char c;
	while (terminal2_read_input(&c)) {
		if (self->tmp_input_len >= sizeof(self->tmp_input)) {
			//TODO: Here some kind of error callback must happen so that this library can be relied on to not exit your application
			printf("nej nej nej\n");
			exit(1);
		}
		self->tmp_input[self->tmp_input_len++] = c;
		TI_try_decode_tmp_input(self);
	}
}

bool is_utf8_continuation(uint8_t c) {
	return ((c & 0b10000000) && (~c & 0b01000000));
}

void TI_try_decode_tmp_input(TIState* self) {
	if (self->tmp_input_len == 0) {
		return;
	}
	uint32_t c;

	if (self->tmp_input[0] <= 127 && self->tmp_input_len == 1) {
		c = self->tmp_input[0];
		Queue_enqueue(&self->input_buffer, &c);
		/*There can be no characters behind*/
		self->tmp_input_len = 0;
		return;
	}
	if (self->tmp_input[0] & 0b11000000 && (~self->tmp_input[0] & 0b00100000)) {
		if (self->tmp_input_len == 2) {
			if (!is_utf8_continuation(self->tmp_input[1])) {
				c = '?';
				Queue_enqueue(&self->input_buffer, &c);
				self->tmp_input[0] = self->tmp_input[self->tmp_input_len - 1];
				self->tmp_input_len = 1;
				return;
			}

			c = decode_utf8_2(self->tmp_input[0], self->tmp_input[1]);
			Queue_enqueue(&self->input_buffer, &c);
			self->tmp_input_len = 0;
		} else {
			return;
		}
	}

	if (self->tmp_input[0] & 0b11100000 && (~self->tmp_input[0] & 0b00010000)) {
		if (self->tmp_input_len == 3) {
			if (!is_utf8_continuation(self->tmp_input[1])) {
				c = '?';
				Queue_enqueue(&self->input_buffer, &c);
				memmove(self->tmp_input, self->tmp_input + 1, 2);
				self->tmp_input_len = 2;
				return;
			}

			if (!is_utf8_continuation(self->tmp_input[2])) {
				c = '?';
				Queue_enqueue(&self->input_buffer, &c);
				self->tmp_input[0] = self->tmp_input[self->tmp_input_len - 1];
				self->tmp_input_len = 1;
				return;
			}

			c = decode_utf8_3(self->tmp_input[0], self->tmp_input[1], self->tmp_input[2]);
			Queue_enqueue(&self->input_buffer, &c);
			self->tmp_input_len = 0;
		} else {
			return;
		}
	}

	if (self->tmp_input[0] & 0b11110000 && (~self->tmp_input[0] & 0b00001000)) {
		if (self->tmp_input_len == 4) {
			if (!is_utf8_continuation(self->tmp_input[1])) {
				c = '?';
				Queue_enqueue(&self->input_buffer, &c);
				memmove(self->tmp_input, self->tmp_input + 1, 3);
				self->tmp_input_len = 3;
				return;
			}
			
			if (!is_utf8_continuation(self->tmp_input[1])) {
				c = '?';
				Queue_enqueue(&self->input_buffer, &c);
				memmove(self->tmp_input, self->tmp_input + 2, 2);
				self->tmp_input_len = 2;
				return;
			}

			if (!is_utf8_continuation(self->tmp_input[2])) {
				c = '?';
				Queue_enqueue(&self->input_buffer, &c);
				self->tmp_input[0] = self->tmp_input[self->tmp_input_len - 1];
				self->tmp_input_len = 1;
				return;
			}

			c = decode_utf8_4(self->tmp_input[0], self->tmp_input[1], self->tmp_input[2], self->tmp_input[3]);
			Queue_enqueue(&self->input_buffer, &c);
			self->tmp_input_len = 0;

		} else {
			return;
		}
	}

	//Cant be a valid unicode sequence
	
	if (self->tmp_input_len == 1) {
		c = '?';
		Queue_enqueue(&self->input_buffer, &c);
		/*There can be no characters behind*/
		self->tmp_input_len -= 1;
	} else {
		c = '?';
		for (size_t i = 0; i < self->tmp_input_len - 1; i++) {
			Queue_enqueue(&self->input_buffer, &c);
		}
		self->tmp_input[0] = self->tmp_input[self->tmp_input_len - 1];
		self->tmp_input_len = 1;
	}
}

TICharInfo* TI_get_char_info_safe(TIState* self, TICharInfo* buffer, uint16_t cursor_x, uint16_t cursor_y) {
	if (cursor_x <= 0 || cursor_x > self->terminal_width)  {
		return NULL;
	} else if (cursor_y <= 0 || cursor_y > self->terminal_height) {
		return NULL;
	}

	return buffer + (cursor_x - 1 + (cursor_y - 1) * self->terminal_width);
}


size_t encode_to_utf8(uint32_t c, char* out) {
	if (c < 0x80) {
		if (out != NULL) {
			*out = c;
		}
		return 1;
	} else if (c < 0x800) {
		if (out != NULL)  {
			out[0] = 0b11000000 | ((0b11111000000 & c) >> 6);
			out[1] = 0b10000000 | ((0b00000111111 & c));
		}
		return 2;
	} else if (c < 0x10000) {
		if (out != NULL)  {
			out[0] = 0b11100000 | ((0b1111000000000000 & c) >> 12);
			out[1] = 0b10000000 | ((0b0000111111000000 & c) >> 6);
			out[2] = 0b10000000 | ((0b0000000000111111 & c));
		}
		return 3;
	} else if (c < 0x110000) {
		if (out != NULL)  {
			out[0] = 0b11110000 | ((0b111000000000000000000 & c) >> 18);
			out[1] = 0b10000000 | ((0b000111111000000000000 & c) >> 12);
			out[2] = 0b10000000 | ((0b000000000111111000000 & c) >> 6);
			out[3] = 0b10000000 | ((0b000000000000000111111 & c));
		}
		return 4;
	} else {
		if (out != NULL) {
			*out = '?';
		}
		return 0;
	}
}


#define T32(c) ((uint32_t) c)

uint32_t decode_utf8_2(char c, char c2) {
	if (!is_utf8_seq(c2)) {
		return 0;
	}
	return ((T32(c) & 0b00011111) << 6) | (T32(c2) & 0b00111111);

}

uint32_t decode_utf8_3(char c, char c2, char c3) {
	if (!(is_utf8_seq(c2) && is_utf8_seq(c3))) {
		return 0;
	}
	return ((T32(c) & 0b00001111) << 12) | ((T32(c2) & 0b00111111) << 6) | (T32(c3) & 0b00111111);
}

uint32_t decode_utf8_4(char c, char c2, char c3, char c4) {
	if (!(is_utf8_seq(c2) && is_utf8_seq(c3) && is_utf8_seq(c4))) {
		return 0;
	} 
	return ((T32(c) & 0b00000111) << 18) | ((T32(c2) & 0b00111111) << 12) | ((T32(c3) & 0b00111111) << 6) | (T32(c4) & 0b00111111);
}

bool is_utf8_seq(char c) {
	return ((c & 0b10000000) && (~c & 0b01000000));
}

int color_to_ansi(TermColor term_color) {
	switch (term_color) {
		case TERM_COLOR_WHITE: return ANSI_BRIGHT(ANSI_COLOR_WHITE);
		case TERM_COLOR_ORANGE: return ANSI_BRIGHT(ANSI_COLOR_CYAN);
		case TERM_COLOR_MEGENTA: return ANSI_BRIGHT(ANSI_COLOR_MAGENTA);
		case TERM_COLOR_LIGHT_BLUE: return ANSI_BRIGHT(ANSI_COLOR_BLUE);
		case TERM_COLOR_YELLOW: return ANSI_BRIGHT(ANSI_COLOR_YELLOW);
		case TERM_COLOR_LIME: return ANSI_BRIGHT(ANSI_COLOR_GREEN);
		case TERM_COLOR_PINK: return ANSI_BRIGHT(ANSI_COLOR_RED);
		case TERM_COLOR_GRAY: return ANSI_BRIGHT(ANSI_COLOR_BLACK);
		case TERM_COLOR_LIGHT_GRAY: return ANSI_COLOR_WHITE;
		case TERM_COLOR_CYAN: return ANSI_COLOR_CYAN;
		case TERM_COLOR_PURPLE: return ANSI_COLOR_MAGENTA;
		case TERM_COLOR_BLUE: return ANSI_COLOR_BLUE;
		case TERM_COLOR_BROWN: return ANSI_COLOR_YELLOW;
		case TERM_COLOR_GREEN: return ANSI_COLOR_GREEN;
		case TERM_COLOR_RED: return ANSI_COLOR_RED;
		case TERM_COLOR_BLACK: return ANSI_COLOR_BLACK;
		default: return ANSI_COLOR_BLACK;
	}
}

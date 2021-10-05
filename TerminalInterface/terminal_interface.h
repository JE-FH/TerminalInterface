#pragma once
//Terminal interface uses control sequences from ECMA-48, missing functionality is implemented os specific 
//When printing, all null chars are replaces with spaces
//Terminal interface also reads input to utf32 buffer
//invalid utf8 formatting from the input buffer should just replace all the bad chars in the sequence with ?
//invalid utf8 formatting to write functions can optionally be handled (replaced by ?) or throw an error
//All control characters should be replaced with a space

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
 
typedef enum TermColor {
	TERM_COLOR_WHITE,
	TERM_COLOR_ORANGE,
	TERM_COLOR_MEGENTA,
	TERM_COLOR_LIGHT_BLUE,
	TERM_COLOR_YELLOW,
	TERM_COLOR_LIME,
	TERM_COLOR_PINK,
	TERM_COLOR_GRAY,
	TERM_COLOR_LIGHT_GRAY,
	TERM_COLOR_CYAN,
	TERM_COLOR_PURPLE,
	TERM_COLOR_BLUE,
	TERM_COLOR_BROWN,
	TERM_COLOR_GREEN,
	TERM_COLOR_RED,
	TERM_COLOR_BLACK
} TermColor;

enum AnsiColors {
	ANSI_COLOR_BLACK = 0,
	ANSI_COLOR_RED = 1,
	ANSI_COLOR_GREEN = 2,
	ANSI_COLOR_YELLOW = 3,
	ANSI_COLOR_BLUE = 4,
	ANSI_COLOR_MAGENTA = 5,
	ANSI_COLOR_CYAN = 6,
	ANSI_COLOR_WHITE = 7,
};

struct TIState;
typedef struct TIState TIState;

struct TIState* TI_init(bool use_utf8, bool add_key_events);

void TI_free(struct TIState* self);

void TI_set_cursor_pos(struct TIState* self, int x, int y);
void TI_get_cursor_pos(struct TIState* self, int* x, int* y);

void TI_set_text_color(struct TIState* self, TermColor color);
void TI_set_background_color(struct TIState* self, TermColor color);

TermColor TI_get_text_color(struct TIState* self);
TermColor TI_get_background_color(struct TIState* self);

uint32_t TI_read_key(struct TIState* self);
uint32_t TI_peek_key(struct TIState* self);

void TI_get_size(struct TIState* self, int* w, int* h);

/*For Strings terminated by Zero*/
void TI_write_no_wrap_sz(struct TIState* self, const char* data);

/*For Sized Strings*/
void TI_write_no_wrap_ss(struct TIState* self, const char* data, size_t len);

/*Writes a single codepoint without changing the current color*/
void TI_write_single(struct TIState* self, uint32_t code_point, TermColor text_color, TermColor background_color);

void TI_update_display(struct TIState* self);

void TI_clear(struct TIState* self);

void TI_scroll_down(struct TIState* self, unsigned int amount);
void TI_scroll_up(struct TIState* self, unsigned int amount);

void TI_clear_current_line(struct TIState* self);

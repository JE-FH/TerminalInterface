#include <terminal_interface.h>
#include <string.h>

const char* crazy_text = "HELLOO!!!!!";

int main() {
	TIState* state = TI_init(true);

	int w, h;

	TI_get_size(state, &w, &h);

	int cx = w / 2;
	int cy = h / 2;

	int startx = cx - strlen(crazy_text)/2;

	TI_set_cursor_pos(state, startx, cy);

	TI_write_no_wrap_sz(state, "hello world!");
	
	TI_update_display(state);

	TI_read_key(state);

	TI_free(state);
	return 0;
}
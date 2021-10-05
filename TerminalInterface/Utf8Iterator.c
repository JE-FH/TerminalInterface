#include "Utf8Iterator.h"
Utf8Iterator Utf8Iterator_new(const char* str, size_t len) {
	utf8proc_ssize_t cp_len = utf8proc_decompose(str, len, NULL, 0, 0);
	if (cp_len < 0) {
		return (Utf8Iterator) {
			NULL,
			0,
			0,
			0
		};
	} else {
		return (Utf8Iterator) {
			str,
			0,
			cp_len,
			len
		};
	}
}


bool Utf8Iterator_has_next(Utf8Iterator* it) {
	return it->offset < it->len;
}

bool Utf8Iterator_next(Utf8Iterator* it, utf8proc_int32_t* c) {
	utf8proc_ssize_t len = utf8proc_iterate(it->str + it->offset, it->len - it->offset, c);
	if (len < 0) {
		return false;
	} else {
		it->offset += len;
		return true;
	}
}
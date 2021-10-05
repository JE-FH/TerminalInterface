#pragma once

#include <utf8proc.h>

typedef struct Utf8Iterator {
	const char* str;
	size_t offset;
	size_t cp_len;
	size_t len;
} Utf8Iterator;

Utf8Iterator Utf8Iterator_new(const char* str, size_t len);

bool Utf8Iterator_has_next(Utf8Iterator* it);

bool Utf8Iterator_next(Utf8Iterator* it, utf8proc_int32_t* c);
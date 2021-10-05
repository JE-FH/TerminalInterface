#pragma once
#include "stdbool.h"
/*Sets the appropiate flags to disable echo and make input immediately available*/
void terminal2_setup();
/*Resets these flags to the standard*/
void terminal2_reset();
/*Read a single character non blocking*/
bool terminal2_read_input(char* out);
void terminal2_enable_blocking();
void terminal2_disable_blocking();
#pragma once

#include <stdint.h>

#include "types.h"

void PrintBytes(const char* name, const struct Bytes bytes);

void PrintStringBytes(const char* name, const char* txt);

uint32_t LittleEndianUint32(const char* data);

void LittleEndianPutUint32(char* out, uint32_t v);

#include "v8.h"

// Copied from https://github.com/v8/v8/blob/master/samples/shell.cc
const char *ToCString(const v8::String::Utf8Value &value)
{
    return *value ? *value : "<string conversion failed>";
}

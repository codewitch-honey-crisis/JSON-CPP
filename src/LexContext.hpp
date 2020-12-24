#ifndef ARDUINO
#include <type_traits>
#include <cinttypes>
#include <cstddef>
#endif

class LexContext {
    virtual int32_t current()=0;
};
#include "range_sum.h"

uint64_t RangeSum(uint64_t from, uint64_t to, uint64_t step) {
    if (from >= to || step == 0) {
        return 0;
    }
    uint64_t n = 1 + ((to - 1) - from) / step;
    return from * n + step * n * (n - 1) / 2;
}
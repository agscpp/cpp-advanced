#include "is_prime.h"

#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <thread>
#include <vector>

namespace {
void Worker(uint64_t x, uint64_t from, uint64_t to, std::atomic<bool>& is_prime) {
    for (auto i : std::views::iota(uint64_t{from}, to)) {
        if (!is_prime) {
            return;
        }
        if (x % i == 0) {
            is_prime = false;
            return;
        }
    }
}
}  // namespace

bool IsPrime(uint64_t x) {
    if (x < 2) {
        return false;
    }

    std::atomic<bool> is_prime = true;
    size_t count_thread = std::thread::hardware_concurrency();
    uint64_t bound = std::min(static_cast<uint64_t>(std::sqrt(x)) + 6, x);
    uint64_t step = std::max(bound / count_thread, 1ul);

    std::vector<std::thread> workers;
    for (uint64_t from = 2; from <= bound; from += step) {
        if (!is_prime) {
            break;
        }
        uint64_t to = std::min(from + step, bound);
        workers.emplace_back(Worker, x, from, to, std::ref(is_prime));
    }
    for (auto& worker : workers) {
        worker.join();
    }
    return is_prime;
}

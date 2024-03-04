#pragma once

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <iterator>
#include <thread>
#include <vector>

namespace {
template <std::random_access_iterator Iterator, class T, class Func>
void Worker(Iterator first, Iterator last, Func func, T& result) {
    auto curr_value = *(first++);
    for (auto it = first; it != last; ++it) {
        curr_value = func(curr_value, *it);
    }
    result = curr_value;
}
}  // namespace

template <std::random_access_iterator Iterator, class T, class Func>
T Reduce(Iterator first, Iterator last, const T& init, Func func) {
    if (first == last) {
        return init;
    }

    size_t count_thread = std::thread::hardware_concurrency();
    size_t size = std::distance(first, last);
    size_t step = std::max(size / count_thread, 1ul);

    if (step == 1ul) {
        T result;
        Worker<Iterator, T>(first, last, func, result);
        return func(result, init);
    }

    std::vector<std::thread> workers;
    std::vector<T> answers(count_thread);
    for (size_t i = 0; i < count_thread - 1; ++i) {
        workers.emplace_back(Worker<Iterator, T, Func>, first, first + step, func,
                             std::ref(answers[i]));
        first += step;
    }
    workers.emplace_back(Worker<Iterator, T, Func>, first, last, func, std::ref(answers.back()));

    for (auto& worker : workers) {
        worker.join();
    }

    T result(init);
    for (const auto& answer : answers) {
        result = func(result, answer);
    }

    return result;
}

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <vector>

class Executor;

class Task : public std::enable_shared_from_this<Task> {
public:
    virtual ~Task() {
    }

    virtual void Run() = 0;

    void AddDependency(std::shared_ptr<Task> dependence);
    void AddTrigger(std::shared_ptr<Task> trigger);
    void SetTimeTrigger(std::chrono::system_clock::time_point at);

    void SetNotificationHandler(std::function<void(std::shared_ptr<Task>)> hander);
    void NotificationHandler();

    bool IsPending();
    bool IsCompleted();
    bool IsFailed();
    bool IsCanceled();
    bool IsFinished();

    std::exception_ptr GetError();
    bool HasLimitations();

    bool CaptureRunner();
    void Cancel();

    void Wait();

private:
    friend Executor;

    enum State { IDLE, PENDING, CAPTURED, COMPLETED, FAILED, CANCELED };

    std::mutex mutex_;
    std::condition_variable cv_;

    State state_ = State::IDLE;
    std::vector<std::weak_ptr<Task>> dependences_;
    std::vector<std::weak_ptr<Task>> subscribers_;
    std::vector<std::weak_ptr<Task>> triggers_;
    std::optional<std::function<void(std::shared_ptr<Task>)>> notification_handler_;

    std::exception_ptr exp_ptr_;
    std::optional<std::chrono::system_clock::time_point> time_trigger_;

    void AddSubscriber(std::shared_ptr<Task> subscriber);

    void Expect();
    void Complete();
    void SetError(std::exception_ptr exp_ptr);
    void NotifySubscribers(std::unique_lock<std::mutex>* guard);
    bool CanExecuted();

    std::optional<std::chrono::system_clock::time_point> GetTimeTrigger();
};

template <class T>
class Future;

template <class T>
using FuturePtr = std::shared_ptr<Future<T>>;

struct Unit {};

class Executor : public std::enable_shared_from_this<Executor> {
public:
    Executor(uint32_t num_threads);
    ~Executor();

    void Submit(std::shared_ptr<Task> task);

    void StartShutdown();
    void WaitShutdown();

    template <class T>
    FuturePtr<T> Invoke(std::function<T()> fn) {
        auto task = std::make_shared<Future<T>>(fn);
        Submit(task);
        return task;
    }

    template <class Y, class T>
    FuturePtr<Y> Then(FuturePtr<T> input, std::function<Y()> fn) {
        auto task = std::make_shared<Future<Y>>(fn);
        task->AddDependency(input);
        Submit(task);
        return task;
    }

    template <class T>
    FuturePtr<std::vector<T>> WhenAll(std::vector<FuturePtr<T>> all) {
        return Invoke<std::vector<T>>([all = std::move(all)]() {
            std::vector<T> result;
            result.reserve(all.size());
            for (auto& task : all) {
                result.emplace_back(task->Get());
            }
            return result;
        });
    }

    template <class T>
    FuturePtr<T> WhenFirst(std::vector<FuturePtr<T>> all) {
        auto main_task = std::make_shared<Future<T>>([all]() {
            for (auto& task : all) {
                if (task->IsFinished()) {
                    return task->Get();
                }
            }
            return T{};
        });
        for (auto& task : all) {
            main_task->AddTrigger(task);
        }
        Submit(main_task);
        return main_task;
    }

    template <class T>
    FuturePtr<std::vector<T>> WhenAllBeforeDeadline(
        std::vector<FuturePtr<T>> all, std::chrono::system_clock::time_point deadline) {
        auto task = std::make_shared<Future<std::vector<T>>>([all = std::move(all)]() {
            std::vector<T> result;
            result.reserve(all.size());
            for (auto& task : all) {
                if (task->IsFinished()) {
                    result.emplace_back(task->Get());
                }
            }
            return result;
        });
        task->SetTimeTrigger(deadline);
        Submit(task);
        return task;
    }

private:
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    std::vector<std::thread> workers_;
    std::deque<std::shared_ptr<Task>> tasks_;
    bool is_finished_ = false;

    void WorkerRunner();
};

std::shared_ptr<Executor> MakeThreadPoolExecutor(uint32_t num_threads);

template <class T>
class Future : public Task {
public:
    Future(std::function<T()> fn) : fn_(std::move(fn)) {
    }

    T Get() {
        Wait();
        if (IsCanceled()) {
            throw std::logic_error{"Task was canceled"};
        }
        if (IsFailed()) {
            rethrow_exception(GetError());
        }
        return value_;
    }

    void Run() override {
        value_ = fn_();
    }

private:
    std::function<T()> fn_;
    T value_;
};

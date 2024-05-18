#include "executor.h"

void Task::AddDependency(std::shared_ptr<Task> dependence) {
    std::lock_guard<std::mutex> guard(mutex_);
    dependence->AddSubscriber(shared_from_this());
    dependences_.push_back(std::move(dependence));
}

void Task::AddTrigger(std::shared_ptr<Task> trigger) {
    std::lock_guard<std::mutex> guard(mutex_);
    trigger->AddSubscriber(shared_from_this());
    triggers_.push_back(std::move(trigger));
}

void Task::AddSubscriber(std::shared_ptr<Task> subscriber) {
    std::lock_guard<std::mutex> guard(mutex_);
    subscribers_.push_back(std::move(subscriber));
}

void Task::SetTimeTrigger(std::chrono::system_clock::time_point at) {
    std::lock_guard<std::mutex> guard(mutex_);
    time_trigger_ = at;
}

void Task::SetNotificationHandler(std::function<void(std::shared_ptr<Task>)> handler) {
    std::lock_guard<std::mutex> guard(mutex_);
    notification_handler_.emplace(std::move(handler));
}

bool Task::IsPending() {
    std::lock_guard<std::mutex> guard(mutex_);
    return state_ == State::PENDING;
}

bool Task::IsCompleted() {
    std::lock_guard<std::mutex> guard(mutex_);
    return state_ == State::COMPLETED;
}

bool Task::IsFailed() {
    std::lock_guard<std::mutex> guard(mutex_);
    return state_ == State::FAILED;
}

bool Task::IsCanceled() {
    std::lock_guard<std::mutex> guard(mutex_);
    return state_ == State::CANCELED;
}

bool Task::IsFinished() {
    return IsCompleted() || IsFailed() || IsCanceled();
}

std::exception_ptr Task::GetError() {
    std::lock_guard<std::mutex> guard(mutex_);
    return exp_ptr_;
}

bool Task::CaptureRunner() {
    std::lock_guard<std::mutex> guard(mutex_);
    switch (state_) {
        case State::PENDING:
            state_ = State::CAPTURED;
            return true;
        case State::IDLE:
        case State::CAPTURED:
        case State::COMPLETED:
        case State::FAILED:
        case State::CANCELED:
            return false;
    }
}

void Task::Cancel() {
    std::unique_lock<std::mutex> lock(mutex_);
    switch (state_) {
        case State::IDLE:
        case State::PENDING:
        case State::CAPTURED:
            state_ = State::CANCELED;
            NotifySubscribers(&lock);
            cv_.notify_all();
            break;
        case State::COMPLETED:
        case State::FAILED:
        case State::CANCELED:
            break;
    }
}

void Task::Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    switch (state_) {
        case State::IDLE:
        case State::CAPTURED:
        case State::PENDING:
            cv_.wait(lock, [&]() {
                return state_ != State::IDLE && state_ != State::PENDING &&
                       state_ != State::CAPTURED;
            });
            break;
        case State::COMPLETED:
        case State::FAILED:
        case State::CANCELED:
            break;
    }
}

void Task::Expect() {
    std::lock_guard<std::mutex> guard(mutex_);
    state_ = State::PENDING;
}

void Task::Complete() {
    std::unique_lock<std::mutex> lock(mutex_);
    state_ = State::COMPLETED;
    NotifySubscribers(&lock);
    cv_.notify_all();
}

void Task::SetError(std::exception_ptr exp_ptr) {
    std::unique_lock<std::mutex> lock(mutex_);
    exp_ptr_ = exp_ptr;
    state_ = State::FAILED;
    NotifySubscribers(&lock);
    cv_.notify_all();
}

void Task::NotificationHandler() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (notification_handler_.has_value()) {
        auto handler = notification_handler_.value();
        lock.unlock();
        handler(shared_from_this());
    }
}

void Task::NotifySubscribers(std::unique_lock<std::mutex>* lock) {
    auto subscribers = std::move(subscribers_);
    subscribers_.clear();
    lock->unlock();
    for (auto& subscriber : subscribers) {
        if (auto task = subscriber.lock(); task) {
            if (this != task.get()) {
                task->NotificationHandler();
            }
        }
    }
    lock->lock();
}

bool Task::HasLimitations() {
    std::lock_guard<std::mutex> guard(mutex_);
    return !triggers_.empty() || !dependences_.empty() || time_trigger_.has_value();
}

bool Task::CanExecuted() {
    std::lock_guard<std::mutex> guard(mutex_);
    if (triggers_.empty() && dependences_.empty() && !time_trigger_.has_value()) {
        return true;
    }
    if (!time_trigger_.has_value() || std::chrono::system_clock::now() < time_trigger_.value()) {
        for (auto& trigger : triggers_) {
            if (auto tr = trigger.lock(); tr && tr->IsFinished()) {
                return true;
            }
        }
        for (auto& dependence : dependences_) {
            if (auto dp = dependence.lock(); dp && !dp->IsFinished()) {
                return false;
            }
        }
        return !dependences_.empty();
    }
    return true;
}

std::optional<std::chrono::system_clock::time_point> Task::GetTimeTrigger() {
    std::lock_guard<std::mutex> guard(mutex_);
    return time_trigger_;
}

Executor::Executor(uint32_t num_threads) {
    workers_.reserve(num_threads);
    for (uint32_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this]() { WorkerRunner(); });
    }
}

Executor::~Executor() {
    StartShutdown();
    WaitShutdown();
}

void Executor::Submit(std::shared_ptr<Task> task) {
    std::lock_guard<std::mutex> guard(queue_mutex_);
    if (!is_finished_ && !task->IsFinished()) {
        task->Expect();
        task->SetNotificationHandler([weak_self = weak_from_this()](auto subscribe) {
            if (auto self = weak_self.lock()) {
                self->Submit(subscribe);
            }
        });
        tasks_.emplace_back(std::move(task));
        queue_cv_.notify_one();
    }
}

void Executor::StartShutdown() {
    std::lock_guard<std::mutex> guard(queue_mutex_);
    is_finished_ = true;
    tasks_ = {};
    queue_cv_.notify_all();
}

void Executor::WaitShutdown() {
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void Executor::WorkerRunner() {
    for (;;) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [&]() { return !tasks_.empty() || is_finished_; });
        if (is_finished_) {
            return;
        }
        auto task = std::move(tasks_.front());
        tasks_.pop_front();
        if (!task->CanExecuted()) {
            if (task->GetTimeTrigger().has_value()) {
                tasks_.push_back(std::move(task));
            }
            continue;
        }
        lock.unlock();
        if (task->CaptureRunner()) {
            try {
                task->Run();
                task->Complete();
            } catch (...) {
                task->SetError(std::current_exception());
            }
        }
    }
}

std::shared_ptr<Executor> MakeThreadPoolExecutor(uint32_t num_threads) {
    return std::make_shared<Executor>(num_threads);
}
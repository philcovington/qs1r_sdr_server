#include <condition_variable>
#include <mutex>

class WaitCondition {
  public:
    WaitCondition() = default;

    // Lock the mutex and wait until the condition is notified
    void wait() {
        std::unique_lock<std::mutex> lock(mutex);
        cond_var.wait(lock);
    }

    // Wait with a timeout
    template <class Rep, class Period> bool wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
        std::unique_lock<std::mutex> lock(mutex);
        return cond_var.wait_for(lock, timeout_duration) == std::cv_status::no_timeout;
    }

    // Notify one waiting thread
    void notify_one() {
        std::lock_guard<std::mutex> lock(mutex);
        cond_var.notify_one();
    }

    // Notify all waiting threads
    void notify_all() {
        std::lock_guard<std::mutex> lock(mutex);
        cond_var.notify_all();
    }

  private:
    std::mutex mutex;
    std::condition_variable cond_var;
};

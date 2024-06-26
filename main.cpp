#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <tuple>

class ThreadMonitor {
public:
    void addThread(std::thread&& t, std::shared_ptr<std::atomic<bool>> status) {
        std::lock_guard<std::mutex> lock(mutex_);
        threads.push_back(std::make_shared<std::thread>(std::move(t)));
        thread_statuses.push_back(status);
        last_heartbeats.push_back(std::chrono::steady_clock::now());
    }

    bool monitorThreads() {
        std::lock_guard<std::mutex> lock(mutex_);
        bool any_thread_running = false;
        auto now = std::chrono::steady_clock::now();
        for (size_t i = 0; i < threads.size(); ++i) {
            if (*thread_statuses[i]) {
                if (threads[i]->joinable()) {
                    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeats[i]).count();
                    if (duration > 2) {
                        std::cerr << "Thread " << i << " nao esta respondendo." << std::endl;
                        *thread_statuses[i] = false;
                        return false;
                    }
                    any_thread_running = true;
                }
                else {
                    std::cerr << "Thread " << i << " foi finalizada abruptamente." << std::endl;
                    *thread_statuses[i] = false;
                    return false;
                }
            }
        }
        return any_thread_running;
    }

    void updateHeartbeat(size_t index) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index < last_heartbeats.size()) {
            last_heartbeats[index] = std::chrono::steady_clock::now();
        }
    }

    void stopAllThreads() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& status : thread_statuses) {
            *status = false;
        }
        for (auto& t : threads) {
            if (t->joinable()) {
                t->join();
            }
        }
    }

    size_t getThreadCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return threads.size();
    }

private:
    std::vector<std::shared_ptr<std::thread>> threads;
    std::vector<std::shared_ptr<std::atomic<bool>>> thread_statuses;
    std::vector<std::chrono::steady_clock::time_point> last_heartbeats;
    mutable std::mutex mutex_;
};

ThreadMonitor g_monitor;
std::atomic<bool> g_running{ true };

void printNumber(int num, std::shared_ptr<std::atomic<bool>> status, size_t index) {
    while (g_running && *status) {
        std::cout << "Thread ID: " << std::this_thread::get_id() << " - Num: " << num << std::endl;
        g_monitor.updateHeartbeat(index);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "Thread " << std::this_thread::get_id() << " (printNumber " << num << ") terminou." << std::endl;
}

void printNumber2(std::shared_ptr<std::atomic<bool>> status, size_t index) {
    for (int i = 0; i < 5 && g_running && *status; ++i) {
        std::cout << "Thread ID: " << std::this_thread::get_id() << " - Num: 3 " << std::endl;
        g_monitor.updateHeartbeat(index);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    *status = false;
    std::cout << "Thread " << std::this_thread::get_id() << " (printNumber2) terminou normalmente." << std::endl;
}

template <typename Func, typename... Args>
void addProtectedThread(Func&& func, Args&&... args) {
    auto status = std::make_shared<std::atomic<bool>>(true);
    size_t index = g_monitor.getThreadCount();
    g_monitor.addThread(
        std::thread([func = std::forward<Func>(func), status, index, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            std::invoke(func, std::get<Args>(args)..., status, index);
            }),
        status
    );
}

int main() {
    addProtectedThread(printNumber, 1);
    addProtectedThread(printNumber, 2);
    addProtectedThread(printNumber2);

    while (g_running) {
        if (!g_monitor.monitorThreads()) {
            g_running = false;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    g_monitor.stopAllThreads();
    std::cout << "Encerrando aplicacao." << std::endl;
    return 0;
}

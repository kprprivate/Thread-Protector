# Thread Protector

Thread Protector is a robust C++ library for protecting and monitoring multi-threaded applications. It offers automatic thread management, unresponsive thread detection, and forced termination handling.

## Features

- Automatic thread monitoring
- Heartbeat system to detect unresponsive threads
- Forced thread termination detection
- Safe thread shutdown mechanism

## Requirements

- C++17 or later
- CMake 3.10 or later (for building)

## Building the Project

1. Clone the repository:
   ```
   git clone https://github.com/kprprivate/Thread-Protector.git
   cd thread-protector
   ```

2. Create a build directory:
   ```
   mkdir build && cd build
   ```

3. Run CMake and build:
   ```
   cmake ..
   make
   ```

## Usage

Here's a basic example of how to use Thread Protector:

```cpp
#include "thread_protector.hpp"

void myThreadFunction(int arg, std::shared_ptr<std::atomic<bool>> status, size_t index) {
    while (*status) {
        // Your thread logic here
        g_monitor.updateHeartbeat(index);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    ThreadMonitor monitor;
    addProtectedThread(myThreadFunction, 42);

    // Main thread logic
    while (monitor.monitorThreads()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    monitor.stopAllThreads();
    return 0;
}
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License.
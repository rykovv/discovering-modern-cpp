#include <tuple>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <iostream>

// mailbox

struct task1_box {
    bool done;
    int result;
};

struct task2_box {
    float calibration;
    long long offset;
};

struct task3_box {
    double x;
    double y;
};

template <typename ...Boxes>
struct mailbox {};

template <>
struct mailbox<> {};

template <typename Box1, typename ...Boxes>
struct mailbox<Box1, Boxes...> {
public:
    template <typename Box>
    bool send(Box const& b) {
        using box = std::optional<Box>;
        std::lock_guard<std::mutex> g{m};
        if (std::get<box>(boxes).has_value()) {
            return false;
        } else {
            std::get<box>(boxes) = b;
            cv.notify_one();
            return true;
        }
    }

    // blocking
    template <typename Box>
    Box receive() {
        using box = std::optional<Box>;
        std::unique_lock<std::mutex> wl {m};
        cv.wait(wl, [this] {
            return std::get<box>(boxes).has_value();
        });
        Box b = std::get<box>(boxes).value();
        std::get<box>(boxes) = {};
        return b;
    }

    // non-blocking
    template <typename Box>
    bool receive(Box & b) {
        using box = std::optional<Box>;
        std::lock_guard<std::mutex> g{m};
        if (std::get<box>(boxes).has_value()) {
            b = std::get<box>(boxes).value();
            std::get<box>(boxes) = {};
            return true;
        } else {
            return false;
        }
    }

    template <typename Box>
    bool full() const {
        using box = std::optional<Box>;
        std::lock_guard<std::mutex> g{m};
        return std::get<box>(boxes).has_value();
    }
private:
    std::tuple<std::optional<Box1>, std::optional<Boxes>...> boxes;
    std::condition_variable cv;
    mutable std::mutex m;
};

int main() {
    mailbox<task1_box, task2_box, task3_box> mb{};

    std::cout << mb.full<task1_box>() << std::endl;
    task3_box b3{1.0, 2.0};
    mb.send(b3);
    std::cout << mb.full<task3_box>() << std::endl;
    task3_box b = mb.receive<task3_box>();
    std::cout << b.x << ", " << b.y << std::endl;
    auto full = mb.receive<task3_box>(b);
    std::cout << full << " == 0" << std::endl;
    return mb.full<task3_box>();
}

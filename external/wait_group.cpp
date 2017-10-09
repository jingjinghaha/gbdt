//this is derived from elf
#include "wait_group.h"
#include <glog/logging.h>

namespace bdl {
namespace luna {

WaitGroup::WaitGroup() : _count(0) {
}

WaitGroup::WaitGroup(int32_t num) : _count(num) {
}

void WaitGroup::add(int n) {
    std::unique_lock<std::mutex> lock(_mutex);
    CHECK(n >= 0) << "input paramter n: " << n << " must be >= 0";
    _count += n;
}

void WaitGroup::done() {
    done(1);
}

void WaitGroup::done(int delta) {
    std::unique_lock<std::mutex> lock(_mutex);
    _count -= delta;
    CHECK(_count >= 0) << "done calling times must not be more than add calling times";

    if (_count == 0) {
        _cond.notify_all();
    }
}

void WaitGroup::wait() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [&]() {
        return _count == 0;
    });
}

int WaitGroup::get_count() {
    std::unique_lock<std::mutex> lock(_mutex);
    return _count;
}

} // namespace luna
} // namespace bdl

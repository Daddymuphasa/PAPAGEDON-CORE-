#pragma once

namespace papagedon {

class DoubleTap final {
public:
    bool Press(double now) noexcept {
        const bool twice = last_ >= 0.0 && now >= last_ && now - last_ <= 0.45;
        last_ = twice ? -1.0 : now;
        return twice;
    }
private:
    double last_ = -1.0;
};

} // namespace papagedon

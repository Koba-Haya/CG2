#pragma once
#include <cstdint>
#include <utility>

class SrvAllocator;

class SrvHandle {
public:
    SrvHandle() = default;

    SrvHandle(SrvAllocator* allocator, uint32_t index)
        : allocator_(allocator), index_(index), valid_(true) {
    }

    ~SrvHandle() { Reset(); }

    SrvHandle(const SrvHandle&) = delete;
    SrvHandle& operator=(const SrvHandle&) = delete;

    SrvHandle(SrvHandle&& other) noexcept { MoveFrom(std::move(other)); }
    SrvHandle& operator=(SrvHandle&& other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    void Reset();

    bool IsValid() const { return valid_; }
    uint32_t Index() const { return index_; }

private:
    void MoveFrom(SrvHandle&& other) {
        allocator_ = other.allocator_;
        index_ = other.index_;
        valid_ = other.valid_;
        other.allocator_ = nullptr;
        other.index_ = 0;
        other.valid_ = false;
    }

    SrvAllocator* allocator_ = nullptr;
    uint32_t index_ = 0;
    bool valid_ = false;
};

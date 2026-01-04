#include "SrvHandle.h"
#include "SrvAllocator.h"

void SrvHandle::Reset() {
    if (!valid_) {
        return;
    }
    if (allocator_) {
        allocator_->Free(index_);
    }
    allocator_ = nullptr;
    index_ = 0;
    valid_ = false;
}

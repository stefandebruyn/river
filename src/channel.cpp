#include <cassert>
#include <cstring>

#include "channel.hpp"

namespace river {
void ChannelBase::serialize(void* const dest) const
{
    assert(dest);

    // Do nothing if not linked to a river.
    if (!linked()) {
        return;
    }

    // Acquire lock if there is one.
    if (link->lock) {
        link->lock->acquire();
    }

    // Copy data from channel to dest.
    const void* const src = link->river->storage->data() + link->channel_offset;
    std::memcpy(dest, src, size());

    // Release lock if there is one.
    if (link->lock) {
        link->lock->release();
    }
}

void ChannelBase::deserialize(const void* const src)
{
    assert(src);

    // Do nothing if not linked to a river.
    if (!linked()) {
        return;
    }

    // Acquire lock if there is one.
    if (link->lock) {
        link->lock->acquire();
    }

    // Copy data from src to channel.
    void* const dest = link->river->storage->data() + link->channel_offset;
    std::memcpy(dest, src, size());

    // Release lock if there is one.
    if (link->lock) {
        link->lock->release();
    }
}
} /* namespace river */

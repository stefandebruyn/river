#include "rivulet.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

namespace river {
void Rivulet::read(void* const dest) const
{
    // Do nothing if dest is null or not linked to a river.
    if (!dest || !linked()) {
        return;
    }

    // Acquire lock if there is one.
    if (link->lock) {
        link->lock->acquire();
    }

    // Copy data from rivulet to dest.
    const void* const src = link->river->storage->data() + link->rivulet_offset;
    std::memcpy(dest, src, link->rivulet_size);

    // Release lock if there is one.
    if (link->lock) {
        link->lock->release();
    }
}

void Rivulet::write(const void* const src)
{
    // Do nothing if src is null or not linked to a river.
    if (!src || !linked()) {
        return;
    }

    // Acquire lock if there is one.
    if (link->lock) {
        link->lock->acquire();
    }

    // Copy data from src to rivulet.
    void* const dest = link->river->storage->data() + link->rivulet_offset;
    std::memcpy(dest, src, link->rivulet_size);

    // Release lock if there is one.
    if (link->lock) {
        link->lock->release();
    }
}

size_t Rivulet::size() const
{
    if (!linked()) {
        return 0;
    }

    return link->rivulet_size;
}
} /* namespace river */

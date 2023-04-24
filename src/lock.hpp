#ifndef RIVER_LOCK_HPP
#define RIVER_LOCK_HPP

namespace river {
/**
 * Interface for a lock.
 */
class Lock {
public:
    /**
     * Acquires the lock.
     */
    virtual void acquire() = 0;

    /**
     * Releases the lock.
     */
    virtual void release() = 0;
};
} /* namespace river */

#endif

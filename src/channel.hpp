#ifndef RIVER_CHANNEL_HPP
#define RIVER_CHANNEL_HPP

#include <memory>

#include "link.hpp"
#include "lock.hpp"
#include "river.hpp"

namespace river {
/**
 * Base class for Channel<T> instantiations.
 */
class ChannelBase : public Linkable {
protected:
    /**
     * Destructor.
     */
    virtual ~ChannelBase() = default;

    /**
     * @see Channel<T>::size()
     */
    virtual size_t size() const = 0;

    /**
     * Reads from the channel backing memory.
     *
     * This will copy exactly Channel<T>::size() bytes to dest.
     *
     * @param dest Read destination.
     */
    virtual void serialize(void* const dest) const final;

    /**
     * Writes to the channel backing memory.
     *
     * This will copy exactly Channel<T>::size() bytes from src.
     *
     * @param src Write source.
     */
    virtual void deserialize(const void* const src) final;
};

template <typename T>
/**
 * Handle to a river channel.
 *
 * @see Builder
 */
class Channel final : public ChannelBase {
public:
    /**
     * Gets the value of the channel.
     *
     * This returns 0 if the river is not built.
     *
     * @returns Channel value.
     */
    T get() const
    {
        T val = T();
        serialize(&val);
        return val;
    }

    /**
     * Sets the value of the channel.
     *
     * This has no effect if the river is not built.
     *
     * @param val New channel value.
     */
    void set(const T val)
    {
        deserialize(&val);
    }

    /**
     * Gets the size of the channel type in bytes.
     *
     * @returns Channel type size in bytes.
     */
    size_t size() const final override
    {
        return sizeof(T);
    }
};
} /* namespace river */

#endif

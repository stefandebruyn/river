#ifndef RIVER_LINK_HPP
#define RIVER_LINK_HPP

#include <memory>

#include "lock.hpp"
#include "river.hpp"

namespace river {
/**
 * Metadata about a memory region within a river.
 */
struct Link final {
    /**
     * Linked river.
     *
     * This is null if the river is not built.
     */
    std::shared_ptr<River> river;

    /**
     * Byte offset of the channel in the river backing memory.
     *
     * This is undefined if the link is not linking a channel or the river is
     * not built.
     */
    size_t channel_offset;

    /**
     * Byte offset of the rivulet in the river backing memory.
     *
     * This is undefined if the link is not linking a rivulet or the river is
     * not built.
     */
    size_t rivulet_offset;

    /**
     * Size of the rivulet in bytes.
     *
     * This is undefined if the link is not linking a rivulet or the river is
     * not built.
     */
    size_t rivulet_size;

    /**
     * Lock protecting the linked memory of the river.
     *
     * This is null if the linked memory is unlocked or the river is not built.
     */
    std::shared_ptr<Lock> lock;
};

/**
 * A handle that is linked to river memory.
 */
class Linkable {
public:
    /**
     * Gets whether the handle is linked.
     *
     * A handle is linked once the river is built and the link is initialized.
     * Only at this point can the link be used to access river backing memory.
     *
     * @returns Whether handle is linked.
     */
    virtual bool linked() const final;

protected:
    /**
     * Befriend Builder so that it can set the link.
     */
    friend class Builder;

    /**
     * River link.
     */
    std::shared_ptr<Link> link;
};
} /* namespace river */

#endif

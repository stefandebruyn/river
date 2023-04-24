#ifndef RIVER_RIVER_HPP
#define RIVER_RIVER_HPP

#include <cstdint>
#include <memory>
#include <vector>

namespace river {
/**
 * River backing memory.
 *
 * @see Builder
 */
class River final {
public:
    /**
     * Constructor.
     *
     * By default, the river is empty.
     */
    River();

private:
    /**
     * Befriend Builder, ChannelBase, and Rivulet so that they can access the
     * river backing memory.
     * @{
     */
    friend class Builder;
    friend class ChannelBase;
    friend class Rivulet;
    /**
     * @}
     */

    /**
     * River backing memory.
     */
    std::shared_ptr<std::vector<uint8_t>> storage;
};
} /* namespace river */

#endif

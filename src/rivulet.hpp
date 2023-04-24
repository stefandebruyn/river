#ifndef RIVER_RIVULET_HPP
#define RIVER_RIVULET_HPP

#include "link.hpp"

namespace river {
/**
 * Handle to a rivulet.
 *
 * @see Builder
 */
class Rivulet final : public Linkable {
public:
    /**
     * Reads the rivulet memory.
     *
     * This will copy exactly Rivulet::size() bytes to dest.
     *
     * @param dest Read destination.
     */
    void read(void* const dest) const;

    /**
     * Writes the rivulet memory.
     *
     * This will copy exactly Rivulet::size() bytes from src.
     *
     * @param dest Read destination.
     */
    void write(const void* const src);

    /**
     * Gets the size of the rivulet in bytes.
     *
     * @returns Rivulet size in bytes.
     */
    size_t size() const;
};
} /* namespace river */

#endif

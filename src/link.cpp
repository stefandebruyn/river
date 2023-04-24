#include "link.hpp"

namespace river {
bool Linkable::linked() const
{
    return (link && link->river);
}
} /* namespace river */

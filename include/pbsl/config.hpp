#ifndef PBSL_CONFIG_H
#define PBSL_CONFIG_H

#include <string>

namespace pbsl::config {

using Key = uint32_t;

inline double constexpr p = 0.5;

inline bool constexpr DebugEnabled() { return true; }

namespace pbsl {
struct Node;

class SkipList;
}

}

#endif //PBSL_CONFIG_H
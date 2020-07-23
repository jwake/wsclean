#pragma once
#include <cstddef>

namespace aocommon {
    template<typename WorkFunctor, typename RecoveryFunctor>
    bool retry(std::size_t maxTries, WorkFunctor work, RecoveryFunctor recovery) {
        for (std::size_t i = 0; i < maxTries; i++) {
            if (!work()) {
                recovery();
            } else { 
                return true;
            }
        }

        return false;
    }
}
#pragma once
#include <cstddef>
#include <thread>

namespace aocommon {
    /*
     *  Retry an operation up to maxTries times, with a recovery method. Useful for file I/O that may occasionally fail.
    */
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

    /*
     * Retry an operation up to maxTries times, waiting two seconds in between.
     */
    template<typename WorkFunctor>
    bool retry(std::size_t maxTries, WorkFunctor work) {
        return retry(maxTries, work, [] { std::this_thread::sleep_for(std::chrono::seconds(2)); });
    }
}
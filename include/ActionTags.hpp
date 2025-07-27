#pragma once

#include "include/PegasusTypes.hpp"

namespace pegasus
{
    namespace ActionTags
    {
        // Core Actions
        extern const ActionTagType FETCH_TAG;
        extern const ActionTagType INST_TRANSLATE_TAG;
        extern const ActionTagType DECODE_TAG;
        extern const ActionTagType EXECUTE_TAG;
        extern const ActionTagType COMPUTE_ADDR_TAG;
        extern const ActionTagType DATA_TRANSLATE_TAG;
        extern const ActionTagType EXCEPTION_TAG;

        // Stop Simulation
        extern const ActionTagType STOP_SIM_TAG;
    };
} // namespace pegasus

#pragma once

#include "include/PegasusTypes.hpp"

namespace pegasus
{
    namespace ActionTags
    {
        // Core Actions
        extern const ActionTagType FETCH_TAG;
        extern const ActionTagType INST_S_STAGE_TRANSLATE_TAG;
        extern const ActionTagType INST_VS_STAGE_TRANSLATE_TAG;
        extern const ActionTagType INST_G_STAGE_TRANSLATE_TAG;
        extern const ActionTagType DECODE_TAG;
        extern const ActionTagType EXECUTE_TAG;
        extern const ActionTagType COMPUTE_ADDR_TAG;
        extern const ActionTagType DATA_TRANSLATE_TAG;
        extern const ActionTagType TRANSLATION_PAGE_EXECUTE;
        extern const ActionTagType EXCEPTION_TAG;
        extern const ActionTagType DATA_S_STAGE_TRANSLATE_TAG;
        extern const ActionTagType DATA_VS_STAGE_TRANSLATE_TAG;
        extern const ActionTagType DATA_G_STAGE_TRANSLATE_TAG;

        // Stop Simulation
        extern const ActionTagType STOP_SIM_TAG;
    };
} // namespace pegasus

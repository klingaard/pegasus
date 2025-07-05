#pragma once

#include <cinttypes>

#include "core/ActionGroup.hpp"

namespace atlas
{
    class TranslatedPage
    {
    public:
        TranslatedPage(uint32_t block_size, uint64_t addr_mask,
                       ActionGroup * decode_action_group) :
            decode_block_(block_size, decode_action_group),
            addr_mask_(addr_mask),
            offset_mask_(~addr_mask)
        {
            translated_page_group_.createAction(
                atlas::Action::createAction<&TranslatedPage::translated_page_execute_>(this,
                                                                                      "TranslatedPageExecute",
                                                                                      ActionTags::TRANSLATION_PAGE_EXECUTE));
            decode_block_.resize(block_size);
        }

        ActionGroup * getTranslatedPageActionGroup() { return &translated_page_group_; }

    private:
        // Main entry for this translated page
        ActionGroup translated_page_group_{"TranslatedPage"};
        Action::ItrType translated_page_execute_(AtlasState* state,
                                                 Action::ItrType action_it);

        ////////////////////////////////////////////////////////////////////////////////
        // Individual instruction execution in the decode block
        class InstExecute
        {
        public:
            InstExecute()
            {
                inst_execute_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setup_inst_>(this,
                                                                           "TranslatedPageSetupInst"));
            }

            ActionGroup * getInstActionGroup() { return &inst_execute_group_; }

        private:

            // Need to decode the instruction at the offset
            Action::ItrType setup_inst_(AtlasState* state,
                                        Action::ItrType action_it);

            ActionGroup  inst_execute_group_;
            AtlasInstPtr inst_;
        };

        // Eventually this vector will be populated with just pure
        // instruction execution action groups
        std::vector<InstExecute> decode_block_;

        const uint64_t addr_mask_;
        const uint64_t offset_mask_;

        ActionGroup *decode_action_group_ = nullptr;

    };
}

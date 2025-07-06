#pragma once

#include <cinttypes>

#include "core/ActionGroup.hpp"

namespace atlas
{
    class TranslatedPage
    {
    public:
        TranslatedPage(uint32_t block_size,
                       uint64_t addr_mask,
                       ActionGroup * fetch_action_group,
                       ActionGroup * decode_action_group) :
            translated_page_group_("TranslatedPageGroup",
                                   atlas::Action::createAction<&TranslatedPage::translated_page_execute_>(this,
                                                                                                          "TranslatedPageExecute",
                                                                                                          ActionTags::TRANSLATION_PAGE_EXECUTE)),
            decode_block_(block_size, {decode_action_group, &translated_page_group_}),
            fetch_action_group_(fetch_action_group),
            addr_mask_(addr_mask),
            offset_mask_(~addr_mask)
        {
        }

        ActionGroup * getTranslatedPageActionGroup() { return &translated_page_group_; }

    private:
        // Main entry for this translated page
        ActionGroup translated_page_group_;
        Action::ItrType translated_page_execute_(AtlasState* state,
                                                 Action::ItrType action_it);

        ////////////////////////////////////////////////////////////////////////////////
        // Individual instruction execution in the decode block
        class InstExecute
        {
        public:
            InstExecute(ActionGroup * decode_action_group,
                        ActionGroup * translated_page_group) :
                decode_action_group_(decode_action_group),
                translated_page_group_(translated_page_group)
            {
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setup_inst_>(this,
                                                                           "TranslatedPageSetupInst"));
            }

            ActionGroup * getInstActionGroup() { return inst_action_group_; }

        private:

            // Need to decode the instruction at the offset
            Action::ItrType setup_inst_(AtlasState* state,
                                        Action::ItrType action_it);

            ActionGroup * decode_action_group_ = nullptr;
            ActionGroup * translated_page_group_ = nullptr;
            ActionGroup   inst_setup_group_;
            ActionGroup * inst_action_group_ = &inst_setup_group_;
            AtlasInstPtr inst_;
        };

        // Eventually this vector will be populated with just pure
        // instruction execution action groups
        std::vector<InstExecute> decode_block_;
        ActionGroup * fetch_action_group_ = nullptr;

        const uint64_t addr_mask_;
        const uint64_t offset_mask_;

        ActionGroup *decode_action_group_ = nullptr;

    };
}

#pragma once

#include <cinttypes>
#include <vector>
#include <map>

#include "core/ActionGroup.hpp"
#include "core/AtlasInst.hpp"

namespace atlas
{
    namespace ActionTags
    {
        extern const ActionTagType TRANSLATION_PAGE_EXECUTE;
    }

    class TranslatedPage
    {
    public:

        using base_type = TranslatedPage;

        TranslatedPage(const AtlasTranslationState::TranslationResult & translation_result,
                       ActionGroup * fetch_action_group,
                       ActionGroup * execute_action_group) :
            translated_page_group_("TranslatedPageGroup",
                                   atlas::Action::createAction<
                                   &TranslatedPage::translatedPageExecute_>(this,
                                                                            "TranslatedPageExecute",
                                                                            ActionTags::TRANSLATION_PAGE_EXECUTE)),
            fetch_action_group_(fetch_action_group),
            execute_action_group_(execute_action_group),
            translation_result_(translation_result)
        {
        }

        ActionGroup * getTranslatedPageActionGroup() { return &translated_page_group_; }

    private:
        // Main entry for this translated page
        ActionGroup translated_page_group_;
        Action::ItrType translatedPageExecute_(AtlasState* state,
                                               Action::ItrType action_it);

        ////////////////////////////////////////////////////////////////////////////////
        // Individual instruction execution in the decode block
        class InstExecute
        {
        public:

            using base_type = InstExecute;

            InstExecute(ActionGroup * translated_page_group,
                        ActionGroup * execute_page_group) :
                translated_page_group_(translated_page_group),
                execute_action_group_(execute_page_group)
            {
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setupInst_>(this,
                                                                          "TranslatedPageSetupInst"));
            }

            InstExecute(InstExecute&& orig) :
                translated_page_group_(std::move(orig.translated_page_group_)),
                execute_action_group_(std::move(orig.execute_action_group_))
            {
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setupInst_>(this,
                                                                          "TranslatedPageSetupInst"));
            }

            InstExecute(const InstExecute & orig) :
                translated_page_group_(orig.translated_page_group_),
                execute_action_group_(orig.execute_action_group_)
            {
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setupInst_>(this,
                                                                          "TranslatedPageSetupInst"));
            }

            const InstExecute& operator=(const InstExecute & orig)
            {
                translated_page_group_ = orig.translated_page_group_;
                execute_action_group_ = orig.execute_action_group_;
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setupInst_>(this,
                                                                          "TranslatedPageSetupInst"));
                return *this;
            }

            ActionGroup * getInstActionGroup() { return inst_action_group_; }

            void setInstAddress(Addr inst_addr) { inst_addr_ = inst_addr; }

        private:

            // Need to decode the instruction at the offset
            Action::ItrType setupInst_(AtlasState* state,
                                       Action::ItrType action_it);

            // Set the inst pointer in AtlasState when the instruction
            // is to be executed again
            Action::ItrType setInst_(AtlasState* state, Action::ItrType action_it);

            ActionGroup * translated_page_group_ = nullptr;
            ActionGroup   inst_setup_group_{"InstSetupGroup"};
            Action        inst_set_inst_{Action::createAction<&InstExecute::setInst_>(this, "InstSetupGroup")};
            ActionGroup * execute_action_group_ = nullptr;
            ActionGroup * inst_action_group_ = &inst_setup_group_;
            Addr inst_addr_ = 0;
            AtlasInstPtr inst_;
        };

        // Eventually this vector will be populated with just pure
        // instruction execution action groups
        using InstExecuteBlock = std::vector<InstExecute>;
        std::unordered_map<Addr, InstExecuteBlock> decode_block_;
        ActionGroup * fetch_action_group_ = nullptr;
        ActionGroup * execute_action_group_ = nullptr;

        // To prevent a construction EVERY TIME the map is queried,
        // cache a copyable block
        const InstExecuteBlock default_block_{2048, InstExecute(&translated_page_group_,
                                                                execute_action_group_)};

        const AtlasTranslationState::TranslationResult translation_result_;
    };
}

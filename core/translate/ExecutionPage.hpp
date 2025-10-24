#pragma once

#include <cinttypes>
#include <vector>
#include <map>

#include "core/ActionGroup.hpp"
#include "core/PegasusInst.hpp"

namespace pegasus
{
    namespace ActionTags
    {
        extern const ActionTagType TRANSLATION_PAGE_EXECUTE;
    }

    /*!
     * \class ExecutionPage
     *
     * This class holds 1 4k group of executable instructions.  The
     * ExecutionCache hold 1..* ExecutionPage objects.
     *
     */
    class ExecutionPage
    {
    public:

        using base_type = ExecutionPage;

        ExecutionPage(const PegasusTranslationState::TranslationResult & translation_result,
                       ActionGroup * fetch_action_group,
                       ActionGroup * execute_action_group) :
            translated_page_group_("ExecutionPageGroup",
                                   pegasus::Action::createAction<
                                   &ExecutionPage::translatedPageExecute_>(this,
                                                                            "ExecutionPageExecute",
                                                                            ActionTags::TRANSLATION_PAGE_EXECUTE)),
            fetch_action_group_(fetch_action_group),
            execute_action_group_(execute_action_group),
            default_block_(2048, InstExecute(&translated_page_group_, execute_action_group_)),
            translation_result_(translation_result)
        {
            // Get the inst execute block at the end of the block.
            // This instruction execute class represents a potential
            // page crosser
            auto & last_inst_exe = default_block_.back();

            // Replace with a different instruction execution instance
            last_inst_exe = InstExecute(&translated_page_group_,
                                        execute_action_group_, true);
        }

        ActionGroup * getExecutionPageActionGroup() { return &translated_page_group_; }

    private:
        // Main entry for this translated page
        ActionGroup translated_page_group_;
        Action::ItrType translatedPageExecute_(PegasusState* state,
                                               Action::ItrType action_it);

        ////////////////////////////////////////////////////////////////////////////////
        // Individual instruction execution in the decode block
        class InstExecute
        {
        public:

            using base_type = InstExecute;

            InstExecute(ActionGroup * translated_page_group,
                        ActionGroup * execute_page_group,
                        bool last_entry = false) :
                translated_page_group_(translated_page_group),
                execute_action_group_(execute_page_group),
                last_entry_(last_entry)
            {
                inst_setup_group_.addAction(
                    pegasus::Action::createAction<&InstExecute::setupInst_>(this,
                                                                            "ExecutionPageSetupInst"));
            }

            InstExecute(InstExecute&& orig) :
                translated_page_group_(std::move(orig.translated_page_group_)),
                execute_action_group_(std::move(orig.execute_action_group_)),
                last_entry_(orig.last_entry_)
            {
                inst_setup_group_.addAction(
                    pegasus::Action::createAction<&InstExecute::setupInst_>(this,
                                                                            "ExecutionPageSetupInst"));
            }

            InstExecute(const InstExecute & orig) :
                translated_page_group_(orig.translated_page_group_),
                execute_action_group_(orig.execute_action_group_),
                last_entry_(orig.last_entry_)
            {
                inst_setup_group_.addAction(
                    pegasus::Action::createAction<&InstExecute::setupInst_>(this,
                                                                            "ExecutionPageSetupInst"));
            }

            const InstExecute& operator=(const InstExecute & orig)
            {
                translated_page_group_ = orig.translated_page_group_;
                execute_action_group_ = orig.execute_action_group_;
                last_entry_ = orig.last_entry_;
                inst_setup_group_.addAction(
                    pegasus::Action::createAction<&InstExecute::setupInst_>(this,
                                                                            "ExecutionPageSetupInst"));
                return *this;
            }

            ActionGroup * getInstActionGroup() { return inst_action_group_; }

            void setInstAddress(Addr inst_addr) { inst_addr_ = inst_addr; }

        private:

            // Need to decode the instruction at the offset
            Action::ItrType setupInst_(PegasusState* state,
                                       Action::ItrType action_it);

            // Set the inst pointer in PegasusState when the instruction
            // is to be executed again
            Action::ItrType setInst_(PegasusState* state, Action::ItrType action_it);

            ActionGroup * translated_page_group_ = nullptr;
            ActionGroup   inst_setup_group_{"InstSetupGroup"};
            Action        inst_set_inst_{Action::createAction<&InstExecute::setInst_>(this, "InstSetupGroup")};
            ActionGroup * execute_action_group_ = nullptr;
            ActionGroup * inst_action_group_ = &inst_setup_group_;
            Addr inst_addr_ = 0;
            PegasusInstPtr inst_;
            bool last_entry_ = false;
        };

        ActionGroup * fetch_action_group_ = nullptr;
        ActionGroup * execute_action_group_ = nullptr;

        using InstExecuteBlock = std::vector<InstExecute>;
        std::unordered_map<Addr, InstExecuteBlock> decode_block_;

        // To prevent a construction EVERY TIME the map is queried,
        // cache a copy-able block
        InstExecuteBlock default_block_;

        const PegasusTranslationState::TranslationResult translation_result_;
    };
}

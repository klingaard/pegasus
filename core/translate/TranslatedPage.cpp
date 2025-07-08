#include "TranslatedPage.hpp"
#include "core/AtlasState.hpp"

namespace atlas
{
    Action::ItrType TranslatedPage::translatedPageExecute_(AtlasState* state,
                                                           Action::ItrType action_it)
    {
        // Check to see if we're still on the same page
        const auto vaddr = state->getPc();
        if ((addr_mask_ & vaddr) == vaddr)
        {
            const auto offset = vaddr & offset_mask_;

            // If this instruction was never fetched/decoded, the
            // instruction group being called is the Decode action
            // group.  This group returns the execution group
            translated_page_group_.
                setNextActionGroup(decode_block_.at(offset).getInstActionGroup());
        }
        else {
            // Go back to fetch
            translated_page_group_.setNextActionGroup(fetch_action_group_);
        }

        // The translated page cannot continue.
        return ++action_it;
    }

    // Need to decode the instruction at the offset
    Action::ItrType TranslatedPage::InstExecute::setupInst_(AtlasState* state,
                                                            Action::ItrType action_it)
    {
        // Decode the instruction at the given PC (in AtlasState)
        auto execute_group = decode_action_group_->execute(state);
        sparta_assert(execute_group->hasTag(ActionTags::EXECUTE_TAG));

        // Set up the execution of the instruction and reset the
        // instruction execute group to the instruction that was just
        // setup
        inst_action_group_ = execute_group->execute(state);

        // Setup the instruction's next action to return to this
        // translated page
        inst_action_group_->setNextActionGroup(translated_page_group_);

        // Capture the instruction late -- the above execute functions
        // can throw
        inst_ = state->getCurrentInst();

        // Go to end...
        return ++action_it;
    }
}

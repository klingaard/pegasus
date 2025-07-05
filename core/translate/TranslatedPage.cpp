#include "TranslatedPage.hpp"

namespace atlas
{
    Action::ItrType TranslatedPage::translated_page_execute_(AtlasState* state,
                                                             Action::ItrType action_it)
    {
        // Check to see if we're still on the same page
        const XLEN vaddr = state->getPc();
        if ((addr_mask_ & vaddr) == addr_mask_)
        {
            const auto offset = vaddr & ~offset_mask_;

            // If this instruction was never fetched/decoded, the
            // instruction group being called is the Decode action
            // group.  This group returns the execution group
            translated_page_group_.
                setNextActionGroup(decode_block_[offset].getInstActionGroup());
        }
        else {
            // Go back to fetch
            translated_page_group_.setNextActionGroup(&fetch_action_group_);
        }

        // The translated page cannot continue.
        return ++action_it;
    }

    // Need to decode the instruction at the offset
    Action::ItrType TranslatedPage::InstExecute::setup_inst_(AtlasState* state,
                                                             Action::ItrType action_it);
    {
        // Decode the instruction at the given PC (in AtlasState)
        auto execute_group = decode_group->execute();
        sparta_assert(execute_group->hasTag(ActionTags::EXECUTE_TAG));
        inst_ = state->getCurrentInst();

        // Set up the execution of the instruction and reset the
        // instruction execute group to the instruction that was just
        // setup
        inst_execute_group_ = execute_group->execute();

        // Go to end...
        return ++action_it;
    }
}

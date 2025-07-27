#include "core/Fetch.hpp"
#include "core/PegasusAllocatorWrapper.hpp"
#include "core/PegasusState.hpp"
#include "core/Execute.hpp"
#include "core/translate/Translate.hpp"
#include "include/ActionTags.hpp"

#include "sparta/events/StartupEvent.hpp"
#include "sparta/simulation/ResourceTreeNode.hpp"
#include "sparta/utils/LogUtils.hpp"

namespace pegasus
{
    Fetch::Fetch(sparta::TreeNode* fetch_node, const FetchParameters*) : sparta::Unit(fetch_node)
    {
        Action fetch_action =
            pegasus::Action::createAction<&Fetch::fetch_>(this, "fetch", ActionTags::FETCH_TAG);
        fetch_action_group_.addAction(fetch_action);

        sparta::StartupEvent(fetch_node, CREATE_SPARTA_HANDLER(Fetch, advanceSim_));
    }

    void Fetch::onBindTreeEarly_()
    {
        auto core_tn = getContainer()->getParentAs<sparta::ResourceTreeNode>();
        state_ = core_tn->getResourceAs<PegasusState>();

        // Connect Fetch, Translate and Execute
        Translate* translate_unit = core_tn->getChild("translate")->getResourceAs<Translate*>();
        Execute* execute_unit = core_tn->getChild("execute")->getResourceAs<Execute*>();

        ActionGroup* inst_translate_action_group = translate_unit->getInstTranslateActionGroup();
        ActionGroup* execute_action_group = execute_unit->getActionGroup();

        fetch_action_group_.setNextActionGroup(inst_translate_action_group);
        inst_translate_action_group->setNextActionGroup(execute_action_group);
        execute_action_group->setNextActionGroup(&fetch_action_group_);
    }

    Action::ItrType Fetch::fetch_(PegasusState* state, Action::ItrType action_it)
    {
        ILOG("Fetching PC 0x" << std::hex << state->getPc());

        // Reset the sim state
        PegasusState::SimState* sim_state = state->getSimState();
        sim_state->reset();

        PegasusTranslationState* translation_state = state->getFetchTranslationState();
        translation_state->reset();
        translation_state->makeRequest(state->getPc(), sizeof(Opcode));

        // Keep going
        return ++action_it;
    }

    void Fetch::advanceSim_()
    {
        // Run
        ActionGroup* next_action_group = &fetch_action_group_;
        while (next_action_group)
        {
            next_action_group = next_action_group->execute(state_);
        }
        // End of sim
    }
} // namespace pegasus

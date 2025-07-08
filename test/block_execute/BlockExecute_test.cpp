
#include "sim/AtlasSim.hpp"
#include "core/AtlasState.hpp"
#include "include/AtlasTypes.hpp"
#include "core/translate/TranslatedPage.hpp"

#include <cinttypes>

int main()
{
    sparta::Scheduler scheduler;

    atlas::AtlasSim atlas_sim(&scheduler, {}, {}, 1);

    atlas_sim.buildTree();
    atlas_sim.configureTree();
    atlas_sim.finalizeTree();

    auto state = atlas_sim.getAtlasState();
    auto fetch_unit = state->getFetchUnit();
    auto fetch_action_group = fetch_unit->getFetchActionGroup();

    constexpr atlas::Addr virt_page_address = 0xccc0000000;
    constexpr atlas::Addr phys_page_address = 0xfff0000000;

    // Create a simple translation page
    state->setPc(virt_page_address);

    state->writeMemory(phys_page_address, 0x33ul);


    auto * tstate = state->getFetchTranslationState();
    tstate->makeRequest(virt_page_address, 4);
    tstate->setResult(virt_page_address, phys_page_address, 4, atlas::PageSize::SIZE_4K);

    atlas::TranslatedPage translation_page(tstate->getResult(),
                                           fetch_action_group,
                                           fetch_unit->getDecodeActionGroup());

    fetch_action_group->setNextActionGroup(translation_page.getTranslatedPageActionGroup());

    atlas::ActionGroup* next_action_group = fetch_unit->getFetchActionGroup();
    uint32_t i = 0;
    do {
        next_action_group = next_action_group->execute(state);
    } while (++i < 100000000);

    std::cout << "Executed: " << state->getSimState()->inst_count << std::endl;

    return 0;
}

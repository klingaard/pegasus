
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

    auto execute_unit = state->getExecuteUnit();
    auto execute_action_group = execute_unit->getActionGroup();


    ////////////////////////////////////////////////////////////////////////////////
    // Simple test -- for loop counter: one add + branch to add then stop

    // Create a simple translation page
    atlas::Addr virt_page_address = 0x00000000C0000000;
    atlas::Addr phys_page_address = 0x0000000080000000;
    state->setPc(virt_page_address);

    // Write this program:
    // 0x0000000080000000
    //     009890b7          	lui	ra,0x989
    //     6800809b          	addiw	ra,ra,1664 # 989680 <main-0x7f676980>
    //     4105              	li	sp,1
    state->writeMemory(phys_page_address,       0x009890b7u);
    state->writeMemory(phys_page_address + 0x4, 0x6800809bu);
    state->writeMemory(phys_page_address + 0x8, 0x4105u);

    // 000000008000000a <loop>:
    //     0105               	addi	sp,sp,1
    //     fe111fe3          	bne	sp,ra,8000000a <loop>
    state->writeMemory(phys_page_address + 0xa, 0x0105u);
    state->writeMemory(phys_page_address + 0xc, 0xfe111fe3u);

    auto * tstate = state->getFetchTranslationState();
    tstate->makeRequest(virt_page_address, 4);
    tstate->setResult(virt_page_address, phys_page_address, 4, atlas::PageSize::SIZE_4K);

    atlas::TranslatedPage translation_page(tstate->getResult(),
                                           fetch_action_group,
                                           execute_action_group);

    fetch_action_group->setNextActionGroup(translation_page.getTranslatedPageActionGroup());

    atlas::ActionGroup* next_action_group = fetch_unit->getFetchActionGroup();
    uint32_t break_out = 10000000;
    do {
        next_action_group = next_action_group->execute(state);
        if (state->getSimState()->inst_count == break_out) {
            std::cout << std::dec << "Hit " << break_out
                      << " cnt: " << state->getSimState()->inst_count
                      << " addr: " << std::hex << state->getPc()
                      << " sp: "   << std::dec << state->getIntRegister(2)->read<uint64_t>()
                      << std::endl;
            break_out += 10000000;
         }
    } while (state->getPc() != 0x00000000c0000010ull);

    std::cout << "Executed: " << state->getSimState()->inst_count << std::endl;

    ////////////////////////////////////////////////////////////////////////////////
    // Cross a 4M page with a 32-bit opcode that crosses a 4k page
    // VA -> PA mapping:
    //    0xFFFF_FFFF_FFFF_0040_0000 -> 0x0000_0000_0000_0840_0000

    virt_page_address = 0xFFFFFFFFFFFF0000000ull | atlas::pageSize(atlas::Pagesize::SIZE_4M);
    phys_page_address = 0x0000000000008000000ull | atlas::pageSize(atlas::Pagesize::SIZE_4M);
    state->setPc(virt_page_address);

    auto * tstate = state->getFetchTranslationState();
    tstate->makeRequest(virt_page_address, 4);
    tstate->setResult(virt_page_address, phys_page_address, 4, atlas::PageSize::SIZE_4M);

    // Write the same program but shifted to cross a page:
    // 0x0000000000008400ff2:
    //     009890b7          	lui	ra,0x989
    //     6800809b          	addiw	ra,ra,1664 # 989680 <main-0x7f676980>
    //     4105              	li	sp,1
    // 0x0000000000008400ffc <loop>:
    //     0105               	addi	sp,sp,1
    const auto page_edge = 0xff2
    state->writeMemory(phys_page_address + page_edge,       0x009890b7u);
    state->writeMemory(phys_page_address + page_edge + 0x4, 0x6800809bu);
    state->writeMemory(phys_page_address + page_edge + 0x8, 0x41050105u); // li + addi

    // Have the branch cross the page
    // 0x0000000000008400ffe:
    //     fe111fe3          	bne	sp,ra,8000000a <loop>
    state->writeMemory(phys_page_address + page_edge + 0xc, 0xfe111fe3u);


    return 0;
}

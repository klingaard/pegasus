#include "TranslatedPage.hpp"
#include "core/AtlasState.hpp"

namespace atlas
{
    Action::ItrType TranslatedPage::translatedPageExecute_(AtlasState* state,
                                                           Action::ItrType action_it)
    {
        // Check to see if we're still on the same page
        const auto vaddr = state->getPc();
        if (translation_result_.isContained(vaddr))
        {
            const auto offset = translation_result_.getOffSet(vaddr);
            auto & inst_execute = decode_block_.at(offset);
            inst_execute.setInstAddress(translation_result_.getPAddr(vaddr));
            // If this instruction was never fetched/decoded, the
            // instruction group being called is the Decode action
            // group.  This group returns the execution group
            translated_page_group_.
                setNextActionGroup(inst_execute.getInstActionGroup());
        }
        else {
            // Go back to fetch
            translated_page_group_.setNextActionGroup(fetch_action_group_);
        }

        // The translated page cannot continue.
        return ++action_it;
    }

    Action::ItrType TranslatedPage::InstExecute::setInst_(AtlasState* state, Action::ItrType action_it) {
        state->setCurrentInst(inst_);
        state->setNextPc(state->getPc() + inst_->getOpcodeSize());
        return ++action_it;
    }

    // Need to decode the instruction at the offset
    Action::ItrType TranslatedPage::InstExecute::setupInst_(AtlasState* state,
                                                            Action::ItrType action_it)
    {
        // Decode the instruction at the given PC (in AtlasState)

        // When compressed instructions are enabled, it is possible for a full sized instruction (32
        // bits) to cross a 4K page boundary meaning that first 16 bits of the instruction are on a
        // different page than the second 16 bits. Fetch will always request translation for a 32
        // bit memory access but Translate may need to be performed twice if it detects that the
        // access crosses a 4K page boundary. Since it is possible for the first 16 bits translated
        // and read from memory to result in a valid compressed instruction, Decode must attempt to
        // decode the first 16 bits before asking Translate to translate the second 16 bit access.
        // This ensures that the correct translation faults are triggered.
        //
        // There are several possible scenarios that result in Decode generating a valid
        // instruction:
        //
        // 1. The 32 bit fetch access does not cross a page
        //    boundary. The 32 bits read from memory are decoded as a
        //    non-compressed instruction.
        //
        // 2. The 32 bit fetch access does not cross a page
        //    boundary. The 32 bits read from memory are decoded as a
        //    compressed instruction. The extra 16 bits are discarded.
        //
        // 3. The 32 bit fetch access crosses a page boundary. The
        //    first 16 bits read are a compressed instruction. The
        //    second 16 bits are never translated or read from memory.
        //
        // 4. The 32 bit fetch access crosses a page boundary. The
        //    first 16 bits read are not a valid compressed
        //    instruction. The second 16 bits are translated and read
        //    from memory. The combined 32 bits are decoded as a
        //    non-compressed instruction.
        //
        //const bool page_crossing_access = result.getAccessSize() == 2;
        const bool page_crossing_access = false;

        // Read opcode from memory
        Opcode & opcode = state->getSimState()->current_opcode;
        OpcodeSize opcode_size = 4;
        if (SPARTA_EXPECT_TRUE(!page_crossing_access))
        {
            opcode = state->readMemory<uint32_t>(inst_addr_);

            // Compression detection
            if ((opcode & 0x3) != 0x3)
            {
                opcode = opcode & 0xFFFF;
                opcode_size = 2;
            }
        }
        else
        {
            if (opcode == 0)
            {
                // Load the first 2B, could be a valid 2B compressed inst
                opcode = state->readMemory<uint16_t>(inst_addr_);
                opcode_size = 2;

                if ((opcode & 0x3) == 0x3)
                {
                    // Go back to inst translate
                    //throw ActionException(fetch_action_group_.getNextActionGroup());
                }
            }
            else
            {
                // Load the second 2B of a possible 4B inst
                opcode |= state->readMemory<uint16_t>(inst_addr_) << 16;
            }
        }

        ++(state->getSimState()->current_uid);

        // Decode instruction with Mavis
        AtlasInstPtr inst = nullptr;
        try
        {
            inst = state->getMavis()->makeInst(opcode, state);
            //assert(state->getCurrentInst() == nullptr);
            state->setCurrentInst(inst);
            // Set next PC, can be overidden by a branch/jump
            // instruction or an exception
            state->setNextPc(state->getPc() + opcode_size);
        }
        catch (const mavis::BaseException & e)
        {
            THROW_ILLEGAL_INST;
        }

        // If we only fetched 2B and found a valid compressed inst,
        // then cancel the translation request for the second 2B
        // if (page_crossing_access && (opcode_size == 2))
        // {
        //     state->getFetchTranslationState()->popRequest();
        // }

        if (SPARTA_EXPECT_FALSE(inst->hasCsr()))
        {
            const uint32_t csr =
                inst->getMavisOpcodeInfo()->getSpecialField(mavis::OpcodeInfo::SpecialField::CSR);
            if (state->getCsrRegister(csr) == nullptr)
            {
                THROW_ILLEGAL_INST;
            }

            // TODO: This is probably not the best place for this check...
            if (csr == SATP)
            {
                const uint32_t tvm_val = READ_CSR_FIELD<RV64>(state, MSTATUS, "tvm");
                if ((state->getPrivMode() == PrivMode::SUPERVISOR) && tvm_val)
                {
                    THROW_ILLEGAL_INST;
                }
            }
        }

        // Set up the execution of the instruction and reset the
        // instruction execute group to the instruction that was just
        // setup
        inst_action_group_ = execute_action_group_->execute(state);

        // Before the instruction's execution AtlasState needs to be
        // set to the captured instruction.
        inst_action_group_->insertActionFront(inst_set_inst_);

        // Setup the instruction's next action (after execute adds
        // finishing action groups) to return to this translated page
        inst_action_group_->getNextActionGroup()->
            setNextActionGroup(translated_page_group_);

        // Capture the instruction late -- the above execute functions
        // can throw
        inst_ = state->getCurrentInst();

        // Set the next action group to the inst_action_group for the return!
        inst_setup_group_.setNextActionGroup(inst_action_group_);

        // Go to end...
        return ++action_it;
    }
}

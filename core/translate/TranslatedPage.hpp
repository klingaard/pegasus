#pragma once

#include <cinttypes>

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

        Addr buildAddrMask(PageSize pg_size, Addr paddr)
        {
            Addr mask = 0;
            switch(pg_size)
            {
                case PageSize::SIZE_4K:
                    mask = ~0x1000;
                case PageSize::SIZE_2M:
                    mask = ~0x200000;
                    break;
                case PageSize::SIZE_4M:
                    mask = ~0x400000;
                    break;
                case PageSize::SIZE_1G:
                    mask = ~0x40000000ull;
                    break;
                case PageSize::SIZE_512G:
                    mask = ~0x1000000000ull;
                    break;
                case PageSize::SIZE_256T:
                    mask = ~0x40000000000ull;
                    break;
                case PageSize::INVALID:
                    throw sparta::SpartaException("invalid page size");
                    break;
            }
            return mask & paddr;
        }

        TranslatedPage(const AtlasTranslationState::TranslationResult & translation_result,
                       ActionGroup * fetch_action_group,
                       ActionGroup * decode_action_group) :
            translated_page_group_("TranslatedPageGroup",
                                   atlas::Action::createAction<&TranslatedPage::translatedPageExecute_>(this,
                                                                                                        "TranslatedPageExecute",
                                                                                                        ActionTags::TRANSLATION_PAGE_EXECUTE)),
            decode_block_(4096, {decode_action_group, &translated_page_group_}),
            fetch_action_group_(fetch_action_group),
            addr_mask_(buildAddrMask(translation_result.getPageSize(), translation_result.getPAddr())),
            offset_mask_(~addr_mask_)
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

            InstExecute(ActionGroup * decode_action_group,
                        ActionGroup * translated_page_group) :
                decode_action_group_(decode_action_group),
                translated_page_group_(translated_page_group)
            {
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setupInst_>(this,
                                                                          "TranslatedPageSetupInst"));
            }

            InstExecute(InstExecute&& orig) :
                decode_action_group_(std::move(orig.decode_action_group_)),
                translated_page_group_(std::move(orig.translated_page_group_))
            {
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setupInst_>(this,
                                                                          "TranslatedPageSetupInst"));
            }

            InstExecute(const InstExecute & orig) :
                decode_action_group_(orig.decode_action_group_),
                translated_page_group_(orig.translated_page_group_)
            {
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setupInst_>(this,
                                                                          "TranslatedPageSetupInst"));
            }

            const InstExecute& operator=(const InstExecute & orig)
            {
                decode_action_group_ = orig.decode_action_group_;
                translated_page_group_ = orig.translated_page_group_;
                inst_setup_group_.addAction(
                    atlas::Action::createAction<&InstExecute::setupInst_>(this,
                                                                          "TranslatedPageSetupInst"));
                return *this;
            }

            ActionGroup * getInstActionGroup() { return inst_action_group_; }

        private:

            // Need to decode the instruction at the offset
            Action::ItrType setupInst_(AtlasState* state,
                                       Action::ItrType action_it);

            ActionGroup * decode_action_group_ = nullptr;
            ActionGroup * translated_page_group_ = nullptr;
            ActionGroup   inst_setup_group_{"InstSetupGroup"};
            ActionGroup * inst_action_group_ = &inst_setup_group_;
            AtlasInstPtr inst_;
        };

        // Eventually this vector will be populated with just pure
        // instruction execution action groups
        std::vector<InstExecute> decode_block_;
        ActionGroup * fetch_action_group_ = nullptr;

        const Addr addr_mask_;
        const Addr offset_mask_;

        ActionGroup *decode_action_group_ = nullptr;

    };
}

#pragma once

#include <cassert>

#include "include/PegasusTypes.hpp"

namespace pegasus
{
    class PegasusTranslationState
    {
      public:
        static const uint32_t MAX_TRANSLATION = 64;

        struct TranslationRequest
        {
          public:
            TranslationRequest() = default;

            TranslationRequest(Addr vaddr, size_t access_size) :
                vaddr_(vaddr),
                access_size_(access_size)
            {}

            Addr getVAddr() const { return vaddr_; }

            size_t getAccessSize() const { return access_size_; }

            bool isValid() const { return access_size_ != 0; }

            void setMisaligned(const size_t misaligned_bytes)
            {
                sparta_assert(misaligned_bytes > 0);
                sparta_assert(misaligned_bytes < access_size_);
                misaligned_bytes_ = misaligned_bytes;
                misaligned_ = true;
            }

            bool isMisaligned() const { return misaligned_; }

            size_t getMisalignedBytes() const { return misaligned_bytes_; }

          private:
            Addr vaddr_ = 0;
            size_t access_size_ = 0;
            bool misaligned_ = false;
            size_t misaligned_bytes_ = 0;
        };

        struct TranslationResult
        {
          public:
            TranslationResult() = default;

            TranslationResult(Addr vaddr, Addr paddr,
                              size_t access_sz, PageSize page_sz) :
                vaddr_(vaddr),
                paddr_(paddr),
                access_size_(access_sz),
                page_index_(pageSize(page_sz) - 1),
                page_mask_(~(page_index_))
            {
            }

            ///////////////////////////////////////////////////////////////
            // Methods for the specific vaddr/paddr this result represents

            // Get the original VAddr
            Addr getVAddr() const { return vaddr_; }

            // Get the original PAddr for the original VAddr
            Addr getPAddr() const { return paddr_; }

            size_t getAccessSize() const { return access_size_; }

            bool isValid() const { return access_size_ != 0; }

            ///////////////////////////////////////////////////////////////
            // Methods for reusing this translation result for other vaddrs

            //
            // How addresses are interpreted (example for a 2M page):
            //
            //   vaddr = 0xffff_abcd_e020_0000
            //   page_index_ = SIZE_2M (0x200000 - 1)  == 0x1f_ffff
            //   page_mask_  = ~page_index_ == 0xffff_ffff_ffe0_0000
            //

            // Check to see if the given vaddr is on the same page as
            // this translation result handles
            bool isContained(Addr vaddr) const {
                return (page_mask_ & vaddr) == (page_mask_ & vaddr_);
            }

            // Based on the page size, generate an address index by
            // masking the page index
            Addr genAddrIndx(Addr vaddr) const { return (vaddr & page_index_); }

            // Generate a new PAddr with the given vaddr
            Addr genPAddr(Addr vaddr) const {
                return (page_mask_ & paddr_) | genAddrIndx(vaddr);
            }

        private:
            Addr vaddr_ = 0;
            Addr paddr_ = 0;
            size_t access_size_ = 0;
            Addr page_index_ = 0;
            Addr page_mask_ = 0;
        };

        void makeRequest(const Addr vaddr, const size_t access_size)
        {
            sparta_assert(access_size > 0);
            sparta_assert(results_cnt_ == 0);
            sparta_assert(requests_cnt_ < requests_.size());
            requests_[requests_cnt_++] = {vaddr, access_size};
        }

        uint32_t getNumRequests() const { return requests_cnt_; }

        TranslationRequest & getRequest()
        {
            sparta_assert(requests_cnt_ > 0);
            return requests_[requests_cnt_ - 1];
        }

        void popRequest()
        {
            sparta_assert(requests_cnt_ > 0);
            --requests_cnt_;
        }

        void setResult(const Addr vaddr, const Addr paddr,
                       const size_t access_size, const PageSize page_size)
        {
            sparta_assert(results_cnt_ < results_.size());
            results_[results_cnt_++] = {vaddr, paddr, access_size, page_size};
        }

        uint32_t getNumResults() const { return results_cnt_; }

        const TranslationResult & getResult() const
        {
            sparta_assert(results_cnt_ > 0);
            return results_[results_cnt_ - 1];
        }

        void popResult()
        {
            sparta_assert(results_cnt_ > 0);
            --results_cnt_;
        }

        void reset()
        {
            requests_cnt_ = 0;
            results_cnt_ = 0;
        }

      private:
        // Translation request
        std::array<TranslationRequest, MAX_TRANSLATION> requests_;
        uint32_t requests_cnt_ = 0;

        // Translation result
        std::array<TranslationResult, MAX_TRANSLATION> results_;
        uint32_t results_cnt_ = 0;
    };
} // namespace pegasus

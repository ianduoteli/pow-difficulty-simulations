#include "arith_uint256.h"
#include <algorithm>

class CBlockIndex
{
    public:

    CBlockIndex* pprev;
    int nHeight;
    uint32_t nTime;
    uint32_t nBits;
    arith_uint256 nChainWork;

    CBlockIndex()
    {
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }

    enum { nMedianTimeSpan=11 };

    int64_t GetMedianTimePast() const
    {
        int64_t pmedian[nMedianTimeSpan];
        int64_t* pbegin = &pmedian[nMedianTimeSpan];
        int64_t* pend = &pmedian[nMedianTimeSpan];

        const CBlockIndex* pindex = this;
        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->pprev)
            *(--pbegin) = pindex->GetBlockTime();

        std::sort(pbegin, pend);
        return pbegin[(pend - pbegin)/2];
    }

    const CBlockIndex* GetAncestor(int height) const
    {
        const CBlockIndex* curr = (const CBlockIndex*)this;
        while (curr->nHeight != height)
        {
            if (curr->pprev == NULL) return NULL;
            curr = curr->pprev;
        }
        return curr;
    }
};

class CBlockHeader
{
    public:

    uint32_t nTime;
    uint32_t nBits;

    CBlockHeader()
    {
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};
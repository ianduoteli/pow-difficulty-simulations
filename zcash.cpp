#include "uint256.h"
#include "arith_uint256.h"
#include "params.h"
#include "blocks.h"

unsigned int ZCashCalculateNextWorkRequired(arith_uint256 bnAvg,
                                       int64_t nLastBlockTime, int64_t nFirstBlockTime,
                                       const ConsensusParams& params,
                                       int nextHeight);

unsigned int ZCASH_GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const ConsensusParams& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    if (params.fPowAllowMinDifficultyBlocks)
    {
    // Special difficulty rule for testnet:
    // If the new block's timestamp is more than 2* 10 minutes
    // then allow mining of a min-difficulty block.
    if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
        return nProofOfWorkLimit;
    }

    // Find the first block in the averaging interval
    const CBlockIndex* pindexFirst = pindexLast;
    arith_uint256 bnTot {0};
    for (int i = 0; pindexFirst && i < params.nZCashPowAveragingWindow; i++) {
        arith_uint256 bnTmp;
        bnTmp.SetCompact(pindexFirst->nBits);
        bnTot += bnTmp;
        pindexFirst = pindexFirst->pprev;
    }

    // Check we have enough blocks
    if (pindexFirst == NULL)
        return nProofOfWorkLimit;

    arith_uint256 bnAvg {bnTot / params.nZCashPowAveragingWindow};

    return ZCashCalculateNextWorkRequired(bnAvg,
                                     pindexLast->GetMedianTimePast(), pindexFirst->GetMedianTimePast(),
                                     params,
                                     pindexLast->nHeight + 1);
}

unsigned int ZCashCalculateNextWorkRequired(arith_uint256 bnAvg,
                                       int64_t nLastBlockTime, int64_t nFirstBlockTime,
                                       const ConsensusParams& params,
                                       int nextHeight)
{
    int64_t averagingWindowTimespan = params.AveragingWindowTimespan(nextHeight);
    int64_t minActualTimespan = params.MinActualTimespan(nextHeight);
    int64_t maxActualTimespan = params.MaxActualTimespan(nextHeight);
    // Limit adjustment step
    // Use medians to prevent time-warp attacks
    int64_t nActualTimespan = nLastBlockTime - nFirstBlockTime;
    nActualTimespan = averagingWindowTimespan + (nActualTimespan - averagingWindowTimespan)/4;

    if (nActualTimespan < minActualTimespan) {
        nActualTimespan = minActualTimespan;
    }
    if (nActualTimespan > maxActualTimespan) {
        nActualTimespan = maxActualTimespan;
    }

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew {bnAvg};
    bnNew /= averagingWindowTimespan;
    bnNew *= nActualTimespan;

    if (bnNew > bnPowLimit) {
        bnNew = bnPowLimit;
    }

    return bnNew.GetCompact();
}
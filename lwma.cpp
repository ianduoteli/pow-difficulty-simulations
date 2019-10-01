#include "uint256.h"
#include "arith_uint256.h"
#include "params.h"
#include "blocks.h"

unsigned int LwmaCalculateNextWorkRequired(const CBlockIndex* pindexLast, const ConsensusParams& params);

unsigned int LWMA_GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const ConsensusParams& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
    
    if (params.fPowAllowMinDifficultyBlocks)
    {
    // Special difficulty rule for testnet:
    // If the new block's timestamp is more than 2* 10 minutes
    // then allow mining of a min-difficulty block.
    if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
        return nProofOfWorkLimit;
    }

    if (params.fPowNoRetargeting)
        return pindexLast->nBits;
        
    return LwmaCalculateNextWorkRequired(pindexLast, params);
}

unsigned int LwmaCalculateNextWorkRequired(const CBlockIndex* pindexLast, const ConsensusParams& params)
{
    const int64_t T = params.nPowTargetSpacing;

   // For T=600, 300, 150 use approximately N=60, 90, 120
    const int64_t N = params.nLwmaAveragingWindow;  

    // Define a k that will be used to get a proper average after weighting the solvetimes.
    const int64_t k = N * (N + 1) * T / 2; 

    const int64_t height = pindexLast->nHeight;
    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    
   // New coins should just give away first N blocks before using this algorithm.
    if (height < N) { return powLimit.GetCompact(); }

    arith_uint256 avgTarget, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t sumWeightedSolvetimes = 0, j = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);

        // Prevent solvetimes from being negative in a safe way. It must be done like this. 
        // In particular, do not attempt anything like  if(solvetime < 0) {solvetime=0;}
        // The +1 ensures new coins do not calculate nextTarget = 0.
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

       // A 6*T limit will prevent large drops in difficulty from long solvetimes.
       //// *** MODIFICATION:
       //// Remove min to recover faster from a hash spike (attacker spikes hash then disappears)
       //// int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);
        int64_t solvetime = thisTimestamp - previousTimestamp;

       // The following is part of "preventing negative solvetimes". 
        previousTimestamp = thisTimestamp;
       
       // Give linearly higher weight to more recent solvetimes.
        j++;
        sumWeightedSolvetimes += solvetime * j; 

        arith_uint256 target;
        target.SetCompact(block->nBits);
        avgTarget += target / N / k; // Dividing by k here prevents an overflow below.
    }

   // Desired equation in next line was nextTarget = avgTarget * sumWeightSolvetimes / k
   // but 1/k was moved to line above to prevent overflow in new coins

    nextTarget = avgTarget * sumWeightedSolvetimes; 

    if (nextTarget > powLimit) { nextTarget = powLimit; }

    return nextTarget.GetCompact();
}

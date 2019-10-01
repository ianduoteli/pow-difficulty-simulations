#include "uint256.h"
#include "params.h"
#include "blocks.h"
#include <stdio.h>

double getDifficulty(uint32_t nBits)
{
    int nShift = (nBits >> 24) & 0xff;
    double dDiff =
        (double)0x0000ffff / (double)(nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for an arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (bnTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

CBlockIndex* doWorkAndAddToTip(double megaHashPerSec, CBlockIndex* tip, CBlockHeader* proposedBlock)
{
    CBlockIndex* newTip = new CBlockIndex();
    newTip->pprev = tip;
    newTip->nHeight = tip->nHeight + 1;
    newTip->nBits = proposedBlock->nBits;
    double powTime = getDifficulty(newTip->nBits) * ((double)((int64_t)1 << 32)) / megaHashPerSec / 1000000.0;
    newTip->nTime = proposedBlock->nTime + powTime;
    newTip->nChainWork = tip->nChainWork + GetBlockProof(*newTip);
    return newTip;
}

char* formatTime(uint32_t nTime)
{
    static char buf[2048];
    uint32_t sec = nTime % 60;
    nTime /= 60;
    uint32_t min = nTime % 60;
    nTime /= 60;
    uint32_t hour = nTime % 24;
    nTime /= 24;
    uint32_t day = nTime;
    sprintf(buf, "%dd %02d:%02d:%02d", day, hour, min, sec);
    return buf;
}

char* formatTimeDelta(uint32_t nTime1, uint32_t nTime2)
{
    static char buf[2048];
    uint32_t nTime = nTime1 - nTime2;
    uint32_t sec = nTime % 60;
    nTime /= 60;
    uint32_t min = nTime % 60;
    nTime /= 60;
    uint32_t hour = nTime % 24;
    nTime /= 24;
    uint32_t day = nTime;
    sprintf(buf, "%dd %02d:%02d:%02d", day, hour, min, sec);
    return buf;
}

unsigned int LWMA_GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const ConsensusParams& params);
unsigned int ZCASH_GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const ConsensusParams& params);
unsigned int DAA_GetNextWorkRequired(const CBlockIndex *pindexPrev, const CBlockHeader *pblock, const ConsensusParams &params);

void playScenario(double preAttackMegaHashPerSec, uint32_t preAttackDuration, double attackMegaHashPerSec, uint32_t attackDuration, double postAttackMegaHashPerSec, uint32_t postAttackDuration, const ConsensusParams& params)
{
    bool printedAttackStarted = false;
    bool printedAttackedStopped = false;
    CBlockIndex* tip = new CBlockIndex(); // genesis
    tip->pprev = NULL;
    tip->nHeight = 0;
    tip->nTime = 0;
    tip->nBits = 0x1d00ffff; // minimum difficulty
    tip->nChainWork = GetBlockProof(*tip);

    printf("STARTED: now mining at %f MH\n", preAttackMegaHashPerSec);

    int preAttackBlocks = 0;
    while (tip->nTime < preAttackDuration + attackDuration + postAttackDuration)
    {
        CBlockHeader* proposed = new CBlockHeader();
        proposed->nTime = tip->nTime + 1;
        //// choose your algorithm here:
        proposed->nBits = LWMA_GetNextWorkRequired(tip, proposed, params);
        double megaHashPerSec = preAttackMegaHashPerSec;
        if (proposed->nTime >= preAttackDuration) 
        {
            if (!printedAttackStarted)
            {
                preAttackBlocks = tip->nHeight;
                printf("ATTACK STARTED: now mining at %f MH\n", attackMegaHashPerSec);
                printedAttackStarted = true;
            }
            megaHashPerSec = attackMegaHashPerSec;

            if (proposed->nTime >= preAttackDuration + attackDuration) 
            {
                if (!printedAttackedStopped)
                {
                    int attackerBlocks = tip->nHeight - preAttackBlocks;
                    int standardBlocks = ((double)attackDuration)/600.0;
                    printf("attacker mined %d blocks instead of %d (%d extra, %.2f%% increase)\n", attackerBlocks, standardBlocks, attackerBlocks - standardBlocks, (double)attackerBlocks / (double)standardBlocks * 100 - 100);
                    printf("ATTACK STOPPED: now mining at %f MH\n", postAttackMegaHashPerSec);
                    printedAttackedStopped = true;
                }
                megaHashPerSec = postAttackMegaHashPerSec;
            }
        }
        
        tip = doWorkAndAddToTip(megaHashPerSec, tip, proposed);
        printf("  added block %04d time %s (+%s) diff %f\n", tip->nHeight, formatTime(tip->nTime), formatTimeDelta(tip->nTime, tip->pprev->nTime), getDifficulty(tip->nBits));
    }
}

int main()
{
    ConsensusParams params;
    params.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    params.fPowAllowMinDifficultyBlocks = false;
    params.fPowNoRetargeting = false;
    params.nPowTargetSpacing = 10 * 60;
    // LWMA
    params.nLwmaAveragingWindow = 20; // 60, 45, 30, 20
    // ZCASH
    params.nZCashPowAveragingWindow = 11;
    params.nZCashPowMaxAdjustDown = 32; // 32% adjustment down
    params.nZCashPowMaxAdjustUp = 16; // 16% adjustment up
    // DAA
    params.nDaaAverageWindow = 30;

    //// INSTRUCTIONS:
    //// 1. choose the algorithm you want in line 105
    //// 2. uncomment the scenario you want to play:

    // no attack (sanity test)
    //playScenario(7*1000, 24*60*60, 7*1000, 12*60*60, 7*1000, 60*60, params);

    // attack 5 minutes at x1000
    //playScenario(7*1000, 24*60*60, 7*1000*1000, 5*60, 7*1000, 7*24*60*60, params);

    // attack 1 hour at x1000
    playScenario(7*1000, 24*60*60, 7*1000*1000, 1*60*60, 7*1000, 14*24*60*60, params);

    // attack 24 hours at x1000
    //playScenario(7*1000, 24*60*60, 7*1000*1000, 24*60*60, 7*1000, 14*24*60*60, params);

    return 0;
}
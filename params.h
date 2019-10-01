class ConsensusParams 
{
    public:

    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    int64_t nLwmaAveragingWindow;
    int64_t nZCashPowAveragingWindow;
    int64_t nZCashPowMaxAdjustDown;
    int64_t nZCashPowMaxAdjustUp;
    int64_t nDaaAverageWindow;

    ConsensusParams()
    {
    }

    int64_t AveragingWindowTimespan(int nHeight) const 
    {
        return nZCashPowAveragingWindow * nPowTargetSpacing;
    }

    int64_t MinActualTimespan(int nHeight) const 
    {
        return (AveragingWindowTimespan(nHeight) * (100 - nZCashPowMaxAdjustUp)) / 100;
    }

    int64_t MaxActualTimespan(int nHeight) const 
    {
        return (AveragingWindowTimespan(nHeight) * (100 + nZCashPowMaxAdjustDown)) / 100;
    }
};
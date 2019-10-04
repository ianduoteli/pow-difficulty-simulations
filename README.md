# PoW Difficulty Simulations

As part of the research for [BRIP-3](https://github.com/bitcoinroyale/whitepaper/issues/17), this is a simulation of mining attack vectors in the form of a hash power spike. Several popular alternative difficulty adjustment algorithms are compared head to head.

Specials thanks to [@zawy12](https://github.com/zawy12) for the implementations and discussion around [this topic](https://github.com/zawy12/difficulty-algorithms).

## Compared algorithms

  * LWMA - currently used by Bitcoin Gold
  * DigiShield3 - currently used by ZCash
  * DAA - currently used by Bitcoin Cash

**LWMA is demonstrated to be the best alternative, it also responds the quickest to changes.**

## Building and running the simulation

1. Compile
    ```
    g++ main.cpp lwma.cpp zcash.cpp daa.cpp uint256.cpp arith_uint256.cpp strencodings.cpp -o diffsim
    ```

2. Edit `main.cpp` to choose the scenario

3. Run the scenario
    ```
    ./diffsim
    ```

## Why is this challenging

Using the same hash algorithm as Bitcoin (sha256) is risky since specialized ASIC mining rigs are several x1000's more powerful than non-specialized equipment. Since mining rigs are widely available for rent, an attacker could rent significant hash power for a short period of time and disrupt the protocol. In addition, in the early days of the protocol, when honest hash power is naturally still low, attacks are significantly easier.

## LWMA parameter tuning

Proposed parameters based on these simulations (this is open for debate of course):

* During early days of the protocol (little hash power)
    ```
    nLwmaAveragingWindow = 30
    solvetime = thisTimestamp - previousTimestamp
    ```
* During mature days of the protocol (substantial hash power)
    ```
    nLwmaAveragingWindow = 45
    solvetime = std::min(6 * T, thisTimestamp - previousTimestamp)
    ```

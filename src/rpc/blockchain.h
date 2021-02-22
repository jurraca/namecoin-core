// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_BLOCKCHAIN_H
#define BITCOIN_RPC_BLOCKCHAIN_H

#include <amount.h>
#include <streams.h>
#include <sync.h>

#include <stdint.h>
#include <vector>

extern RecursiveMutex cs_main;

class CBlock;
class CBlockIndex;
class CBlockPolicyEstimator;
class CChainState;
class CTxMemPool;
class ChainstateManager;
class UniValue;
struct NodeContext;
namespace util {
class Ref;
} // namespace util

static constexpr int NUM_GETBLOCKSTATS_PERCENTILES = 5;

/**
 * Get the difficulty of the net wrt to the given bits.
 *
 * @return A floating point number that is a multiple of the main net minimum
 * difficulty (4295032833 hashes).
 */
double GetDifficultyForBits(uint32_t nBits);

/** Callback for when block tip changed. */
void RPCNotifyBlockChange(const CBlockIndex*);

/** Block description to JSON */
UniValue blockToJSON(const CBlock& block, const CBlockIndex* tip, const CBlockIndex* blockindex, bool txDetails = false) LOCKS_EXCLUDED(cs_main);

/** Mempool information to JSON */
UniValue MempoolInfoToJSON(const CTxMemPool& pool);

/** Mempool to JSON */
UniValue MempoolToJSON(const CTxMemPool& pool, bool verbose = false, bool include_mempool_sequence = false);

/** Block header to JSON */
UniValue blockheaderToJSON(const CBlockIndex* tip, const CBlockIndex* blockindex) LOCKS_EXCLUDED(cs_main);

/** Used by getblockstats to get feerates at different percentiles by weight  */
void CalculatePercentilesByWeight(CAmount result[NUM_GETBLOCKSTATS_PERCENTILES], std::vector<std::pair<CAmount, int64_t>>& scores, int64_t total_weight);

NodeContext& EnsureNodeContext(const util::Ref& context);
CTxMemPool& EnsureMemPool(const util::Ref& context);
ChainstateManager& EnsureChainman(const util::Ref& context);
CBlockPolicyEstimator& EnsureFeeEstimator(const util::Ref& context);

/**
 * Helper to create UTXO snapshots given a chainstate and a file handle.
 * @return a UniValue map containing metadata about the snapshot.
 */
UniValue CreateUTXOSnapshot(NodeContext& node, CChainState& chainstate, CAutoFile& afile);

#endif

// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2016 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_AUXPOW_H
#define BITCOIN_AUXPOW_H

#include <consensus/params.h>
#include <primitives/pureheader.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

#include <memory>
#include <vector>

class CBlock;
class CBlockHeader;
class CBlockIndex;
class CValidationState;
class UniValue;

namespace auxpow_tests
{
class CAuxPowForTest;
}

/** Header for merge-mining data in the coinbase.  */
static const unsigned char pchMergedMiningHeader[] = { 0xfa, 0xbe, 'm', 'm' };

/* Because it is needed for auxpow, the definition of CMerkleTx is moved
   here from wallet.h.  */

/** A transaction with a merkle branch linking it to the block chain. */
class CMerkleTx
{
private:
  /** Constant used in hashBlock to indicate tx has been abandoned */
    static const uint256 ABANDON_HASH;

public:
    CTransactionRef tx;
    uint256 hashBlock;
    std::vector<uint256> vMerkleBranch;

    /* An nIndex == -1 means that hashBlock (in nonzero) refers to the earliest
     * block in the chain we know this or any in-wallet dependency conflicts
     * with. Older clients interpret nIndex == -1 as unconfirmed for backward
     * compatibility.
     */
    int nIndex;

    CMerkleTx()
    {
        SetTx(MakeTransactionRef());
        Init();
    }

    explicit CMerkleTx(CTransactionRef arg)
    {
        SetTx(std::move(arg));
        Init();
    }

    void Init()
    {
        hashBlock = uint256();
        nIndex = -1;
    }

    void SetTx(CTransactionRef arg)
    {
        tx = std::move(arg);
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(tx);
        READWRITE(hashBlock);
        READWRITE(vMerkleBranch);
        READWRITE(nIndex);
    }

    void SetMerkleBranch(const CBlockIndex* pindex, int posInBlock);

    /**
     * Return depth of transaction in blockchain:
     * <0  : conflicts with a transaction this deep in the blockchain
     *  0  : in memory pool, waiting to be included in a block
     * >=1 : this many blocks deep in the main chain
     */
    int GetDepthInMainChain(const CBlockIndex* &pindexRet) const;
    int GetDepthInMainChain() const { const CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet); }
    bool IsInMainChain() const { const CBlockIndex *pindexRet; return GetDepthInMainChain(pindexRet) > 0; }
    int GetBlocksToMaturity() const;
    bool hashUnset() const { return (hashBlock.IsNull() || hashBlock == ABANDON_HASH); }
    bool isAbandoned() const { return (hashBlock == ABANDON_HASH); }
    void setAbandoned() { hashBlock = ABANDON_HASH; }

    const uint256& GetHash() const { return tx->GetHash(); }
    bool IsCoinBase() const { return tx->IsCoinBase(); }
};

/**
 * Data for the merge-mining auxpow.  This uses a merkle tx (the parent block's
 * coinbase tx) and a manual merkle branch to link the actual Namecoin block
 * header to the parent block header, which is mined to satisfy the PoW.
 */
class CAuxPow
{

private:

  /**
   * The parent block's coinbase tx, which is used to link the auxpow from
   * the tx input to the parent block header.
   */
  CMerkleTx coinbaseTx;

  /** The merkle branch connecting the aux block to our coinbase.  */
  std::vector<uint256> vChainMerkleBranch;

  /** Merkle tree index of the aux block header in the coinbase.  */
  int nChainIndex;

  /** Parent block header (on which the real PoW is done).  */
  CPureBlockHeader parentBlock;

  friend UniValue AuxpowToJSON(const CAuxPow& auxpow);
  friend class auxpow_tests::CAuxPowForTest;

public:

  /* Prevent accidental conversion.  */
  inline explicit CAuxPow (CTransactionRef txIn)
    : coinbaseTx (txIn)
  {}

  CAuxPow () = default;

  ADD_SERIALIZE_METHODS;

  template<typename Stream, typename Operation>
    inline void
    SerializationOp (Stream& s, Operation ser_action)
  {
    READWRITE (coinbaseTx);
    READWRITE (vChainMerkleBranch);
    READWRITE (nChainIndex);
    READWRITE (parentBlock);
  }

  /**
   * Check the auxpow, given the merge-mined block's hash and our chain ID.
   * Note that this does not verify the actual PoW on the parent block!  It
   * just confirms that all the merkle branches are valid.
   * @param hashAuxBlock Hash of the merge-mined block.
   * @param nChainId The auxpow chain ID of the block to check.
   * @param params Consensus parameters.
   * @return True if the auxpow is valid.
   */
  bool check (const uint256& hashAuxBlock, int nChainId,
              const Consensus::Params& params) const;

  /**
   * Returns the parent block hash.  This is used to validate the PoW.
   */
  inline uint256
  getParentBlockHash () const
  {
    return parentBlock.GetHash ();
  }

  /**
   * Calculate the expected index in the merkle tree.  This is also used
   * for the test-suite.
   * @param nNonce The coinbase's nonce value.
   * @param nChainId The chain ID.
   * @param h The merkle block height.
   * @return The expected index for the aux hash.
   */
  static int getExpectedIndex (uint32_t nNonce, int nChainId, unsigned h);

  /**
   * Check a merkle branch.  This used to be in CBlock, but was removed
   * upstream.  Thus include it here now.
   */
  static uint256 CheckMerkleBranch (uint256 hash,
                                    const std::vector<uint256>& vMerkleBranch,
                                    int nIndex);

  /**
   * Constructs a minimal CAuxPow object for the given block header and
   * returns it.  The caller should make sure to set the auxpow flag on the
   * header already, since the block hash to which the auxpow commits depends
   * on that!
   */
  static std::unique_ptr<CAuxPow> createAuxPow (const CPureBlockHeader& header);

  /**
   * Initialises the auxpow of the given block header.  This builds a minimal
   * auxpow object like createAuxPow and sets it on the block header.  Returns
   * a reference to the parent header so it can be mined as a follow-up.
   */
  static CPureBlockHeader& initAuxPow (CBlockHeader& header);

};

#endif // BITCOIN_AUXPOW_H

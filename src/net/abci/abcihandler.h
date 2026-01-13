#ifndef _ABCIHANDLER_H_
#define _ABCIHANDLER_H_

#include "tendermint/abci/types.pb.h"

/**
 * The implementor of ABCIHandler is the actual handler of ABCI requests
 * received from cometbft. Client code should implement this interface
 * and give an instance of this implementor via the ABCIServer constructor.
 */
class ABCIHandler {
  public:
    virtual void echo(const tendermint::abci::RequestEcho& req, tendermint::abci::ResponseEcho* res) = 0;
    virtual void flush(const tendermint::abci::RequestFlush& req, tendermint::abci::ResponseFlush* res) = 0;
    virtual void info(const tendermint::abci::RequestInfo& req, tendermint::abci::ResponseInfo* res) = 0;
    virtual void init_chain(const tendermint::abci::RequestInitChain& req, tendermint::abci::ResponseInitChain* res) = 0;
    virtual void prepare_proposal(const tendermint::abci::RequestPrepareProposal& req, tendermint::abci::ResponsePrepareProposal* res) = 0;
    virtual void process_proposal(const tendermint::abci::RequestProcessProposal& req, tendermint::abci::ResponseProcessProposal* res) = 0;
    virtual void check_tx(const tendermint::abci::RequestCheckTx& req, tendermint::abci::ResponseCheckTx* res) = 0;
    virtual void commit(const tendermint::abci::RequestCommit& req, tendermint::abci::ResponseCommit* res) = 0;
    virtual void finalize_block(const tendermint::abci::RequestFinalizeBlock& req, tendermint::abci::ResponseFinalizeBlock* res) = 0;
    virtual void query(const tendermint::abci::RequestQuery& req, tendermint::abci::ResponseQuery* res) = 0;
    virtual void list_snapshots(const tendermint::abci::RequestListSnapshots& req, tendermint::abci::ResponseListSnapshots* res) = 0;
    virtual void offer_snapshot(const tendermint::abci::RequestOfferSnapshot& req, tendermint::abci::ResponseOfferSnapshot* res) = 0;
    virtual void load_snapshot_chunk(const tendermint::abci::RequestLoadSnapshotChunk& req, tendermint::abci::ResponseLoadSnapshotChunk* res) = 0;
    virtual void apply_snapshot_chunk(const tendermint::abci::RequestApplySnapshotChunk& req, tendermint::abci::ResponseApplySnapshotChunk* res) = 0;
    virtual void extend_vote(const tendermint::abci::RequestExtendVote& req, tendermint::abci::ResponseExtendVote* res) = 0;
    virtual void verify_vote_extension(const tendermint::abci::RequestVerifyVoteExtension& req, tendermint::abci::ResponseVerifyVoteExtension* res) = 0;
};

#endif

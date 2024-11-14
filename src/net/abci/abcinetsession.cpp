#include "abcinetsession.h"

#include "../../utils/logger.h"

// TODO: redirect all calls to the ABCIHandler

// TODO: review all the error handling and the net code in general

ABCINetSession::ABCINetSession(ABCIHandler* handler, stream_protocol::socket socket, std::shared_ptr<ABCINetServer> server)
    : handler_(handler), socket_(std::move(socket)), server_(server) {}

void ABCINetSession::start() {
    do_read_message();
}

void ABCINetSession::close() {
    GLOGXTRACE("Before socket close");
    socket_.close();
    GLOGXTRACE("After socket close");
}

void ABCINetSession::do_read_message() {
    auto self(shared_from_this());
    // Read the varint message length asynchronously
    read_varint([this, self](bool success, uint64_t msg_len) {
        if (!success || msg_len == 0 || msg_len > MAX_MESSAGE_SIZE) {
            //socket_.close();
            server_->notify_failure("Error reading message length (failed)");
            return;
        }
                if (msg_len == 0) {
            //socket_.close();
            server_->notify_failure("Error reading message length (len==0)");
            return;
                if (msg_len > MAX_MESSAGE_SIZE) {
            //socket_.close();
            server_->notify_failure("Error reading message length (maxlen)");
            return;
        }}

        message_data_.resize(msg_len);
        // Read the message data
        asio::async_read(socket_, asio::buffer(message_data_), [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (ec) {
                //socket_.close();
                server_->notify_failure("Error reading message data: " + ec.message());
                return;
            }
            // Process the message
            process_request();
        });
    });
}

void ABCINetSession::process_request() {
    // Parse the message into a Request
    cometbft::abci::v1::Request request;
    if (!request.ParseFromArray(message_data_.data(), message_data_.size())) {
        //socket_.close();
        server_->notify_failure("Failed to parse request");
        return;
    }

    // Create a Response message
    cometbft::abci::v1::Response response;

    // Handle the request
    switch (request.value_case()) {
        case cometbft::abci::v1::Request::kEcho: {
            const auto& echo_req = request.echo();
            auto* echo_resp = response.mutable_echo();
            // Actually set the correct/expected echo answer here.
            echo_resp->set_message(echo_req.message());
            // The caller doesn't actually have to do anything.
            handler_->echo(echo_req, echo_resp);
            break;
        }
        case cometbft::abci::v1::Request::kFlush: {
            const auto& flush_req = request.flush();
            auto* flush_resp = response.mutable_flush();
            handler_->flush(flush_req, flush_resp);
            break;
        }
        case cometbft::abci::v1::Request::kInfo: {
            const auto& info_req = request.info();
            auto* info_resp = response.mutable_info();
            handler_->info(info_req, info_resp);
            break;
        }
        case cometbft::abci::v1::Request::kInitChain: {
            const auto& init_chain_req = request.init_chain();
            auto* init_chain_resp = response.mutable_init_chain();
            handler_->init_chain(init_chain_req, init_chain_resp);
            break;
        }
        case cometbft::abci::v1::Request::kPrepareProposal: {
            const auto& prepare_proposal_req = request.prepare_proposal();
            auto* prepare_proposal_resp = response.mutable_prepare_proposal();
            handler_->prepare_proposal(prepare_proposal_req, prepare_proposal_resp);
            break;
        }
        case cometbft::abci::v1::Request::kProcessProposal: {
            const auto& process_proposal_req = request.process_proposal();
            auto* process_proposal_resp = response.mutable_process_proposal();
            handler_->process_proposal(process_proposal_req, process_proposal_resp);
            break;
        }
        case cometbft::abci::v1::Request::kCheckTx: {
            const auto& check_tx_req = request.check_tx();
            auto* check_tx_resp = response.mutable_check_tx();
            handler_->check_tx(check_tx_req, check_tx_resp);
            break;
        }
        case cometbft::abci::v1::Request::kQuery: {
            const auto& query_req = request.query();
            auto* query_resp = response.mutable_query();
            handler_->query(query_req, query_resp);
            break;
        }
        case cometbft::abci::v1::Request::kCommit: {
            const auto& commit_req = request.commit();
            auto* commit_resp = response.mutable_commit();
            handler_->commit(commit_req, commit_resp);
            break;
        }
        case cometbft::abci::v1::Request::kExtendVote: {
            const auto& extend_vote_req = request.extend_vote();
            auto* extend_vote_resp = response.mutable_extend_vote();
            handler_->extend_vote(extend_vote_req, extend_vote_resp);
            break;
        }
        case cometbft::abci::v1::Request::kVerifyVoteExtension: {
            const auto& verify_vote_extension_req = request.verify_vote_extension();
            auto* verify_vote_extension_resp = response.mutable_verify_vote_extension();
            handler_->verify_vote_extension(verify_vote_extension_req, verify_vote_extension_resp);
            break;
        }
        case cometbft::abci::v1::Request::kFinalizeBlock: {
            const auto& finalize_block_req = request.finalize_block();  
            auto* finalize_block_resp = response.mutable_finalize_block();
            handler_->finalize_block(finalize_block_req, finalize_block_resp);
            break;
        }
        case cometbft::abci::v1::Request::kListSnapshots: {
            const auto& list_snapshots_req = request.list_snapshots();
            auto* list_snapshots_resp = response.mutable_list_snapshots();
            handler_->list_snapshots(list_snapshots_req, list_snapshots_resp);
            break;
        }
        case cometbft::abci::v1::Request::kOfferSnapshot: {
            const auto& offer_snapshot_req = request.offer_snapshot();
            auto* offer_snapshot_resp = response.mutable_offer_snapshot();
            handler_->offer_snapshot(offer_snapshot_req, offer_snapshot_resp);
            break;
        }
        case cometbft::abci::v1::Request::kLoadSnapshotChunk: {
            const auto& load_snapshot_chunk_req = request.load_snapshot_chunk();
            auto* load_snapshot_chunk_resp = response.mutable_load_snapshot_chunk();
            handler_->load_snapshot_chunk(load_snapshot_chunk_req, load_snapshot_chunk_resp);
            break;
        }
        case cometbft::abci::v1::Request::kApplySnapshotChunk: {
            const auto& apply_snapshot_chunk_req = request.apply_snapshot_chunk();
            auto* apply_snapshot_chunk_resp = response.mutable_apply_snapshot_chunk();
            handler_->apply_snapshot_chunk(apply_snapshot_chunk_req, apply_snapshot_chunk_resp);
            break;
        }
        default: {
            server_->notify_failure("Received an unknown request type");
            auto* exception_resp = response.mutable_exception();
            exception_resp->set_error("Unknown request type");
            break;
        }
    }

    // Serialize the response
    size_t response_size = response.ByteSizeLong();
    response_data_.resize(response_size);
    if (!response.SerializeToArray(response_data_.data(), response_size)) {
        //socket_.close();
        server_->notify_failure("Failed to serialize response");
        return;
    }

    // Write the response
    do_write_message();
}

void ABCINetSession::do_write_message() {
    auto self(shared_from_this());
    // Write the varint length prefix and response data
    std::vector<uint8_t> write_buffer;
    write_varint(response_data_.size(), write_buffer);
    write_buffer.insert(write_buffer.end(), response_data_.begin(), response_data_.end());

    asio::async_write(socket_, asio::buffer(write_buffer), [this, self](boost::system::error_code ec, std::size_t /*length*/) {
        if (ec) {
            //socket_.close();
            server_->notify_failure("Error writing response: " + ec.message());
            return;
        }
        // Continue reading the next message
        do_read_message();
    });
}

void ABCINetSession::read_varint(std::function<void(bool, uint64_t)> handler) {
    auto self(shared_from_this());
    varint_value_ = 0;
    varint_shift_ = 0;
    do_read_varint_byte(handler);
}

void ABCINetSession::do_read_varint_byte(std::function<void(bool, uint64_t)> handler) {
    auto self(shared_from_this());
    asio::async_read(socket_, asio::buffer(&varint_byte_, 1), [this, self, handler](boost::system::error_code ec, std::size_t /*length*/) {
        if (ec) {
            handler(false, 0);
            server_->notify_failure("Error reading varint byte: " + ec.message());
            return;
        }
        varint_value_ |= ((uint64_t)(varint_byte_ & 0x7F)) << varint_shift_;
        if (!(varint_byte_ & 0x80)) {
            handler(true, varint_value_);
        } else {
            varint_shift_ += 7;
            if (varint_shift_ >= 64) {
                handler(false, 0); // Varint too long
                server_->notify_failure("Varint too long");
                return;
            }
            do_read_varint_byte(handler);
        }
    });
}

void ABCINetSession::write_varint(uint64_t value, std::vector<uint8_t>& buffer) {
    while (true) {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value) {
            byte |= 0x80;
        }
        buffer.push_back(byte);
        if (!value) {
            break;
        }
    }
}

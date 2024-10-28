#include "abcinetsession.h"

// TODO: redirect all calls to the ABCIHandler

// TODO: review all the error handling and the net code in general

ABCINetSession::ABCINetSession(ABCIHandler* handler, stream_protocol::socket socket, std::shared_ptr<ABCINetServer> server)
    : handler_(handler), socket_(std::move(socket)), server_(server) {}

void ABCINetSession::start() {
    do_read_message();
}

void ABCINetSession::close() {
    std::cout << "ABCINetSession::close() start" << std::endl;
    socket_.close();
    std::cout << "ABCINetSession::close() done" << std::endl;
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

            std::cout << "Got Echo" << std::endl;
            const auto& echo_req = request.echo();
            auto* echo_resp = response.mutable_echo();

            // FIXME: move the echo implementation into the handler
            echo_resp->set_message(echo_req.message());

            // call handler
            // FIXME: just checking if the types are correct & this compiles for now
            handler_->echo(echo_req, echo_resp);

            break;
        }
        case cometbft::abci::v1::Request::kFlush: {
            std::cout << "Got Flush" << std::endl;
            response.mutable_flush();
            break;
        }
        case cometbft::abci::v1::Request::kInfo: {
            std::cout << "Got Info" << std::endl;
            const auto& info_req = request.info();
            auto* info_resp = response.mutable_info();
            info_resp->set_version("1.0.0");
            info_resp->set_last_block_height(0);
            info_resp->set_last_block_app_hash("");
            break;
        }
        case cometbft::abci::v1::Request::kInitChain: {


            std::cout << "Got InitChain" << std::endl;
            auto* init_chain_resp = response.mutable_init_chain();
            // Populate InitChainResponse as needed
            break;
        }
        case cometbft::abci::v1::Request::kPrepareProposal: {
            std::cout << "Got PrepareProposal" << std::endl;
            auto* prepare_resp = response.mutable_prepare_proposal();
            // Implement PrepareProposal logic here
            prepare_resp->clear_txs();
            break;
        }
        case cometbft::abci::v1::Request::kProcessProposal: {
            std::cout << "Got ProcessProposal" << std::endl;
            auto* process_resp = response.mutable_process_proposal();
            process_resp->set_status(cometbft::abci::v1::PROCESS_PROPOSAL_STATUS_ACCEPT);
            break;
        }
        case cometbft::abci::v1::Request::kCheckTx: {
            std::cout << "Got CheckTx" << std::endl;
            const auto& check_tx_req = request.check_tx();
            auto* check_tx_resp = response.mutable_check_tx();
            check_tx_resp->set_code(0); // Success
            break;
        }
        case cometbft::abci::v1::Request::kQuery: {
            std::cout << "Got Query" << std::endl;
            const auto& query_req = request.query();
            auto* query_resp = response.mutable_query();
            query_resp->set_code(0); // Success
            break;
        }
        case cometbft::abci::v1::Request::kCommit: {
            std::cout << "Got Commit" << std::endl;
            auto* commit_resp = response.mutable_commit();
            commit_resp->set_retain_height(0);
            break;
        }
        case cometbft::abci::v1::Request::kExtendVote: {
            std::cout << "Got ExtendVote" << std::endl;
            auto* extend_vote_resp = response.mutable_extend_vote();
            extend_vote_resp->set_vote_extension("");
            break;
        }
        case cometbft::abci::v1::Request::kVerifyVoteExtension: {
            std::cout << "Got VerifyVoteExtension" << std::endl;
            auto* verify_vote_resp = response.mutable_verify_vote_extension();
            verify_vote_resp->set_status(cometbft::abci::v1::VERIFY_VOTE_EXTENSION_STATUS_ACCEPT);
            break;
        }
        case cometbft::abci::v1::Request::kFinalizeBlock: {
            //std::cout << "Got FinalizeBlock" << std::endl;

            const auto& finalize_req = request.finalize_block();  

            //std::cout << "THE BLOCK HEIGHT IS NOW = " << finalize_req.height() << std::endl;
            //server_->lastBlockHeight_ = finalize_req.height();

            auto* finalize_resp = response.mutable_finalize_block();
            finalize_resp->set_app_hash("");
            break;
        }
        case cometbft::abci::v1::Request::kListSnapshots: {
            std::cout << "Got ListSnapshots" << std::endl;
            auto* list_snapshots_resp = response.mutable_list_snapshots();
            // Populate ListSnapshotsResponse as needed
            break;
        }
        case cometbft::abci::v1::Request::kOfferSnapshot: {
            std::cout << "Got OfferSnapshot" << std::endl;
            auto* offer_snapshot_resp = response.mutable_offer_snapshot();
            // Populate OfferSnapshotResponse as needed
            break;
        }
        case cometbft::abci::v1::Request::kLoadSnapshotChunk: {
            std::cout << "Got LoadSnapshotChunk" << std::endl;
            auto* load_snapshot_chunk_resp = response.mutable_load_snapshot_chunk();
            // Populate LoadSnapshotChunkResponse as needed
            break;
        }
        case cometbft::abci::v1::Request::kApplySnapshotChunk: {
            std::cout << "Got ApplySnapshotChunk" << std::endl;
            auto* apply_snapshot_chunk_resp = response.mutable_apply_snapshot_chunk();
            // Populate ApplySnapshotChunkResponse as needed
            break;
        }
        default: {
            std::cerr << "Unknown request type" << std::endl;
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

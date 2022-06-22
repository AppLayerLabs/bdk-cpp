#include "db.h"


bool DBService::has(std::string key, std::string prefix) {
  rpcdb::HasRequest request;
  rpcdb::HasResponse response;
  ClientContext context;

  key = prefix + key;
  request.set_key(key);

  lock.lock();
  Status status = db_stub_->Has(&context, request, &response);
  lock.unlock();
  if (status.ok()) {
    return response.has();
  } else { 
    Utils::logToFile("DB Has Comm Failed");
    throw "";
  }
}

std::string DBService::get(std::string key, std::string prefix) {
  rpcdb::GetRequest request;
  rpcdb::GetResponse response;
  ClientContext context;

  key = prefix + key;
  request.set_key(key);
  lock.lock();
  Status status = db_stub_->Get(&context, request, &response);
  lock.unlock();
  if (status.ok()) {
    return response.value();
  } else { 
    Utils::logToFile("DB Get Comm Failed");
    throw "";
  }
}

bool DBService::put(std::string key, std::string value, std::string prefix) {
  rpcdb::PutRequest request;
  rpcdb::PutResponse response;
  ClientContext context;

  key = prefix + key;
  request.set_key(key);
  request.set_value(value);
  lock.lock();
  Status status = db_stub_->Put(&context, request, &response);
  lock.unlock();
  if (status.ok()) {
    return true;
  } else { 
    Utils::logToFile("DB Put Comm Failed");
    throw "";
  }
}

bool DBService::del(std::string key, std::string prefix) {
  rpcdb::DeleteRequest request;
  rpcdb::DeleteResponse response;
  ClientContext context;

  key = prefix + key;
  request.set_key(key);
  lock.lock();
  Status status = db_stub_->Delete(&context, request, &response);
  lock.unlock();
  if (status.ok()) {
    return true;
  } else {
    Utils::logToFile("DB Delete Comm Failed");
    throw "";;
  }
}

bool DBService::close() { 
  rpcdb::CloseRequest request;
  rpcdb::CloseResponse response;
  ClientContext context;;

  lock.lock();
  Status status = db_stub_->Close(&context, request, &response);
  lock.unlock();
  if (status.ok()) {
    return true;
  } else {
    Utils::logToFile("DB Close Comm Failed");
    throw "";
  }
}

std::vector<DBEntry> DBService::readBatch(std::string prefix) {
  std::vector<DBEntry> entries;
  rpcdb::NewIteratorWithStartAndPrefixRequest requestNewIterator;
  rpcdb::NewIteratorWithStartAndPrefixResponse responseNewIterator;
  ClientContext contextNewIterator;


  lock.lock();
  requestNewIterator.set_start("");
  requestNewIterator.set_prefix(prefix);

  Status status = db_stub_->NewIteratorWithStartAndPrefix(&contextNewIterator, requestNewIterator, &responseNewIterator);

  if (status.ok()) {
    auto id = responseNewIterator.id();
    while (true) {
      rpcdb::IteratorNextRequest requestIteratorNext;
      rpcdb::IteratorNextResponse responseIteratorNext;
      ClientContext contextIteratorNext;
      requestIteratorNext.set_id(id);

      db_stub_->IteratorNext(&contextIteratorNext, requestIteratorNext, &responseIteratorNext);

      if (responseIteratorNext.data().size() == 0) { break; };
      for (auto entry : responseIteratorNext.data()) {
        entries.push_back({entry.key(), entry.value()});
      }
    }

    rpcdb::IteratorReleaseRequest requestRelease;
    rpcdb::IteratorReleaseResponse responseRelease;
    ClientContext contextRelease;
    requestRelease.set_id(id);

    db_stub_->IteratorRelease(&contextRelease, requestRelease, &responseRelease);    
  } else {
    Utils::logToFile("DB readBatch Comm Failed");
    throw "";
  }

  lock.unlock();
  return entries;
}


bool DBService::writeBatch(WriteBatchRequest &request, std::string prefix) {
  rpcdb::WriteBatchRequest requestWriteBatch;
  rpcdb::WriteBatchResponse responseWriteBatch;
  ClientContext contextWriteBatch;

  for (auto &entry : request.puts) {
    auto putRequest = requestWriteBatch.add_puts();
    putRequest->set_key(entry.key);
    putRequest->set_value(entry.value);
  }

  for (auto &entry : request.dels) {
    auto delRequest = requestWriteBatch.add_deletes();
    delRequest->set_key(entry.key);
  }

  requestWriteBatch.set_id(0);
  requestWriteBatch.set_continues(false);

  lock.lock();
  Status status = db_stub_->WriteBatch(&contextWriteBatch, requestWriteBatch, &responseWriteBatch);
  lock.unlock();

  if (status.ok()) {
    return true;
  } else {
    Utils::logToFile("DB WriteBatch Comm Failed");
    throw "";
  }
}
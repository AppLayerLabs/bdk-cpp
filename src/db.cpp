#include "db.h"


bool DBService::has(std::string key) {
  rpcdb::HasRequest request;
  rpcdb::HasResponse response;
  ClientContext context;

  request.set_key(key);

  Status status = db_stub_->Has(&context, request, &response);
  if (status.ok()) {
    return response.has();
  } else { 
    Utils::logToFile("DB Has Comm Failed");
    throw "";
  }
}

std::string DBService::get(std::string key) {
  rpcdb::GetRequest request;
  rpcdb::GetResponse response;
  ClientContext context;

  request.set_key(key);
  Status status = db_stub_->Get(&context, request, &response);
  if (status.ok()) {
    return response.value();
  } else { 
    Utils::logToFile("DB Get Comm Failed");
    throw "";
  }
}

bool DBService::put(std::string key, std::string value) {
  rpcdb::PutRequest request;
  rpcdb::PutResponse response;
  ClientContext context;

  request.set_key(key);
  request.set_value(value);
  Status status = db_stub_->Put(&context, request, &response);
  if (status.ok()) {
    return true;
  } else { 
    Utils::logToFile("DB Put Comm Failed");
    throw "";
  }
}

bool DBService::del(std::string key) {
  rpcdb::DeleteRequest request;
  rpcdb::DeleteResponse response;
  ClientContext context;

  request.set_key(key);
  Status status = db_stub_->Delete(&context, request, &response);
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

  Status status = db_stub_->Close(&context, request, &response);
  if (status.ok()) {
    return true;
  } else {
    Utils::logToFile("DB Close Comm Failed");
    throw "";
  }
}


#pragma once
#include "request_handler.h"
#include "file_store.h"
#include <regex>
#include <mutex>

class ApiHandler : public RequestHandler {
public:
 // args[0] = mount‐point ("/api"),
 // args[1] = data_path for DiskFileStore
 ApiHandler(const std::string& mount,
           std::shared_ptr<FileStore> store)
    : mount_(mount), store_(std::move(store)) { }

 std::unique_ptr<HttpResponse> handle_request(const HttpRequest& req) override;
 static const std::string kName;
 std::string get_kName() { return kName; };
private:
 std::string mount_;
 std::shared_ptr<FileStore> store_;
 std::mutex store_mutex_; // Mutex for thread safety

 // path pattern: /api/{Entity} or /api/{Entity}/{id}
 bool parse_path(const std::string& p, std::string& entity, std::optional<int>& id);
};
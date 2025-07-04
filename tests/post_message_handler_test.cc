#include <gtest/gtest.h>
#include "post_message_handler.h"
#include "message_store.h"
#include "http_types.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

class PostMessageHandlerTest : public ::testing::Test {
protected:
    fs::path temp_dir;

    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "post_message_test_dir";
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        fs::create_directories(temp_dir);
        fs::path messages_path = temp_dir / "messages";
        if (fs::exists(messages_path)) {
            fs::remove_all(messages_path);
        }

        MessageStore::instance().load_from_file((temp_dir / "empty").string());
    }

    void TearDown() override {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
        MessageStore::instance().load_from_file((temp_dir / "empty").string());
    }
};

// build request
static HttpRequest make_request(const std::string& method,
                                const std::optional<std::string>& user_id,
                                const std::string& body_json = "",
                                const std::string& path = "/messages/post") {
    HttpRequest req;
    req.method = method;
    req.path = path;
    if (user_id.has_value()) {
        req.session_context.user_id = user_id.value();
    }
    req.body = body_json;
    return req;
}

// non post 
TEST_F(PostMessageHandlerTest, NonPostMethod) {
    PostMessageHandler handler(temp_dir.string());
    HttpRequest req = make_request("GET", "alice", R"({"content":"hi"})");
    auto resp = handler.handle_request(req);

    EXPECT_EQ(resp->status_code, 405);
    EXPECT_EQ(resp->headers["Allow"], "POST");
    EXPECT_EQ(resp->body, "Only POST is allowed on /messages/post\n");
}

// no user id 
TEST_F(PostMessageHandlerTest, MissingSession) {
    PostMessageHandler handler(temp_dir.string());
    HttpRequest req = make_request("POST", std::nullopt, R"({"content":"hi"})");
    auto resp = handler.handle_request(req);

    EXPECT_EQ(resp->status_code, 401);
    EXPECT_EQ(resp->body, "User not authenticated\n");
}

// invalid json
TEST_F(PostMessageHandlerTest, InvalidJsonBody) {
    PostMessageHandler handler(temp_dir.string());
    HttpRequest req = make_request("POST", "alice", R"({content: noquotes})");
    auto resp = handler.handle_request(req);

    EXPECT_EQ(resp->status_code, 400);
    EXPECT_EQ(resp->body, "Expected JSON { \"content\": \"<message>\" }\n");
}

// successful and writes to json 
TEST_F(PostMessageHandlerTest, SuccessfulPost) {
    PostMessageHandler handler(temp_dir.string());

    std::string body = R"({"content":"Hello, Test!"})";
    HttpRequest req = make_request("POST", "bob", body);
    auto resp = handler.handle_request(req);

    EXPECT_EQ(resp->status_code, 201);
    EXPECT_EQ(resp->body, "Message stored\n");
    auto all = MessageStore::instance().get_all();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0].username, "bob");
    EXPECT_EQ(all[0].content,  "Hello, Test!");
    EXPECT_FALSE(all[0].timestamp.empty());
}

// consecutive messages
TEST_F(PostMessageHandlerTest, ConsecutivePostsCreateMultipleFiles) {
    PostMessageHandler handler(temp_dir.string());
    HttpRequest req1 = make_request("POST", "alice", R"({"content":"Msg1"})");
    auto resp1 = handler.handle_request(req1);
    EXPECT_EQ(resp1->status_code, 201);
    HttpRequest req2 = make_request("POST", "alice", R"({"content":"Msg2"})");
    auto resp2 = handler.handle_request(req2);
    EXPECT_EQ(resp2->status_code, 201);

    auto all = MessageStore::instance().get_all();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0].content, "Msg1");
    EXPECT_EQ(all[1].content, "Msg2");
}

// works with existing directory
TEST_F(PostMessageHandlerTest, ExistingMessagesDirIsReused) {
    fs::path messages_path = temp_dir / "messages";
    fs::create_directories(messages_path);
    ASSERT_TRUE(fs::exists(messages_path));
    PostMessageHandler handler(temp_dir.string());
    HttpRequest req = make_request("POST", "carol", R"({"content":"Hi"})");
    auto resp = handler.handle_request(req);
    EXPECT_EQ(resp->status_code, 201);
}

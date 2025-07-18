cmake_minimum_required(VERSION 3.10.0)
project(name-not-found-404)

include(FetchContent)

# enable coverage flags
option(ENABLE_COVERAGE "Enable coverage flags" OFF)
if(ENABLE_COVERAGE)
  add_compile_options(-O0 -g --coverage)
  link_libraries(--coverage)
endif()

# Make sure developers do not run cmake in the main project directory
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed.")
endif()

# Turn on debug builds if in dev shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    message(STATUS "Setting debug build type by default in devel shell")
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Debug build: Fetching nlohmann_json from GitHub")
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.2
  )
  FetchContent_MakeAvailable(nlohmann_json)
else()
  message(STATUS "Non-Debug build: finding nlohmann_json from system")
  find_package(nlohmann_json 3.2.0 REQUIRED)
  message(STATUS "nlohmann_json version: ${nlohmann_json_VERSION}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(CTest)
enable_testing()

include(GoogleTest)
add_subdirectory(/usr/src/googletest googletest)

set(Boost_NO_BOOST_CMAKE ON)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system filesystem log log_setup regex)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

# Core libraries
add_library(server_main_lib src/server_main.cc)
add_library(server_lib src/server.cc)
add_library(handler_registry src/handler_registry.cc)
add_library(session_lib src/session.cc)
add_library(session_store_lib src/session_store.cc)
add_library(config_parser_lib src/config_parser.cc)
add_library(request_parser_lib src/request_parser.cc)
add_library(logger_lib src/logger.cc)
add_library(dispatcher_lib src/dispatcher.cc)
add_library(disk_file_store_lib src/disk_file_store.cc)
add_library(fake_file_store_lib src/fake_file_store.cc)
add_library(message_store_lib src/message_store.cc)

# Handler libraries
add_library(echo_handler_lib OBJECT src/echo_handler.cc)
add_library(not_found_handler_lib OBJECT src/not_found_handler.cc)
add_library(static_handler_lib OBJECT src/static_handler.cc)
add_library(api_handler_lib OBJECT src/api_handler.cc)
add_library(health_handler_lib OBJECT src/health_handler.cc)
add_library(sleep_handler_lib OBJECT src/sleep_handler.cc)
add_library(logout_handler_lib OBJECT src/logout_handler.cc)
add_library(session_middleware_handler_lib src/session_middleware_handler.cc)
add_library(get_messages_handler_lib OBJECT src/get_messages_handler.cc)
add_library(register_handler_lib OBJECT src/register_handler.cc)
add_library(login_handler_lib OBJECT src/login_handler.cc)
add_library(post_message_handler_lib OBJECT src/post_message_handler.cc)

# Link dependencies

# Dispatcher links
target_link_libraries(config_parser_lib dispatcher_lib)
target_link_libraries(session_lib dispatcher_lib)
target_link_libraries(echo_handler_lib PUBLIC handler_registry Boost::log Boost::log_setup)
target_link_libraries(post_message_handler_lib PUBLIC session_middleware_handler_lib handler_registry nlohmann_json::nlohmann_json)
target_link_libraries(sleep_handler_lib PUBLIC handler_registry Boost::log Boost::log_setup)
target_link_libraries(static_handler_lib PUBLIC handler_registry)
target_link_libraries(not_found_handler_lib PUBLIC handler_registry)
target_link_libraries(disk_file_store_lib PUBLIC Boost::filesystem)
target_link_libraries(fake_file_store_lib PUBLIC Boost::filesystem)
target_link_libraries(api_handler_lib PUBLIC handler_registry disk_file_store_lib fake_file_store_lib nlohmann_json::nlohmann_json)
target_link_libraries(health_handler_lib PUBLIC handler_registry)
target_link_libraries(logout_handler_lib PUBLIC handler_registry)
target_link_libraries(register_handler_lib PUBLIC handler_registry nlohmann_json::nlohmann_json)
target_link_libraries(post_message_handler_lib PUBLIC handler_registry nlohmann_json::nlohmann_json)
target_link_libraries(message_store_lib PUBLIC
  Boost::system
  Boost::thread
)

target_link_libraries(session_middleware_handler_lib
    PRIVATE
      session_store_lib
      handler_registry
      Boost::log
      Boost::log_setup
      Boost::system
      Boost::regex
)
target_link_libraries(session_store_lib Boost::log Boost::log_setup)
target_link_libraries(login_handler_lib PUBLIC handler_registry nlohmann_json::nlohmann_json)

target_link_libraries(get_messages_handler_lib
  PUBLIC
    handler_registry
    disk_file_store_lib
    nlohmann_json::nlohmann_json
    Boost::log
    Boost::log_setup
    Boost::system
    Boost::filesystem
    session_middleware_handler_lib
)

target_link_libraries(post_message_handler_lib
  PUBLIC
    handler_registry
    nlohmann_json::nlohmann_json
    session_middleware_handler_lib
)

# Main server executable
add_executable(server src/server_main.cc)
target_link_libraries(server logger_lib server_main_lib server_lib 
  session_lib config_parser_lib echo_handler_lib not_found_handler_lib 
  static_handler_lib request_parser_lib Boost::log_setup Boost::log 
  Boost::system Boost::regex api_handler_lib disk_file_store_lib 
  fake_file_store_lib nlohmann_json::nlohmann_json health_handler_lib 
  sleep_handler_lib logout_handler_lib get_messages_handler_lib register_handler_lib login_handler_lib
  session_middleware_handler_lib message_store_lib post_message_handler_lib)

# Tests
add_executable(config_parser_test tests/config_parser_test.cc)
target_link_libraries(config_parser_test config_parser_lib logger_lib echo_handler_lib not_found_handler_lib static_handler_lib Boost::log Boost::log_setup Boost::system gtest_main)

add_executable(post_message_handler_test tests/post_message_handler_test.cc)
target_link_libraries(post_message_handler_test post_message_handler_lib session_middleware_handler_lib message_store_lib pthread Boost::log Boost::log_setup Boost::system gtest_main gmock_main)

add_executable(message_store_test tests/message_store_test.cc)
target_link_libraries(message_store_test message_store_lib pthread Boost::log Boost::log_setup Boost::system gtest_main)

add_executable(not_found_handler_test tests/not_found_handler_test.cc)
target_link_libraries(not_found_handler_test not_found_handler_lib gtest_main gmock_main)

add_executable(echo_handler_test tests/echo_handler_test.cc)
target_link_libraries(echo_handler_test echo_handler_lib request_parser_lib gtest_main gmock_main Boost::log Boost::log_setup)

add_executable(static_handler_test tests/static_handler_test.cc)
target_link_libraries(static_handler_test static_handler_lib request_parser_lib gtest_main)

add_executable(request_parser_test tests/request_parser_test.cc)
target_link_libraries(request_parser_test request_parser_lib gtest_main)

add_executable(api_handler_test tests/api_handler_test.cc)
target_link_libraries(api_handler_test api_handler_lib fake_file_store_lib request_parser_lib gtest_main Boost::system)

add_executable(dispatcher_test tests/dispatcher_test.cc)
target_link_libraries(dispatcher_test dispatcher_lib gtest_main gmock_main)

add_executable(parse_common_api_test tests/parse_common_api_test.cc)
target_link_libraries(parse_common_api_test config_parser_lib echo_handler_lib not_found_handler_lib static_handler_lib Boost::log Boost::log_setup Boost::system gtest_main)

add_executable(server_test tests/server_test.cc)
target_link_libraries(server_test server_lib session_lib request_parser_lib Boost::system Boost::log_setup Boost::log gtest_main)

add_executable(logger_test tests/logger_test.cc)
target_link_libraries(logger_test logger_lib Boost::log Boost::log_setup Boost::system gtest_main)

add_executable(health_handler_test tests/health_handler_test.cc)
target_link_libraries(health_handler_test health_handler_lib gtest_main Boost::log Boost::log_setup Boost::system)

add_executable(logout_handler_test tests/logout_handler_test.cc)
target_link_libraries(logout_handler_test logout_handler_lib session_middleware_handler_lib gtest_main Boost::log Boost::log_setup Boost::system)


add_executable(session_middleware_handler_test tests/session_middleware_test.cc)
target_link_libraries(session_middleware_handler_test
    PRIVATE
      session_middleware_handler_lib
      gtest_main
      gmock_main
)

add_executable(session_store_test tests/session_store_test.cc)
target_link_libraries(session_store_test 
    session_store_lib
    gtest_main
    Boost::log
    Boost::log_setup
    Boost::system
)

add_executable(get_messages_handler_test tests/get_messages_handler_test.cc)
target_link_libraries(get_messages_handler_test
  PRIVATE
    get_messages_handler_lib
    disk_file_store_lib
    handler_registry
    gtest_main
    nlohmann_json::nlohmann_json
    Boost::log
    Boost::log_setup
    Boost::system
    Boost::filesystem
)

add_executable(register_handler_test tests/register_handler_test.cc)
target_link_libraries(register_handler_test register_handler_lib request_parser_lib gtest_main Boost::log Boost::log_setup Boost::system nlohmann_json::nlohmann_json)

add_executable(login_handler_test tests/login_handler_test.cc)
target_link_libraries(login_handler_test login_handler_lib request_parser_lib session_middleware_handler_lib gtest_main Boost::log Boost::log_setup Boost::system nlohmann_json::nlohmann_json)

# tests go under here

# gtest discovery
gtest_discover_tests(config_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(post_message_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(message_store_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(echo_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(not_found_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(static_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(request_parser_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(dispatcher_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(server_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(logger_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(api_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(health_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(logout_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(session_middleware_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(session_store_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(get_messages_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(register_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(login_handler_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# Coverage config
include(cmake/CodeCoverageReportConfig.cmake)
generate_coverage_report(
  TARGETS 
    server
    server_main_lib
    server_lib 
    session_lib 
    session_store_lib
    config_parser_lib 
    echo_handler_lib 
    not_found_handler_lib 
    static_handler_lib
    logger_lib
    dispatcher_lib
    api_handler_lib
    health_handler_lib
    logout_handler_lib
    session_middleware_handler_lib
    get_messages_handler_lib
    register_handler_lib
    login_handler_lib
    message_store_lib
    post_message_handler_lib
  TESTS 
    config_parser_test
    echo_handler_test
    static_handler_test
    not_found_handler_test
    request_parser_test
    server_test
    logger_test
    parse_common_api_test
    dispatcher_test
    api_handler_test
    health_handler_test
    logout_handler_test
    session_middleware_handler_test
    session_store_test
    get_messages_handler_test
    register_handler_test
    login_handler_test
    message_store_test
    post_message_handler_test
  )

# Integration tests
add_test(NAME EchoServerIntegrationTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_test.sh)
add_test(NAME StaticHandlerIntegrationTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/static_integration_test.sh)
add_test(NAME NotFoundHandlerIntegrationTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/not_found_integration_test.sh)
add_test(NAME APIHandlerIntegrationTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/api_integration_test.sh)
add_test(NAME HealthHandlerIntegrationTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/health_handler_integration_test.sh)
add_test(NAME ConcurrentIntegrationTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/concurrent_integration_test.sh)
add_test(NAME LogoutHandlerIntegrationTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/logout_handler_integration_test.sh)

add_test(NAME GetMessagesHandlerIntegrationTest
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/get_messages_handler_integration_test.sh)

add_test(NAME RegisterHandlerIntegrationTest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/register_handler_integration_test.sh)

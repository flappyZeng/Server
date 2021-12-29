# cmake的最小版本
cmake_minimum_required(VERSION 2.8)
project(server) # 定义的项目

include (./utils.cmake)  #用来重定义__FILE__

# make过程中显示详细的make命令
set(CMAKE_VERBOSE_MAKEFILE ON)

#自定义的编译参数
set(CMAKE_CXX_fLAGS "$ENV{CXXFLAGS} -rdynamic -03 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")


include_directories(.)
include_directories(/server/include)
link_directories(/server/lib)

find_library(YAMLCPP yaml-cpp)
find_package(ragel)

# 把编写的源码映射到lib_src上
set(LIB_SRC
    server/address.cpp
    server/bytearray.cpp
    server/config.cpp
    server/fd_manager.cpp
    server/fiber.cpp
    server/hook.cpp
    server/http/http.cpp
    server/http/httpclient_parser_rl.cpp
    server/uri_rl.cpp
    server/http/http11_parser_rl.cpp
    server/http/http_parser.cpp
    server/http/http_server.cpp
    server/http/httpsession.cpp
    server/http/http_connection.cpp
    server/http/servlet.cpp
    server/iomanager.cpp
    server/log.cpp
    server/scheduler.cpp
    server/socket.cpp
    server/socketstream.cpp
    server/stream.cpp
    server/tcp_server.cpp
    server/thread.cpp
    server/timer.cpp
    server/utils.cpp
    )


# 生成shared库
add_library(server SHARED ${LIB_SRC})
force_readfine_file_macro_for_sources(server) # __FILE__

# 生成测试文件mytest
add_executable(test_server tests/mytest.cpp)
add_dependencies(test_server server)
force_readfine_file_macro_for_sources(test_server) # __FILE__
target_link_libraries(test_server server ${YAMLCPP} -lpthread -ldl)  # 加上 -lpthread -luuid，不然没法使用pthread系列函数

#新的单元测试文件test_config
add_executable(test_config tests/test_config.cpp)
add_dependencies(test_config server)
force_readfine_file_macro_for_sources(test_config) # __FILE__
target_link_libraries(test_config server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_thread
add_executable(test_thread tests/test_thread.cpp)
add_dependencies(test_thread server)
force_readfine_file_macro_for_sources(test_thread) # __FILE__
target_link_libraries(test_thread server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_utils
add_executable(test_utils tests/test_utils.cpp)
add_dependencies(test_utils server)
force_readfine_file_macro_for_sources(test_utils) # __FILE__
target_link_libraries(test_utils server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_fiber
add_executable(test_fiber tests/test_fiber.cpp)
add_dependencies(test_fiber server)
force_readfine_file_macro_for_sources(test_fiber) # __FILE__
target_link_libraries(test_fiber server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_schduler
add_executable(test_scheduler tests/test_scheduler.cpp)
add_dependencies(test_scheduler server)
force_readfine_file_macro_for_sources(test_scheduler) # __FILE__
target_link_libraries(test_scheduler server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_iomanager
add_executable(test_iomanager tests/test_iomanager.cpp)
add_dependencies(test_iomanager server)
force_readfine_file_macro_for_sources(test_iomanager) # __FILE__
target_link_libraries(test_iomanager server ${YAMLCPP} -lpthread -ldl)


#新的单元测试文件test_hook
add_executable(test_hook tests/test_hook.cpp)
add_dependencies(test_hook server)
force_readfine_file_macro_for_sources(test_hook) # __FILE__
target_link_libraries(test_hook server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_address
add_executable(test_address tests/test_address.cpp)
add_dependencies(test_address server)
force_readfine_file_macro_for_sources(test_address) # __FILE__
target_link_libraries(test_address server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_socket
add_executable(test_socket tests/test_socket.cpp)
add_dependencies(test_socket server)
force_readfine_file_macro_for_sources(test_socket) # __FILE__
target_link_libraries(test_socket server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_bytearray
add_executable(test_bytearray tests/test_bytearray.cpp)
add_dependencies(test_bytearray server)
force_readfine_file_macro_for_sources(test_bytearray) # __FILE__
target_link_libraries(test_bytearray server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_http
add_executable(test_http tests/test_http.cpp)
add_dependencies(test_http server)
force_readfine_file_macro_for_sources(test_http) # __FILE__
target_link_libraries(test_http server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_http_parser
add_executable(test_http_parser tests/test_http_parser.cpp)
add_dependencies(test_http_parser server)
force_readfine_file_macro_for_sources(test_http_parser) # __FILE__
target_link_libraries(test_http_parser server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_tcpserver
add_executable(test_tcpserver tests/test_tcpserver.cpp)
add_dependencies(test_tcpserver server)
force_readfine_file_macro_for_sources(test_tcpserver) # __FILE__
target_link_libraries(test_tcpserver server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件echo_server
add_executable(echo_server example/echo_server.cpp)
add_dependencies(echo_server server)
force_readfine_file_macro_for_sources(echo_server) # __FILE__
target_link_libraries(echo_server server ${YAMLCPP} -lpthread -ldl)


#新的单元测试文件test_http_server
add_executable(test_http_server tests/test_http_server.cpp)
add_dependencies(test_http_server server)
force_readfine_file_macro_for_sources(test_http_server) # __FILE__
target_link_libraries(test_http_server server ${YAMLCPP} -lpthread -ldl)

#新的单元测试文件test_http_connection
add_executable(test_http_connection tests/test_http_connection.cpp)
add_dependencies(test_http_connection server)
force_readfine_file_macro_for_sources(test_http_connection) # __FILE__
target_link_libraries(test_http_connection server ${YAMLCPP} -lpthread -ldl)


#新的单元测试文件test__uri
add_executable(test_uri tests/test_uri.cpp)
add_dependencies(test_uri server)
force_readfine_file_macro_for_sources(test_uri) # __FILE__
target_link_libraries(test_uri server ${YAMLCPP} -lpthread -ldl)


# 输出的lib的生成路径 
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

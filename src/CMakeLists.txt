add_library(web_server http_server.cc response.cc)

target_include_directories(web_server PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_executable(run run.cc)
target_link_libraries(run PUBLIC web_server)

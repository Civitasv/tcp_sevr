add_library(web_server tcp_socket.cc response.cc)

target_include_directories(web_server PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_executable(run run.cc)
target_link_libraries(run PUBLIC web_server)

add_custom_command(
  TARGET run
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/templates/
          ${CMAKE_BINARY_DIR}/src/templates/
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/static/
          ${CMAKE_BINARY_DIR}/src/static/)

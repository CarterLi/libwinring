add_executable(bench "demo/bench.cpp")
add_executable(cp "demo/cp.c")
add_executable(echoServer "demo/echoServer.cpp")
target_link_libraries(echoServer "ws2_32" "mswsock")

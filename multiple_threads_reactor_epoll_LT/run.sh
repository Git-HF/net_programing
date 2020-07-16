#!/bin/bash

g++ hf_channel.cc hf_event_loop.cc hf_event_loop_in_new_thread.cc hf_epoller.cc hf_thread.cc hf_socket_funcs.cc hf_socket.cc hf_acceptor.cc hf_tcp_connection.cc hf_tcp_server.cc hf_buffer.cc hf_event_loop_thread_pool.cc main.cc -std=c++11 -lpthread

if [ $? -eq 0 ]; then
     echo "-------------------------------start run--------------------------------"
     ./a.out 
else
     echo "failed"
fi

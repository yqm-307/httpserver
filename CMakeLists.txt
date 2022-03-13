cmake_minimum_required(VERSION 2.6)
project(webserver CXX C)

aux_source_directory(./src SRC_FILE)

include_directories(include)

set(EXECUTABLE_OUTPUT_PATH .)

add_compile_options(-g -w)

add_executable(webserver ${SRC_FILE})

target_link_libraries(webserver
    -lpthread
    -lmysqlclient
    -I/usr/include/mysql	
    -L/usr/lib64/mysql
    )



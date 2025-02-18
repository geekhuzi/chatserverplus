aux_source_directory(../log LOG_LIST)

add_executable(procctl procctl.cpp)
add_executable(checkproc checkproc.cpp ${LOG_LIST})

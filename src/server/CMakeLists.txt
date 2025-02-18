add_subdirectory(watchdog)

aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
aux_source_directory(./log LOG_LIST)
aux_source_directory(./muduoCPP11 MUDUO_LIST)
set(WATCHDOG_LIST ./watchdog/watchdog.cpp)

# 错误的,这个关键字只能添加目录
# aux_source_directory(./watchdog/watchdog.cpp WATCHDOG_LIST)


add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST} ${LOG_LIST} ${WATCHDOG_LIST} ${MUDUO_LIST})
# add_executable(Procctl ./watchdog/procctl.cpp)
# add_executable(Checkproc ./watchdog/checkproc.cpp ${LOG_LIST})
link_directories(/usr/local/lib)
target_link_libraries(ChatServer mysqlclient hiredis pthread) # muduoC11
# mysqclclient
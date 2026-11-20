add_executable(tcp_server WIN32 main.cpp serverwindow.cpp serverwindow.h ../protocol.h)
target_link_libraries(tcp_server PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

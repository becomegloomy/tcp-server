add_executable(tcp_client WIN32 main.cpp clientwindow.cpp clientwindow.h ../protocol.h)
target_link_libraries(tcp_client PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

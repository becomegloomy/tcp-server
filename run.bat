REM Для теста - сразу запуск сервера и двух клиентов
@echo off
start "" /B server\tcp_server.exe
start "" /B client\tcp_client.exe
start "" /B client\tcp_client.exe
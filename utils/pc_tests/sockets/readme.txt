  Сборка тестов в данной директории выполняется командой make. При этом будут 
собраны 2 исполняемых файла: server и client - это сервер и клиент, 
предназначенные для запуска на ПК. Для работы тестов необходимо, чтобы на плате 
были запущены соответствующие "ответные части" тестов.
  Суть тестов проста - осуществляется односторонняя передача значений счетчика от 
клиента к серверу. Сервер проверяет, что в принимаемых значениях счетчика нет 
"разрыва", т.е. каждое новое значение счетчика больше предыдущего на 1. При 
этом оценивается скорость передачи в клиенте и скорость приема на сервере. 
Значения счетчика передаются не по одному, а "пачками", размер пачки указан в 
макроопределении BUF_SIZE в файле client.c. Изменение размера пачки, разумеется, 
влияет на получаемые результаты оценки производительности сети.
  В Makefile можно выбрать вид сокетов, используемых для обмена: если в 
переменной CFLAGS указано макроопредение TCP_SOCKET:

	CFLAGS  = -DTCP_SOCKET

то программы будут собраны для работы через сокеты TCP, иначе UDP.

  Запуск тестов. Тест server запускается без параметров:

	server

  Тест client имеет один параметров:

	client <ip-адрес машины, на котором запущен сервер>


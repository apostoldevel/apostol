#### APOSTOL WEB SERVICE
========================

**Apostol Web Service** - Web сервис **Апостол**, исходные коды на C++.



#### СТРУКТУРА КАТАЛОГОВ
------------------------

	  conf/			содержит файлы с настройками
	  src/			содержит файлы с исходным кодом
	    apostol/		содержит файлы с исходным кодом: Apostol Web Service
	    lib/		содержит файлы с исходным кодом библиотек
	      delphi/		содержит файлы с исходным кодом библиотеки: Delphi classes for C++
	    mod/		содержит файлы с исходным кодом дополнений (модификаций)
	      WebServer/	содержит файлы с исходным кодом дополнения: Web-Server
	      Client365/	содержит файлы с исходным кодом дополнения: Клиент 365



#### ОПИСАНИЕ
-------------

**Apostol Web Service** - это платформа для быстрого создания Web сервисов.

**apostol** - приложение *Linux демон* в задачу которого состоит запуск и поддержка работоспособности двух серверов:
 
1. HTTP Server;
2. PostgreSQL Server.

*Сервера построены на базе библиотеки libdelphi с применением **асинхронной** модели программирования.*



###### HTTP Server

**Асинхронный HTTP сервер**.

TCP/IP сервер с поддержкой протокола HTTP.
	


###### PostgreSQL Server

**Асинхронный PostgreSQL сервер**.
	
Сервер создаёт пул подключений к СУБД Postgres и позволяет отправлять SQL запросы в асинхронном режиме.

*Сервер построен на базе библиотеки *libpq* с применением асинхронных команд обработки.*
	


#### УСТАНОВКА
--------------



#### СБОРКА
-----------



#### ЗАПУСК
-----------





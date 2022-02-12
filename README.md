# Апостол (Apostol)

**Апостол** - это HTTP-сервер с прямым доступом к СУБД [PostgreSQL](https://www.postgresql.org/), исходные коды на C++.

* Основная идея заключается в том, чтобы соединить напрямую HTTP-сервер с базой данных исключив из цепочки обработки HTTP-запроса посредников в виде скриптовых языков программирования.


* Основные преимущества:
    * **Автономность**: После сборки Вы получаете полностью готовый к работе бинарный файл в виде системной службы (демона) под ОС Linux.
    * **Скорость**: Запросы к HTTP-серверу и базе данных выполняются на столько быстро на сколько это позволяет операционная система и СУБД.
  
ОПИСАНИЕ
-

**Апостол** реализован в виде фреймворка для разработки серверного программного обеспечения (системных служб) с применением асинхронной модели программирования на базе [epoll API](https://man7.org/linux/man-pages/man7/epoll.7.html) с прямым доступом к СУБД [PostgreSQL](https://www.postgresql.org/) (через библиотеку: libpq) специально для высоко-нагруженных систем.

Фреймворк имеет модульную конструкцию, встроенный HTTP-сервер и PQ-клиент ([PostgreSQL](https://www.postgresql.org/)).

С помощью готовых модулей Апостол можно превратить в:

- [Сервер авторизации](https://github.com/apostoldevel/module-AuthServer) (OAuth 2.0);
- [Сервер приложений](https://github.com/apostoldevel/module-AppServer) (REST API);
- [Сервер сообщений](https://github.com/apostoldevel/process-MessageServer) (SMTP/FCM/API);
- [Веб-сервер](https://github.com/apostoldevel/module-WebServer) (HTTP);
- [Файл сервер](https://github.com/apostoldevel/module-FileServer);
- Proxy-сервер например [Сбербанк эквайринг](https://github.com/apostoldevel/module-SBAcquiring) или [MTS Communicator](https://github.com/apostoldevel/module-M2M);
- [Сервер потоковых данных](https://github.com/apostoldevel/process-StreamServer) (UDP).

Апостол имеет встроенную поддержку WebSocket: [WebSocket API](https://github.com/apostoldevel/module-WebSocketAPI).

Объединив всё выше перечисленное можно создать информационную систему [Апостол CRM](https://github.com/apostoldevel/apostol-crm) или [Центральную систему для станций зарядки электо-автомобилей](https://github.com/apostoldevel/apostol-cs) почему бы и нет ;-).

_С Апостол Ваши возможности ограничены только Вашей фантазий._

СТРУКТУРА КАТАЛОГОВ
-
    auto/                       содержит файлы со скриптами
    cmake-modules/              содержит файлы с модулями CMake
    conf/                       содержит файлы с настройками
    src/                        содержит файлы с исходным кодом
    ├─app/                      содержит файлы с исходным кодом: Apostol
    ├─core/                     содержит файлы с исходным кодом: Apostol Core
    ├─lib/                      содержит файлы с исходным кодом библиотек
    | └─delphi/                 содержит файлы с исходным кодом библиотеки*: Delphi classes for C++
    └─modules/                  содержит файлы с исходным кодом дополнений (модулей)
    www/                        содержит файлы с Веб-сайтом

СБОРКА И УСТАНОВКА
-
Для установки **Апостол** Вам потребуется:

1. Компилятор C++;
1. [CMake](https://cmake.org) или интегрированная среда разработки (IDE) с поддержкой [CMake](https://cmake.org);
1. Библиотека [libpq-dev](https://www.postgresql.org/download) (libraries and headers for C language frontend development);
1. Библиотека [postgresql-server-dev-12](https://www.postgresql.org/download) (libraries and headers for C language backend development).

### Linux (Debian/Ubuntu)

Для того чтобы установить компилятор C++ и необходимые библиотеки на Ubuntu выполните:
~~~
$ sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

###### Подробное описание установки C++, CMake, IDE и иных компонентов необходимых для сборки проекта не входит в данное руководство. 

#### PostgreSQL

Для того чтобы установить PostgreSQL воспользуйтесь инструкцией по [этой](https://www.postgresql.org/download/) ссылке.

###### Параметр `--make` необходим для установки базы данных в первый раз. Далее установочный скрипт можно запускать или без параметров или с параметром `--install`.

Для установки **Апостол** (без Git) необходимо:

1. Скачать **Апостол** по [ссылке](https://github.com/ufocomp/apostol/archive/master.zip);
1. Распаковать;
1. Настроить `CMakeLists.txt` (по необходимости);
1. Собрать и скомпилировать (см. ниже).

Для установки **Апостол** с помощью Git выполните:
~~~
$ git clone https://github.com/ufocomp/apostol.git
~~~

###### Сборка:
~~~
$ cd apostol
$ ./configure
~~~

###### Компиляция и установка:
~~~
$ cd cmake-build-release
$ make
$ sudo make install
~~~

По умолчанию бинарный файл `apostol` будет установлен в:
~~~
/usr/sbin
~~~

Файл конфигурации и необходимые для работы файлы, в зависимости от варианта установки, будут расположены в: 
~~~
/etc/apostol
или
~/apostol
~~~

ЗАПУСК 
-
###### Если `INSTALL_AS_ROOT` установлено в `ON`.

**`apostol`** - это системная служба (демон) Linux. 
Для управления **`apostol`** используйте стандартные команды управления службами.

Для запуска `apostol` выполните:
~~~
$ sudo service apostol start
~~~

Для проверки статуса выполните:
~~~
$ sudo service apostol status
~~~

Результат должен быть **примерно** таким:
~~~
● apostol.service - LSB: starts the apostol web service
   Loaded: loaded (/etc/init.d/apostol; generated; vendor preset: enabled)
   Active: active (running) since Tue 2020-08-25 23:04:53 UTC; 4 days ago
     Docs: man:systemd-sysv-generator(8)
  Process: 6310 ExecStop=/etc/init.d/apostol stop (code=exited, status=0/SUCCESS)
  Process: 6987 ExecStart=/etc/init.d/apostol start (code=exited, status=0/SUCCESS)
    Tasks: 3 (limit: 4915)
   CGroup: /system.slice/apostol.service
           ├─6999 apostol: master process /usr/sbin/apostol
           └─7000 apostol: worker process ("web server")
~~~

### **Управление**.

Управлять **`apostol`** можно с помощью сигналов.
Номер главного процесса по умолчанию записывается в файл `/run/apostol.pid`. 
Изменить имя этого файла можно при конфигурации сборки или же в `apostol.conf` секция `[daemon]` ключ `pid`. 

Главный процесс поддерживает следующие сигналы:

|Сигнал   |Действие          |
|---------|------------------|
|TERM, INT|быстрое завершение|
|QUIT     |плавное завершение|
|HUP	  |изменение конфигурации, запуск новых рабочих процессов с новой конфигурацией, плавное завершение старых рабочих процессов|
|WINCH    |плавное завершение рабочих процессов|	

Управлять рабочими процессами по отдельности не нужно. Тем не менее, они тоже поддерживают некоторые сигналы:

|Сигнал   |Действие          |
|---------|------------------|
|TERM, INT|быстрое завершение|
|QUIT	  |плавное завершение|

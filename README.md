# Апостол (Apostol)

**Апостол** - это HTTP-сервер с прямым доступом к СУБД [PostgreSQL](https://www.postgresql.org/), исходные коды на C++.

* Основная идея заключается в том, чтобы соединить напрямую HTTP-сервер с базой данных исключив из цепочки обработки HTTP-запроса посредников в виде скриптовых языков программирования.


* Основные преимущества:
    * **Автономность**: После сборки Вы получаете полностью готовый к работе бинарный файл в виде системной службы (демона) под ОС Linux;
    * **Скорость**: Запросы к HTTP-серверу и базе данных выполняются на столько быстро на сколько это позволяет операционная система и СУБД;
    * **Пул соединений**: Апостол имеет собственный настраиваемый пул соединений с PostgreSQL.

ОПИСАНИЕ
-

**Апостол** реализован в виде фреймворка для разработки серверного программного обеспечения (системных служб) с применением асинхронной модели программирования на базе [epoll API](https://man7.org/linux/man-pages/man7/epoll.7.html) с прямым доступом к СУБД [PostgreSQL](https://www.postgresql.org/) (через библиотеку: libpq) специально для высоко-нагруженных систем.

МОДУЛИ
-

Фреймворк имеет модульную конструкцию, встроенный HTTP-сервер и PQ-клиент ([PostgreSQL](https://www.postgresql.org/)).

### Данная сборка поставляется двумя модулями:

- [WebServer](https://github.com/apostoldevel/module-WebServer) (Веб-сервер);
    * Обеспечивает работу [Swagger UI](https://swagger.io/tools/swagger-ui) который будет доступен по адресу http://localhost:8080 в вашем браузере после запуска **Апостол**.


- [PQFetch](https://github.com/apostoldevel/module-PQFetch) (Postgres Fetch);
    * Предоставляет возможность принимать и отправлять HTTP-запросы на языке программирования PL/pqSQL.

### С помощью дополнительных модулей Апостол можно превратить в:

- [Сервер авторизации](https://github.com/apostoldevel/module-AuthServer) (OAuth 2.0);
- [Сервер приложений](https://github.com/apostoldevel/module-AppServer) (REST API);
- [Сервер сообщений](https://github.com/apostoldevel/process-MessageServer) (SMTP/FCM/API);
- [Файл сервер](https://github.com/apostoldevel/module-FileServer);
- Proxy-сервер например [Сбербанк эквайринг](https://github.com/apostoldevel/module-SBAcquiring) или [MTS Communicator](https://github.com/apostoldevel/module-M2M);
- [Сервер потоковых данных](https://github.com/apostoldevel/process-StreamServer) (UDP).

**Апостол** имеет встроенную поддержку WebSocket: [WebSocket API](https://github.com/apostoldevel/module-WebSocketAPI).

Объединив всё выше перечисленное можно создать информационную систему [Апостол CRM](https://github.com/apostoldevel/apostol-crm) или [Центральную систему для станций зарядки электо-автомобилей](https://github.com/apostoldevel/apostol-cs) почему бы и нет ;-).

_С Апостол Ваши возможности ограничены только Вашей фантазий._

Docker
-

Вы можете собрать образ самостоятельно или получить уже готовый из докер-хаб:

### Собрать

~~~
docker build -t apostol .
~~~

### Получить

~~~
docker pull apostoldevel/apostol
~~~

### Запустить

Если собрали самомтоятельно:
~~~
docker run -d -p 8080:8080 --rm --name apostol apostol
~~~

Если получили готовый образ:
~~~
docker run -d -p 8080:8080 --rm --name apostol apostoldevel/apostol
~~~

Swagger UI будет доступен по адресу http://localhost:8080 или http://host-ip:8080 в вашем браузере.

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
1. Библиотека [postgresql-server-dev-all](https://www.postgresql.org/download) (libraries and headers for C language backend development).

### Linux (Debian/Ubuntu)

Для того чтобы установить компилятор C++ и необходимые библиотеки на Ubuntu выполните:
~~~
sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

###### Подробное описание установки C++, CMake, IDE и иных компонентов необходимых для сборки проекта не входит в данное руководство.

#### PostgreSQL

Для того чтобы установить PostgreSQL воспользуйтесь инструкцией по [этой](https://www.postgresql.org/download/) ссылке.

#### База данных

Для того чтобы установить базу данных необходимо выполнить:

1. Прописать наименование базы данных в файле db/sql/sets.conf (по умолчанию: web)
1. Прописать пароли для пользователей СУБД [libpq-pgpass](https://postgrespro.ru/docs/postgrespro/13/libpq-pgpass):
   ~~~
   $ sudo -iu postgres -H vim .pgpass
   ~~~
   ~~~
   *:*:*:http:http
   ~~~
1. Указать в файле настроек `/etc/postgresql/{version}/main/pg_hba.conf`:
   ~~~
   # TYPE  DATABASE        USER            ADDRESS                 METHOD
   local	web		http					md5
   ~~~
1. Применить настройки:
   ~~~
   $ sudo pg_ctlcluster <version> main reload
   ~~~   
1. Выполнить:
   ~~~
   $ cd db/
   $ ./install.sh --make
   ~~~

###### Параметр `--make` необходим для установки базы данных в первый раз. Далее установочный скрипт можно запускать или без параметров или с параметром `--install`.

Для установки **Апостол** (без Git) необходимо:

1. Скачать **Апостол** по [ссылке](https://github.com/ufocomp/apostol/archive/master.zip);
1. Распаковать;
1. Настроить `CMakeLists.txt` (по необходимости);
1. Собрать и скомпилировать (см. ниже).

Для установки **Апостол** с помощью Git выполните:
~~~
git clone https://github.com/ufocomp/apostol.git
~~~

###### Сборка:
~~~
cd apostol
./configure
~~~

###### Компиляция и установка:
~~~
cd cmake-build-release
make
sudo make install
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
sudo systemctl start apostol
~~~

Для проверки статуса выполните:
~~~
sudo systemctl status apostol
~~~

Результат должен быть **примерно** таким:
~~~
● apostol.service - Apostol
     Loaded: loaded (/etc/systemd/system/apostol.service; enabled; vendor preset: enabled)
     Active: active (running) since Sat 2019-04-06 00:00:00 MSK; 3y ago
    Process: 461158 ExecStartPre=/usr/bin/rm -f /run/apostol.pid (code=exited, status=0/SUCCESS)
    Process: 461160 ExecStartPre=/usr/sbin/apostol -t (code=exited, status=0/SUCCESS)
    Process: 461162 ExecStart=/usr/sbin/apostol (code=exited, status=0/SUCCESS)
   Main PID: 461163 (apostol)
      Tasks: 2 (limit: 77011)
     Memory: 2.6M
        CPU: 44ms
     CGroup: /system.slice/apostol.service
             ├─461163 apostol: master process /usr/sbin/apostol
             └─461164 apostol: worker process ("pq fetch", "web server")
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
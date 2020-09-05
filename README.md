# Апостол (Apostol)

**Апостол** - это платформа для создания системного программного обеспечения, исходные коды на C++.

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

ОПИСАНИЕ
-

Платформа написана на языке программирования C++, имеет модульную конструкцию и включает в себя встроенную поддержку СУБД [PostgreSQL](https://www.postgresql.org/).

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

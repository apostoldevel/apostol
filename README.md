# Апостол (Apostol)

**Апостол** - это платформа для создания системного программного обеспечения, исходные коды на C++.

СТРУКТУРА КАТАЛОГОВ
-
    auto/                       содержит файлы со скриптами
    cmake-modules/              содержит файлы с модулями CMake
    conf/                       содержит файлы с настройками
    db/                         содержит файлы со скриптами базы данных
    ├─bin/                      содержит исполняемые файлы для автоматизации установки базы данных
    ├─scripts/                  содержит файлы со скриптами для автоматизации установки базы данных
    ├─sql/                      содержит файлы со скриптами базы данных
    | └─kernel/                 содержит файлы со скриптами базы данных: Ядро
    src/                        содержит файлы с исходным кодом
    ├─app/                      содержит файлы с исходным кодом: Apostol
    ├─core/                     содержит файлы с исходным кодом: Apostol Core
    ├─lib/                      содержит файлы с исходным кодом библиотек
    | └─delphi/                 содержит файлы с исходным кодом библиотеки*: Delphi classes for C++
    ├─workers/                  содержит файлы с исходным кодом дополнений (модулей)
    | └─WebService/             содержит файлы с исходным кодом дополнения: Веб-сервис
    ├─helpers/                  содержит файлы с исходным кодом дополнений (модулей)
    | └─CertificateDownloader/  содержит файлы с исходным кодом дополнения: Загрузчик сертификатов
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
1. Библиотека [postgresql-server-dev-10](https://www.postgresql.org/download) (libraries and headers for C language backend development).
1. Библиотека [libdelphi](https://github.com/ufocomp/libdelphi) (Delphi classes for C++);
1. Библиотека [apostol-core](https://github.com/ufocomp/apostol-core) (Apostol Core C++);

###### **ВНИМАНИЕ**: Устанавливать `libdelphi` не нужно, достаточно скачать и разместить в каталоге `src/lib` проекта.

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
1. Скачать **libdelphi** по [ссылке](https://github.com/ufocomp/libdelphi/archive/master.zip);
1. Распаковать в `src/lib/delphi`;
1. Скачать **apostol-core** по [ссылке](https://github.com/ufocomp/apostol-core/archive/master.zip);
1. Распаковать в `src/core`;
1. Настроить `CMakeLists.txt` (по необходимости);
1. Собрать и скомпилировать (см. ниже).

Для установки **Апостол** с помощью Git выполните:
~~~
$ git clone https://github.com/ufocomp/apostol.git
~~~

Чтобы добавить **libdelphi** в проект с помощью Git выполните:
~~~
$ cd apostol/src/lib
$ git clone https://github.com/ufocomp/libdelphi.git delphi
$ cd ../../../
~~~

Чтобы добавить **apostol-core** в проект с помощью Git выполните:
~~~
$ cd apostol/src
$ git clone https://github.com/ufocomp/apostol-core.git core
$ cd ../../
~~~

###### Сборка:
~~~
$ cd apostol
$ cmake -DCMAKE_BUILD_TYPE=Release . -B cmake-build-release
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
● apostol.service - LSB: starts the apostol web servcie
   Loaded: loaded (/etc/init.d/apostol; generated; vendor preset: enabled)
   Active: active (running) since Thu 2019-08-15 14:11:34 BST; 1h 1min ago
     Docs: man:systemd-sysv-generator(8)
  Process: 16465 ExecStop=/etc/init.d/apostol stop (code=exited, status=0/SUCCESS)
  Process: 16509 ExecStart=/etc/init.d/apostol start (code=exited, status=0/SUCCESS)
    Tasks: 3 (limit: 4915)
   CGroup: /system.slice/apostol.service
           ├─16520 apostol: master process /usr/sbin/abc
           └─16521 apostol: worker process
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

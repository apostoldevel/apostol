Apostol Web Service
=

Веб-сервис **Апостол**, исходные коды на C++.

СТРУКТУРА КАТАЛОГОВ
-

    auto/             содержит файлы со скриптами
    conf/             содержит файлы с настройками
    doc/              содержит файлы с документацией
    ├─www/            содержит файлы с документацией в формате html
    src/              содержит файлы с исходным кодом
    ├─apostol/        содержит файлы с исходным кодом: Apostol Web Service
    ├─core/           содержит файлы с исходным кодом: Apostol Core
    ├─lib/            содержит файлы с исходным кодом библиотек
    | └─delphi/       содержит файлы с исходным кодом библиотеки: Delphi classes for C++
    ├─modules/        содержит файлы с исходным кодом дополнений (модулей)
    | ├─WebServer/    содержит файлы с исходным кодом дополнения: Web-Server
    | ├─Exchange/     содержит файлы с исходным кодом дополнения: Exchange (Криптобиржа)
    | └─Client365/    содержит файлы с исходным кодом дополнения: Клиент 365
    └─utils/          содержит файлы с исходным кодом вспомогательных функций

ОПИСАНИЕ
-

**Apostol Web Service** (AWS) - платформа, написанная на C++, для быстрого создания **RESTful API** 
сервисов под Linux, со встроенной поддержкой СУБД [PostgreSQL](https://www.postgresql.org/).

Платформа **AWS** построена на базе библиотеки **`libdelphi`** с применением **асинхронной** модели программирования.

Бинарный файл **`apostol`** - системная служба Linux (демон) в задачу которого входит запуск и поддержка 
работоспособности двух внутренних служб:

1. Server (HTTP-сервер);
1. PQServer (PostgreSQL-сервер).

###### При реализации **apostol**, вдохновение черпалось из исходных кодов [nginx](http://nginx.org). Поэтому управление **apostol** схоже с [управлением nginx](http://nginx.org/ru/docs/control.html#reconfiguration), как следствие производительность и отказоустойчивость **apostol** на уровне [nginx](http://nginx.org).

### Server

#### **Асинхронный HTTP сервер**.

Принимает HTTP запросы от клиентов и в зависимости от значения в заголовке `User-Agent` распределяет их между модулями 
(модификациями). При отсутствии заголовка `User-Agent` или должного, в нём, значения направляет запрос в модуль 
**`Web-Server`** превращая AWS в **Веб-сервер**.

###### Асинхронная работа сервера реализована с помощью `epoll API`. 

### PQServer

#### **Асинхронный PostgreSQL сервер**.
	
Сервер создаёт пул подключений к СУБД Postgres и позволяет отправлять SQL запросы в **асинхронном** режиме.
Указать диапазон минимального и максимального количества подключений к СУБД можно в файле конфигурации в секции `[postgres]`

###### Сервер построен на базе библиотеки `libpq` с применением асинхронных команд.
	
МОДУЛИ
-

Апостол спроектирован таким образом, что основной код отвечает:
- за работу приложения в качестве системной службы;
- реализацию протоколов TCP/IP, HTTP;
- взаимодействие с СУБД Postgres. 

Код реализующий логику того или иного сервиса находится отдельно от основного кода в расширениях (модулях). 

Модуль - это класс C++ принимающий на входе данные от HTTP-сервера и PQ-сервера (PostgeSQL). Программисту, 
который реализует логику Веб-сервиса, не нужно вникать в особенности работы мультиплексированного ввода-вывода, 
функций обработки сигналов Linux (для построения демона) и тонкостях реализации асинхронной работы с PostgreSQL. 
Апостол всё это скрывает, позволяя разработчику сконцентрировать своё внимание только на реализации логики самого 
Веб-сервиса, разрабатывая код в привычном для него стиле. Точками соприкосновения с Апостол будут только входящие 
и исходящие структуры данных.

## Чтобы создать новый модуль нужно выполнить:

1. Собрать проект, как описано ниже;
1. В директории mod/ создать папку с названием Вашего модуля;
1. В директории с названием Вашего модуля создать, с аналогичным названием, файлы MyModuleName.cpp/.hpp;
1. В заголовочном файле MyModuleName.hpp объявить класс C++ производный от класса `CApostolModule`:
1. В `Mudules.hpp`, после класса `CApostolModule`, добавьте заголовочный файл Вашего модуля `#include "MyModuleName/MyModuleName.hpp"` 
1. В `Mudules.hpp` найдите функцию `CreateModule` и добавьте условие вызова Вашего модуля согласно значения в 
`User-Agent`.

Всё!

Пример объявления класса `class MyModuleName`:
~~~
        class MyModuleName: public CApostolModule {
        private:

        protected:

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override {
            
            };
           
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override {
           
            };

        public:

            MyModuleName(): CApostolModule() {};

            ~MyModuleName() override = default;

            static class MyModuleName *CreateModule() {
                return new MyModuleName();
            }

            void Execute(CHTTPConnection *AConnection) override {
            
            };

        };
~~~ 

СБОРКА
-

Для сборки проекта Вам потребуется:

1. Компилятор C++;
1. [CMake](https://cmake.org) или интегрированная среда разработки (IDE) с поддержкой [CMake](https://cmake.org);
1. Библиотека [libpq-dev](https://www.postgresql.org/download/) (libraries and headers for C language frontend development).
1. Библиотека [postgresql-server-dev-10](https://www.postgresql.org/download/) (libraries and headers for C language backend development)

Для того чтобы установить компилятор C++ и необходимые библиотеки на Ubuntu выполните:
~~~
sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

###### Подробное описание установки C++, CMake, IDE и иных компонентов необходимых для сборки проекта не входит в данное руководство. 

Для сборки Апостол, необходимо:

1. Скачать Апостол по [ссылке](https://github.com/ufocomp/apostol/archive/master.zip);
1. Распаковать;
1. Скомпилировать (см. ниже).

Для сборки Апостол, с помощью Git выполните:
~~~
git clone https://github.com/ufocomp/apostol.git
~~~

###### Сборка:
~~~
cd apostol
cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" . -B cmake-build-release
cmake --build cmake-build-release --target apostol -- -j 4
~~~

УСТАНОВКА
-

Для установки **Апостол** нужно выполнить:
~~~
sudo cmake --build cmake-build-release --target install -- -j 4
~~~

По умолчанию **Апостол** будет установлен в:
~~~
/usr/sbin
~~~

Файл конфигурации и необходимые для работы файлы будут расположены в: 
~~~
/etc/apostol
~~~

ЗАПУСК
-

Апостол - системная служба (демон) Linux. 
Для управления Апостол используйте стандартные команды управления службами.

Для запуска Апостол выполните:
~~~
sudo systemctl start apostol
~~~

Для проверки статуса выполните:
~~~
sudo systemctl status apostol
~~~

Результат должен быть примерно таким:
~~~
sudo systemctl status apostol
● apostol.service - LSB: starts the apostol web service
   Loaded: loaded (/etc/init.d/apostol)
   Active: active (running) since Sun 2019-04-07 01:42:54 MSK; 21h ago
   CGroup: /system.slice/apostol.service
           ├─26772 apostol: master process /usr/sbin/apostol
           └─26773 apostol: worker process (apostol)
~~~

### **Управление apostol**.

Управлять **`apostol`** можно с помощью сигналов.
Номер главного процесса по умолчанию записывается в файл `/usr/local/apostol/logs/apostol.pid`. 
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

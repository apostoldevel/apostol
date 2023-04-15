[![ru](https://img.shields.io/badge/lang-ru-green.svg)](https://github.com/apostoldevel/apostol/blob/master/README.ru-RU.md)

# Apostol

**Apostol** is a framework for developing server-side (backend) applications, with source code in C++.

**Apostol** is an HTTP server with direct access to the [PostgreSQL](https://www.postgresql.org/).

* The main idea is to connect the HTTP server directly to the database, eliminating intermediary in the form of script programming languages from the `HTTP request` processing chain.


* The main advantages:
    * **Autonomy**: After building, you get a fully ready-to-use binary file in the form of a system service (daemon) under Linux OS;
    * **Speed**: Queries to the HTTP server and the database are executed as fast as the operating system and DBMS allow;
    * **Connection pool**: Apostol has its own customizable connection pool with [PostgreSQL](https://www.postgresql.org/).
     
DESCRIPTION
-
**Apostol** is implemented as a framework for developing server software (system services) using an asynchronous programming model based on the [epoll API](https://man7.org/linux/man-pages/man7/epoll.7.html) with direct access to the [PostgreSQL](https://www.postgresql.org/) (through the library: `libpq`), specifically for highly loaded systems.

MODULES
-

The framework has a modular design, built-in HTTP server, and [PostgreSQL](https://www.postgresql.org/) client.

### This build comes with two modules:

- [WebServer](https://github.com/apostoldevel/module-WebServer) (Web server);
    * Provides the [Swagger UI](https://swagger.io/tools/swagger-ui) which will be available at [http://localhost:8080](http://localhost:8080) in your browser after launching **Apostol**.


- [PGFetch](https://github.com/apostoldevel/module-PGFetch) (Postgres Fetch);
    * Enables receiving and sending `HTTP requests` in `the PL/pgSQL` programming language.
  
### With additional modules, Apostol can be turned into:

- [AuthServer](https://github.com/apostoldevel/module-AuthServer) (Authorization server OAuth 2.0);
- [AppServer](https://github.com/apostoldevel/module-AppServer) (Application server);
- [MessageServer](https://github.com/apostoldevel/process-MessageServer) (Message server: SMTP/FCM/API);
- [FileServer](https://github.com/apostoldevel/module-FileServer) (File server);
- [Replication](https://github.com/apostoldevel/process-Replication) (Database replication);
- [StreamServer](https://github.com/apostoldevel/process-StreamServer) (Streaming data server).

Apostol has built-in WebSocket support: [WebSocket API](https://github.com/apostoldevel/module-WebSocketAPI).

Combining all the above, you can create an information system [CRM System](https://github.com/apostoldevel/apostol-crm), [Central system for charging points](https://github.com/apostoldevel/apostol-cs) or [Telegram bot on PL/pgSQL](https://github.com/apostoldevel/apostol-pgtg) why not ;-).

_With Apostol your possibilities are only limited by your imagination._

Projects
-

Projects implemented at **Apostol**:

* [OCPP CSS](https://ocpp-css.com) (Central System as Service for Charging Points)
* [BitDeals](https://testnet.bitdeals.org/info/about) (Bitcoin payment processing service)
* [Fenomy](https://fenomy.com) (Ecosystem of personal and commercial security)
* [Ship Safety ERP](https://ship-safety.ru) (ERP system for organization of production activities of the shipping company)
* [PlugMe](https://plugme.ru) (CRM system for charging stations and owners of electric vehicles)
* [DEBT Master](https://debt-master.ru) (A system for automating debt collection)

Docker
-

You can build an image yourself or get a ready-made one from Docker Hub:

### Build

~~~
docker build -t apostol .
~~~

### Get

~~~
docker pull apostoldevel/apostol
~~~
### Run

If you built it yourself:
~~~
docker run -d -p 8080:8080 --rm --name apostol apostol
~~~

If you got a ready-made image:
~~~
docker run -d -p 8080:8080 --rm --name apostol apostoldevel/apostol
~~~

Swagger UI will be available at [http://localhost:8080](http://localhost:8080) or http://host-ip:8080 in your browser.

DIRECTORY STRUCTURE
-
    auto/                       contains scripts files
    cmake-modules/              contains CMake modules files
    conf/                       contains configuration files
    src/                        contains source code files
    ├─app/                      contains source code files: Apostol
    ├─core/                     contains source code files: Apostol Core
    ├─lib/                      contains source code files for libraries
    | └─delphi/                 contains source code files for the library*: Delphi classes for C++
    └─modules/                  contains source code files for add-ons (modules)
    www/                        contains files for the website

BUILD AND INSTALLATION
-
To install **Apostol**, you will need:

1. C++ compiler;
2. [CMake](https://cmake.org) or an Integrated Development Environment (IDE) with [CMake](https://cmake.org) support;
3. [libpq-dev](https://www.postgresql.org/download) library (libraries and headers for C language frontend development);
4. [postgresql-server-dev-all](https://www.postgresql.org/download) library (libraries and headers for C language backend development).

### Linux (Debian/Ubuntu)

To install the C++ compiler and necessary libraries on Ubuntu, run:
~~~
sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

###### A detailed description of how to install C++, CMake, IDE, and other components required for the project build is not included in this guide.

#### PostgreSQL

To install PostgreSQL, use the instructions at [this](https://www.postgresql.org/download/) link.

#### Database

To install the database, you need to perform the following steps:

1. Specify the name of the database in the db/sql/sets.conf file (by default: web)
1. Specify the passwords for the DBMS users [libpq-pgpass](https://postgrespro.ru/docs/postgrespro/13/libpq-pgpass):
   ~~~
   $ sudo -iu postgres -H vim .pgpass
   ~~~
   ~~~
   *:*:*:http:http
   ~~~
1. Specify in the configuration file /etc/postgresql/{version}/main/pg_hba.conf:
   ~~~
   # TYPE  DATABASE        USER            ADDRESS                 METHOD
   local  web    http          md5
   ~~~
1. Apply the settings:
   ~~~
   $ sudo pg_ctlcluster <version> main reload
   ~~~   
1. Execute:
   ~~~
   $ cd db/
   $ ./install.sh --make
   ~~~

###### The --make parameter is required to install the database for the first time. After that, the installation script can be run without parameters or with the --install parameter.

To install **Apostol** (without Git), you need to:

1. Download [Apostol](https://github.com/apostoldevel/apostol/archive/master.zip);
2. Unpack it;
3. Configure CMakeLists.txt (if necessary);
4. Build and compile (see below).

To install Apostol using Git, execute:
~~~
git clone https://github.com/apostoldevel/apostol.git
~~~

###### Build:
~~~
cd apostol
./configure
~~~

###### Compilation and installation:
~~~
cd cmake-build-release
make
sudo make install
~~~

By default, the apostol binary will be installed in:
~~~
/usr/sbin
~~~

The configuration file and files required for operation, depending on the installation option, will be located in:
~~~
/etc/apostol
or
~/apostol
~~~

LAUNCH
-
###### If `INSTALL_AS_ROOT` is set to ON.

`apostol` is a Linux system service (daemon).
To manage `apostol`, use standard service management commands.

To launch `apostol`, execute:
~~~
sudo systemctl start apostol
~~~

To check the status, execute:
~~~
sudo systemctl status apostol
~~~

The result should be something like this:
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

MANAGEMENT
-

`apostol` can be managed using signals.
The main process number is written by default to the `/run/apostol.pid` file.
You can change the name of this file during the build configuration or in the `apostol.conf` `[daemon]` section with the `pid` key.

The main process supports the following signals:

|Signal   |Action            |
|---------|------------------|
|TERM, INT|fast shutdown     |
|QUIT     |graceful shutdown |
|HUP      |configuration change, launching new worker processes with new configuration, graceful shutdown of old worker processes|
|WINCH    |graceful shutdown of worker processes|

There is no need to manage worker processes individually. Nevertheless, they also support some signals:

|Signal   |Action            |
|---------|------------------|
|TERM, INT|fast shutdown     |
|QUIT     |graceful shutdown |
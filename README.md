Apostol Web Service
=

**Apostol Web Service**, C++ source codes.

Structure
-

    auto/             contains script files
    conf/             contains settings files
    doc/              contains documentation files
    ├─www/            contains html documentation files
    src/              contains source files
    ├─app/            contains source files: Apostol Web Service
    ├─core/           contains source files: Apostol Core
    ├─lib/            contains library source files
    └─modules/        contains files with source code of modules (add-ons)
      └─WebServer/    contains add-on source files: Web-Server

Overview
-

**Apostol Web Service** (AWS) - C++ Framework for quick creation **RESTful API**
services for Linux, with integrated [PostgreSQL](https://www.postgresql.org/) DBMS support.

**AWS** is built on the basis of the library **`libdelphi`** using the **asynchronous** programming model.

Binary file **`apostol`** - Linux system service (daemon), whose task is to start two internal services:

1. HTTP server (Server);
1. PostgreSQL server (PQServer).

###### When implementing **Apostol**, inspiration was taken from the source code [nginx](http://nginx.org). Therefore, managing the **Apostol** is like [managing nginx](http://nginx.org/en/docs/control.html#reconfiguration), which leads to the performance and fault tolerance of the **Apostol** no worse than [nginx](http://nginx.org).

#### **HTTP Server**.

The HTTP server accepts requests from clients and, depending on the value in the `User-Agent` header, distributes them between the modules (add-ons). If the header or value of the `User-Agent` is missing, sends a request to the module **`WebServer`** turns AWS into **Web Server**.

###### Asynchronous server operation is performed using `epoll API`. 

#### **PostgreSQL server**.
	
The server creates a connection pool with the PostgreSQL DBMS and allows sending SQL queries in **asynchronous** mode. You can specify the range of minimum and maximum number of connections to the DBMS in the configuration file in the section `[postgres]`.

###### PostgreSQL support is disabled by default.
	
Modules
-

The apostle is arranged so that the main code answers:
- for use as a system service;
- implementation of the TCP / IP, HTTP protocols;
- interaction with the PostgreSQL DBMS.

The code that implements the service logic is located separately from the main code in modules (add-ons).

Settings
-

###### CMake configuration options

In the file `CMakeLists.txt`, specify the phages:

Boolean flag **WITH_POSTGRESQL** can be used to enable PostgreSQL support. The default value is **OFF**.

Boolean flag **WITH_SQLITE3** can be used to enable sqlite3 support. The default value is **OFF**.

Build and installing
-

To build you need:

1. The compiler C++;
1. [CMake](https://cmake.org);
1. The library [libdelphi](https://github.com/ufocomp/libdelphi/) (Delphi classes for C++);

###### **ATTENTION**: You do not need to install `libdelphi`, just download and put it in the `src/lib` directory of the project.

If PostgreSQL support is enabled, you will need:

1. The library [libpq-dev](https://www.postgresql.org/download/) (libraries and headers for C language frontend development);
1. The library [postgresql-server-dev-10](https://www.postgresql.org/download/) (libraries and headers for C language backend development).

To install the C++ compiler and necessary libraries in Ubuntu, run:
~~~
sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

To install PostgreSQL, use the instructions for [this](https://www.postgresql.org/download/) link.

###### A detailed description of the installation of C++, CMake, IDE, and other components necessary for building the project is not included in this guide. 

To install (without Git) you need:

1. Download [Apostol](https://github.com/ufocomp/apostol/archive/master.zip);
1. Unpack;
1. Download [libdelphi](https://github.com/ufocomp/libdelphi/archive/master.zip);
1. Unpack in `src/lib/delphi`;
1. Configure `CMakeLists.txt` (if necessary);
1. Build and compile (see below).

To install (with Git) you need:
~~~
git clone https://github.com/ufocomp/apostol.git
~~~

To add **libdelphi** to the project, using Git, do:
~~~
cd apostol/src/lib
git clone https://github.com/ufocomp/libdelphi.git delphi
cd ../../../
~~~

###### Build:
~~~
cd apostol
cmake -DCMAKE_BUILD_TYPE=Release . -B cmake-build-release
~~~

###### Make and install:
~~~
cd cmake-build-release
make
sudo make install
~~~

By default **apostle** will be set to:
~~~
/usr/sbin
~~~

The configuration file and the necessary files will be located in: 
~~~
/etc/apostol
~~~

Run
-

**`apostol`** - Linux system service (daemon).

To manage **Apostol**, use the standard service management commands.

To start **Apostol**, run:
~~~
sudo service apostol start
~~~

To check the status, run:
~~~
sudo service apostol status
~~~

The result should be something like this:
~~~
● apostol.service - LSB: starts the Apostol Web Service
   Loaded: loaded (/etc/init.d/apostol)
   Active: active (running) since Wed 2019-01-02 03:04:05 MSK; 21h ago
   CGroup: /system.slice/apostol.service
           ├─26772 apostol: master process /usr/sbin/apostol
           └─26773 apostol: worker process (apostol)
~~~

### Controlling **Apostol**.

**`apostol`** can be controlled with signals.

The process ID of the master process is written to the file `/usr/local/apostol/logs/apostol.pid` by default.
This name may be changed at configuration time, or in the section `apostol.conf` of the key `[daemon]` key `pid`.

The master process supports the following signals:

| Signal | Action |
| --------- | ------------------ |
| TERM, INT | fast shutdown |
| QUIT | graceful shutdown |
| HUP | changing configuration, starting new worker processes with a new configuration, graceful shutdown of old worker processes |
| WINCH | graceful shutdown of worker processes |

Individual worker processes can be controlled with signals as well, though it is not required. The supported signals are:

| Signal | Action |
| --------- | ------------------ |
| TERM, INT | fast shutdown |
| QUIT | graceful shutdown |

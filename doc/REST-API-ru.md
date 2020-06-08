# API Apostol Web Service.

## Общая информация
 * Базовая конечная точка (endpoint): [localhost:8080](http://localhost:8080)
 * Все конечные точки возвращают `JSON-объект`
 * Все поля, относящиеся ко времени и меткам времени, указаны в **миллисекундах**. 

## HTTP коды возврата
 * HTTP `4XX` коды возврата применимы для некорректных запросов - проблема на стороне клиента.
 * HTTP `5XX` коды возврата используются для внутренних ошибок - проблема на стороне сервера. Важно **НЕ** рассматривать это как операцию сбоя. Статус выполнения **НЕИЗВЕСТЕН** и может быть успешным.
 
## Коды ошибок
 * Любая конечная точка может вернуть ошибку.
  
**Пример ответа:**
```json
{
  "error": {
    "code": 404,
    "message": "Not Found"
  }
}
```

## Общая информация о конечных точках
 * Для `GET` конечных точек параметры должны быть отправлены в виде `строки запроса (query string)` .
 * Для `POST` конечных точек, некоторые параметры могут быть отправлены в виде `строки запроса (query string)`, а некоторые в виде `тела запроса (request body)`:
 * При отправке параметров в виде `тела запроса` допустимы следующие типы контента:
    * `application/x-www-form-urlencoded` для `query string`;
    * `multipart/form-data` для `HTML-форм`;
    * `application/json` для `JSON`.
 * Параметры могут быть отправлены в любом порядке.
 
## Конечные точки
### Тест подключения
 
```http request
GET /api/v1/ping
```
Проверить подключение к REST API.
 
**Параметры:**
НЕТ
 
**Пример ответа:**
```json
{}
```
 
### Проверить время сервера
```http request
GET /api/v1/time
```
Проверить подключение к REST API и получить текущее время сервера.
 
**Параметры:**
 НЕТ
  
**Пример ответа:**
```json
{
  "serverTime": 1583495795455
}
```

## Авторизованные конечные точки
 
 * Авторизованные конечные точки передаются HTTP методом `POST`.
 
#### Аутентификация 

Аутентификация пользователя в системе возможна одним из следующих способов:

  - По паре: **Имя пользователя** (`username`) и **пароль** (`password`); 
  - По паре: **Электронный адрес** (`email`) и **пароль** (`password`);
  - По паре: **Телефон** (`phone`) и **пароль** (`password`);

После успешной аутентификации система выдаст серию ключей:
  * **Ключ сессии** (`session`) для идентификации пользователя;
  * **Ключ API** (`key`) для аутентификации в системе (без участия пользователя);
  * **Секретный ключ сессии** (`secret`) для подписи `POST` запросов методом `HMAC-SHA256`.

**Ключ сессии** - это `hex` строка длиной 40 символов. Выдается сроком на 60 дней и не меняется со временем. Необходим для идентификации пользователя в системе.

**Ключ API** - это `hex` строка длиной 40 символов. Выдается сроком на 60 дней но теряет свою актуальность после применения. Используется для аутентификации в системе (без участия пользователя) и/или для получения нового `секретного ключ сессии`.  

**Секретный ключ сессии** - это `произвольная` строка длиной 64 символов. Не имеет срока. Необходим для подписи `POST` запросов методом `HMAC-SHA256`. 

**ВАЖНО**:
  - Клиентское приложение должно сохранить у себя эти `ключи` для прохождения процедуры аутентификации без участия пользователя.
  - Клиентское приложение должно самостоятельно позаботится о надёжном способе хранения ключей.
  
###### **Ключ сессии** (`AWS-Session`) и **Ключ API** (`API-Key`), в ответ на `POST` запрос `/sign/in`, дополнительно, будут добавлены в HTTP заголовок [Set-Cookie](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie) с параметром `HttpOnly`. 

#### Запрос к серверу приложений

 * Все запросы к серверу приложений выполнятся в рамках открытой `сессии`. 
 * Все `POST` запросы, кроме запросов на **Аутентификацию** (`/sign/in`) и **Регистрацию** (`/sign/up`) должны содержать данные авторизации.

В качестве данных авторизации могут выступать:
 * **Имя пользователя** и **пароль**, переданные в HTTP заголовке `Authorization` (схема авторизации `Basic`);
 * **Ключ сессии** и **Ключ API**, переданные в HTTP заголовке `Authorization` (схема авторизации `Token`);
 * **Набор данных** для подписи методом `HMAC-SHA256` (_описание следует ниже_).

#### HTTP заголовок `Authorization` 

Формат:
~~~
Authorization: <схема авторизации> <данные пользователя>
~~~

Формат токена:
~~~
<Ключ сессии>:<Ключ API>
~~~

**<схема авторизации>**
 
  * Basic
  * Token
   
**<данные пользователя>**

  * Если используется схема авторизации `Basic`, `данные пользователя` формируются следующим образом:
    Логин и пароль, разделенные двоеточием (username:password). 
    Результирующая строка, кодируется в base64 (dXNlcm5hbWU6cGFzc3dvcmQ=).
    
    Запрос по этой схеме выполняется каждый раз в новой сессии. Сессия открывается, выполняется запрос, сессия закрывается.
    
  * Если используется схема авторизации `Token`, `данные пользователя` формируются следующим образом:
    Ключ сессии и ключ API, разделенные двоеточием.
    
    Запрос по этой схеме выполняется в ранее открытой сессии. Для авторизации внутри сессии используется `Ключ API`. 
    
**Пример:**
```http request
POST /api/v1/whoami HTTP/1.1
Host: localhost:8080
Authorization: Basic dXNlcm5hbWU6cGFzc3dvcmQ=
````

```http request
POST /api/v1/whoami HTTP/1.1
Host: localhost:8080
Authorization: Token 4b641a6d7ec3c961ae421bb4b58eb83b56f99b1f:cb72b1c56d43d0cd6286ed19e743eed42df7816e
````

**ВАЖНО**: Ключ API теряет актуальность:
 - При каждом запросе по схеме авторизации `Token`;
 - При смене **Агента** (`User-Agent` HTTP заголовка);
 - По истечении 24 часов (при передаче ключа через `Cookie` ключ не теряет актуальность, но срок его хранения ограничен сутками).

Новый **Ключ API** возвращается в HTTP заголовках `Key` и [Set-Cookie](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Set-Cookie) (`API-Key`). Это правило действует только для запросов по схеме авторизации `Token`.

У **Ключа API** есть отложенное время "смерти". В течении пяти секунд после использования сервер приложений будет считать его действующим. Это даёт возможность, используя один ключ, отправлять несколько запросов подряд (группы запросов). Действующий **Ключ API** нужно будет получить из HTTP заголовка ответа на последний, отправленный на сервер, запрос.  

**ВНИМАНИЕ**: Если передать не актуальный **Ключ API**, то сессия будет скомпрометирована и закрыта. 

###### Это не самый удобный и не самый надежный способ отправки данных авторизации. Более удобный и рекомендуемый способ описан ниже.   

#### Подпись методом `HMAC-SHA256`

Вместо HTTP заголовка `Authorization` можно использовать подпись. 

Для передачи данных авторизации в виде подписи используются следующие HTTP заголовки:
  * `Session` - ключ сессии;
  * `Nonce` - данное время в миллисекундах;
  * `Signature` - подпись.

**Примеры:**

**Пример создания подписи на JavaScript (без данных в теле сообщения):**
~~~javascript
// CryptoJS - Standard JavaScript cryptography library

const body = null;

const Session = localStorage.getItem('Session'); // efa885ebde1baa991a3c798fc1141f6bec92fc90
const Secret = localStorage.getItem('Secret'); // y2WYJRE9f13g6qwFOEOe0rGM/ISlGFEEesUpQadHNd/aJL+ExKRj5E6OSQ9TuJRC

const Path = '/whoami';
const Nonce = (Date.now() * 1000).toString(); // 1589998352818000
const Body = JSON.stringify(body); // if body === null then Body = "null" <-- string  

const sigData = `${Path}${Nonce}${Body}`; // /whoami1589998352818000null

const Signature = CryptoJS.HmacSHA256(sigData, Secret).toString(); // 91609292e250fc30c48c2ad387d1121c703853fa88ce027e6ba0efe1fcb50ba1

let headers = new Headers();

headers.append('Session', Session);
headers.append('Nonce', Nonce);
headers.append('Signature', Signature);
headers.append('Content-Type', 'application/json');

const init = {
    method: 'POST',
    headers: headers,
    body: Body,
    mode: "cors"
};

const apiPath = `/api/v1${Path}`;

fetch(apiPath, init)
    .then((response) => {
        return response.json();
    })
    .then((json) => {
        console.log(json);
    })
    .catch((e) => {
        console.log(e.message);
});
~~~
* **openssl command:**
```shell script
echo -n "/whoami1589998352818000null" | \
openssl sha256 -hmac "y2WYJRE9f13g6qwFOEOe0rGM/ISlGFEEesUpQadHNd/aJL+ExKRj5E6OSQ9TuJRC"
(stdin)= 91609292e250fc30c48c2ad387d1121c703853fa88ce027e6ba0efe1fcb50ba1
```
* **curl command:**
```curl
curl -X POST \
     -H "Session: efa885ebde1baa991a3c798fc1141f6bec92fc90" \
     -H "Nonce: 1589998352818000" \
     -H "Signature: 91609292e250fc30c48c2ad387d1121c703853fa88ce027e6ba0efe1fcb50ba1" \
     http://localhost:8080/api/v1/whoami
````     
* **http request:**
```http request
POST /api/v1/whoami HTTP/1.1
Host: localhost:8080
Session: efa885ebde1baa991a3c798fc1141f6bec92fc90
Nonce: 1589998352818000
Signature: 91609292e250fc30c48c2ad387d1121c703853fa88ce027e6ba0efe1fcb50ba1
````
  
**Пример создания подписи на JavaScript (с данными в теле сообщения):**
~~~javascript
// CryptoJS - Standard JavaScript cryptography library

const body = {
  classcode : 'client',
  statecode : 'enabled',
  actioncode: 'invite'
};

const Session = localStorage.getItem('Session'); // efa885ebde1baa991a3c798fc1141f6bec92fc90
const Secret = localStorage.getItem('Secret'); // y2WYJRE9f13g6qwFOEOe0rGM/ISlGFEEesUpQadHNd/aJL+ExKRj5E6OSQ9TuJRC

const Path = '/method/get';
const Nonce = (Date.now() * 1000).toString(); // 1589998352902000
const Body = JSON.stringify(body); // <-- JSON string  

const sigData = `${Path}${Nonce}${Body}`; // /method/get1589998352902000{"classcode":"client","statecode":"enabled","actioncode":"invite"}

const Signature = CryptoJS.HmacSHA256(sigData, Secret).toString(); // 2b2bf5188ea40dfe8207efec56956b6170bdbc2f0ab0bffd8b50acd60979b09b

let headers = new Headers();

headers.append('Session', Session);
headers.append('Nonce', Nonce);
headers.append('Signature', Signature);
headers.append('Content-Type', 'application/json');

const init = {
    method: 'POST',
    headers: headers,
    body: Body,
    mode: "cors"
};

const apiPath = `/api/v1${Path}`;

fetch(apiPath, init)
    .then((response) => {
        return response.json();
    })
    .then((json) => {
        console.log(json);
    })
    .catch((e) => {
        console.log(e.message);
});
~~~
* **openssl command:**
```shell script
echo -n "/method/get1589998352902000{\"classcode\":\"client\",\"statecode\":\"enabled\",\"actioncode\":\"invite\"}" | \
openssl sha256 -hmac "y2WYJRE9f13g6qwFOEOe0rGM/ISlGFEEesUpQadHNd/aJL+ExKRj5E6OSQ9TuJRC"
(stdin)= 2b2bf5188ea40dfe8207efec56956b6170bdbc2f0ab0bffd8b50acd60979b09b
````
* **curl command:**
```curl
curl -X POST \
     -H "Session: efa885ebde1baa991a3c798fc1141f6bec92fc90" \
     -H "Nonce: 1589998352902000" \
     -H "Signature: 2b2bf5188ea40dfe8207efec56956b6170bdbc2f0ab0bffd8b50acd60979b09b" \
     -d "{\"classcode\":\"client\",\"statecode\":\"enabled\",\"actioncode\":\"invite\"}" \
     http://localhost:8080/api/v1/method/get
````
* **http request:**
```http request
POST /api/v1/method/get HTTP/1.1
Host: localhost:8080
Session: efa885ebde1baa991a3c798fc1141f6bec92fc90
Nonce: 1589998352902000
Signature: 2b2bf5188ea40dfe8207efec56956b6170bdbc2f0ab0bffd8b50acd60979b09b

{"classcode":"client","statecode":"enabled","actioncode":"invite"}
````

### Аутентификация

```http request
POST /api/v1/sign/in
```
Вход в систему по предоставленным данным от пользователя.

**Параметры:**

Имя | Тип | Описание
------------ | ------------ | ------------
username | STRING | Имя пользователя (`login`).
phone | STRING | Телефон.
email | STRING | Электронный адрес.
password | STRING | Пароль.
agent | STRING | Агент.
host | STRING | IP адрес.
  
Группы:
 - `<username>` `<password>` `[<agent>]` `[<host>]`
 - `<phone>` `<password>` `[<agent>]` `[<host>]`
 - `<email>` `<password>` `[<agent>]` `[<host>]`
  
**Описание ответа:**

Поле | Тип | Описание
------------ | ------------ | ------------
session | STRING | Ключ сессии
key | STRING | Одноразовый ключ
secret | STRING | Секретный ключ
result | BOOL | Результат
message | STRING | Сообщение об ошибке
  
**Пример:**

Запрос:
```http request
POST /api/v1/sign/in HTTP/1.1
Host: localhost:8080
Content-Type: application/json

{"username": "admin", "password": "admin"}
```

Ответ (положительный):
```json
{"session":"149d2ae6fa3f82eda21f7dba21824f199d508343","key":"8a364d7154fbb408a8033ba1cc74f0c38b8d6118","secret":"pkPm5zmHKj04Xr/NH1nKr6ZEUqOfyacC79HLFIQTrBgA6ApbgBvGiJlMBy4XBApt","result":true,"message":"Успешно."}
```

Ответ (отрицательный):
```json
{"error": {"code": 0, "message": "Вход в систему невозможен. Проверьте правильность имени пользователя и повторите ввод пароля."}}
```

### Регистрация
```http request
POST /api/v1/sign/up
```
Регистрация нового пользователя в системе.

**Параметры:**

Имя | Тип | Значение | Описание
------------ | ------------| ------------ |------------
type | STRING | entity, physical, individual | Тип пользователя.
username | STRING | | Имя пользователя (login)/ИНН.
password | STRING | Пароль.
name | JSON | | Полное наименование компании/Ф.И.О.
phone | STRING | | Телефон.
email | STRING | | Электронный адрес.
info | JSON  | | Дополнительная информация.
description | STRING | Описание.

**Формат `name`:**

Ключ | Тип | Описание
------------ | ------------ | ------------
name | STRING | Полное наименование организации/Ф.И.О одной строкой
short | STRING | Краткое наименование организации
first | STRING | Имя
last | STRING | Фамилия
middle | STRING | Отчество

Группы:
 - `<name>` `[<short>]`
 - `<first>` `<last>` `[<middle>]` `[<short>]`

**Формат `info`:** произвольный.

**Описание ответа:**

Поле | Тип | Описание
------------ | ------------ | ------------
id | NUMERIC | Идентификатор записи
result | BOOL | Результат
message | STRING | Сообщение об ошибке
  
**Пример:**

Запрос:
```http request
POST /api/v1/sign/up HTTP/1.1
Host: localhost:8080
Content-Type: application/json

{"type":"physical","username":"ivan","password":"Passw0rd","name":{"name":"Иванов Иван Иванович","short":"Иванов Иван","first":"Иван","last":"Иванов","middle":"Иванович"},"phone":"+79001234567","email":"ivan@mail.ru"}
```

Ответ (положительный):
```json
{"id":2,"result":true,"message":"Успешно."}
``` 
 
Ответ (отрицательный):
```json
{"id":null,"result":false,"message":"Учётная запись \"ivan\" уже существует."}
``` 
 
### Кто я?
```http request
POST /api/v1/whoami
```
Получить информацию об авторизованном пользователе.

**Параметры:**
 НЕТ
  
**Пример ответа:**
```json
{
  "id": 2,
  "userid": 1009,
  "username": "ivan",
  "fullname": "Иванов Иван Иванович",
  "phone": "+79001234567",
  "email": "ivan@mail.ru",
  "session_userid": 1009,
  "session_username": "ivan",
  "session_fullname": "Иванов Иван Иванович",
  "area": 100000000015,
  "area_code": "root",
  "area_name": "Корень",
  "interface": 100000000010,
  "interface_sid": "I:1:0:0",
  "interface_name": "Все"
}
```

### Текущие значения
```http request
POST /api/v1/current/<who>
```
Получить информацию о текущих значениях.

Где `<who>`:

- `session` (сессия);
- `user` (пользователь);
- `userid` (идентификатор пользователя);
- `username` (login пользователя);
- `area` (зона);
- `interface` (интерфейс);
- `language` (язык);
- `operdate` (дата операционного дня).
 
**Параметры:**
 НЕТ
  
## Язык
### Список языков
```http request
POST /api/v1/language
```
Получить список доступных языков.

**Параметры:**
 НЕТ
  
### Текущий язык
```http request
POST /api/v1/current/language
```
Получить идентификатор выбранного (текущего) языка.

**Параметры:**
 НЕТ
  
### Установить язык
```http request
POST /api/v1/language/set
```
Установить язык. 

**Параметры:**

Имя | Тип | Значение | Описание
------------ | ------------ | ------------ |------------
id | NUMERIC |  | Идентификатор
code | STRING | en, ru | Код

Группы:
 - `<id>`
 - `<code>`
  
## Объект
### Общие параметры

 * Для конечных точек возвращающих данные **ОБЪЕКТА** в виде списка `/list` применимо единное правило и следующее параметры:

Имя | Тип | Значение | Описание
------------ | ------------ | ------------ | ------------
fields | JSON | Массив строк | Список полей. Если не указан, вернёт все доступные поля. 
search | JSON | Массив объектов | Условия для отбора данных. 
filter | JSON | Объект | Фильтр для отбора данных в виде пары ключ/значение. В качестве ключа указывается имя поля. `filter` является краткой формой `search`. 
reclimit | INTEGER | Число | Возвращает не больше заданного числа строк (может быть меньше, если сам запрос выдал меньшее количество строк).
recoffset | INTEGER | Число | Пропускает указанное число строк в запросе.
orderby | JSON | Массив строк | Порядок сортировки (список полей). Допустимо указание `ASC` и `DESC` (`name DESC`) 

**Расшифровка параметра `search`**:

Ключ | Тип | Обязательный | Описание
------------ | ------------ | ------------ | ------------
condition | STRING | НЕТ | Условие. Может принимать только два значения `AND` (по умолчанию) или `OR`. 
field | STRING | ДА | Наименование поля, к которому применяется условие. 
compare | STRING | НЕТ | Сравнение. Может принимать только одно из значений: (`EQL`, `NEQ`, `LSS`, `LEQ`, `GTR`, `GEQ`, `GIN`, `LKE`, `ISN`, `INN`).
value | STRING | ДА/НЕТ | Искомое значение. 
valarr | JSON | ДА/НЕТ | Искомые значения в виде массива. Если указан ключ `valarr`, то ключи `compare` и `value` игнорируются.

**Формат `compare`:**

Значение | Описание
------------ | ------------
EQL | Равно (по умолчанию). 
NEQ | Не равно. 
LSS | Меньше. 
LEQ | Меньше или равно. 
GTR | Больше. 
GEQ | Больше или равно. 
GIN | Для поиска вхождений JSON. 
LKE | LIKE - Значение ключа `value` должно передаваться вместе со знаком '%' в нужном месте;
ISN | Ключ `value` должен быть опушен (не указан). 
INN | Ключ `value` должен быть опушен (не указан). 

**Примеры:**

```http request
POST /api/v1/object/geolocation/list HTTP/1.1
Host: localhost:8080
Content-Type: application/json
Authorization: Basic YWRtaW46YWRtaW4=

{"filter": {"object": 2, "code": "default"}}
````

```http request
POST /api/v1/address/list HTTP/1.1
Host: localhost:8080
Content-Type: application/json
Authorization: Basic YWRtaW46YWRtaW4=

{"fields": ["description"], "search": [{"field": "apartment", "compare": "GEQ", "value": "5"}]}
````

## Геолокация
### Координаты геолокации
```http request
POST /api/v1/object/geolocation[/get | /set]
```
Получить или установить координаты геолокации для объекта. 

Где:
  * `/get` - `Получить`  
  * `/set` - `Установить`  

При отсутствии `/get` или `/set` работает следующее правило:

 * Если значение `coordinates` не указано или равно `null`, то метот работает как `Получить` иначе как `Установить`.

### Установить
```http request
POST /api/v1/object/geolocation/set
```
Установить координаты геолокации для объекта. 

**Параметры:**

Имя | Тип | Значение | Описание
------------ | ------------ | ------------ |------------
id | NUMERIC |  | Идентификатор объекта
coordinates | JSON | null OR json | Координаты
  
**Формат `coordinates`:**

Ключ | Тип | Описание
------------ | ------------ | ------------
id | NUMERIC | Идентификатор
code | STRING | Код
name | STRING | Наименование
latitude | NUMERIC | Широта
longitude | NUMERIC | Долгота
accuracy | NUMERIC | Точность (высота над уровнем моря)
description | STRING | Описание

Если ключ `id` не указан то действие считается как `Добавить`. Если ключ `id` указан и значение ключа `name` не `null` то действие считается как `Обновить` иначе как `Удалить`. Значения не указанных ключей считаются как `null`.   

### Получить
```http request
POST /api/v1/object/geolocation/get
```
Получить координаты геолокации для объекта. 

**Параметры:**

Имя | Тип | Значение | Описание
------------ | ------------ | ------------ |------------
id | NUMERIC |  | Идентификатор объекта
fields | JSON | [field, field, ...] | Поля

Где `fields` - это массив JSON STRING наименований полей в таблице, если не указано то запрос вернет все поля.  

### Список
```http request
POST /api/v1/object/geolocation/list
```
Получить координаты геолокации в виде списка. 

**Параметры:**
[Общие параметры для объекта](#общие-параметры)

## Метод
```http request
POST /api/v1/method[/list | /get]
```
Запросить информацию о методах документооборота.

`/list` предоставит список всех доступных методов. 

Для `/get` доступен как минимум один из **параметров** указанный в теле запроса:

Имя | Тип | Описание
------------ | ------------ | ------------
object | NUMERIC | Идентификатор объекта
class | NUMERIC | Идентификатор класса
state | NUMERIC | Идентификатор состояния
classcode | STRING | Код класса (вместо идентификатора)
statecode | STRING | Код состояния (вместо идентификатора)
  
**Пример:**

Запрос:
```http request
POST /api/v1/method/get HTTP/1.1
Host: localhost:8080
Content-Type: application/x-www-form-urlencoded
Authorization: Basic YWRtaW46YWRtaW4=

classcode=client&statecode=enabled
```

Ответ:
```json
[
  {"id":100000000211,"parent":null,"action":100000000053,"actioncode":"disable","label":"Закрыть","visible":true},
  {"id":100000000212,"parent":null,"action":100000000054,"actioncode":"delete","label":"Удалить","visible":true}
]
```

# Отправка и обработка HTTP‑запросов в PostgreSQL без расширений и внешних языков

Во многих проектах PostgreSQL используется как «пассивное» хранилище: приложения ходят в базу за данными, а вся интеграция с внешними API реализуется снаружи.

В этой статье я покажу другой подход: как отправлять и обрабатывать HTTP‑запросы непосредственно из PostgreSQL, не устанавливая дополнительных расширений и не используя внешние языки программирования — только [PL/pgSQL](https://postgrespro.ru/docs/postgresql/16/plpgsql).

При этом:

- HTTP‑запросы выполняются **асинхронно**;
- входящие и исходящие запросы логируются в таблицах;
- обработка ответов делается через **функции обратного вызова** (callback) в самой базе.

---

## О среде выполнения

Для демонстрации нам понадобится предварительно настроенная база данных. Доступ к ней можно получить двумя способами:

1. через веб‑интерфейс на нашем сайте: <https://apostoldevel.com/pgweb>
2. локально — запустив окружение через Docker Compose (описано ниже).

По ссылке открывается [pgweb](https://github.com/sosedoff/pgweb) — веб‑обозреватель для PostgreSQL. В нём удобно:

- выполнять SQL‑запросы из статьи;
- просматривать таблицы и представления;
- изучать код функций на [PL/pgSQL](https://postgrespro.ru/docs/postgresql/16/plpgsql).

![pgweb](pgweb.png)

---

## Локальный запуск через Docker Compose

### Сборка контейнеров

```bash
./docker-build.sh
```

### Запуск контейнеров

```bash
./docker-up.sh
```

После запуска:

- **Swagger UI** ([swagger-ui](https://github.com/swagger-api/swagger-ui))  
  доступен по адресу:
  - <http://localhost:8080>
  - или `http://<host-ip>:8080`

- **Pgweb** ([pgweb](https://github.com/sosedoff/pgweb)) — веб‑обозреватель для PostgreSQL  
  доступен по адресу:
  - <http://localhost:8081/pgweb>
  - или `http://<host-ip>:8081/pgweb`

Этого достаточно, чтобы выполнять HTTP‑запросы непосредственно из PostgreSQL.

> Вместо pgweb можно использовать любой другой клиент для работы с PostgreSQL.  
> PostgreSQL из контейнера доступен на порту `5433`.

---

## HTTP‑клиент в PostgreSQL

Начнём с самого простого: выполним HTTP‑запрос к локальному серверу.

```sql
SELECT http.fetch('http://localhost:8080/api/v1/time', content => null::text);
```

Запрос выполняется **асинхронно**, поэтому вместо ответа вы получите `uuid` — идентификатор запроса.

Под капотом:

- исходящие HTTP‑запросы сохраняются в таблицу `http.request`,
- результаты запросов — в таблицу `http.response`.

Посмотреть историю запросов и ответов можно через представление `http.fetch`:

```sql
SELECT * FROM http.fetch ORDER BY datestart DESC;
```

![fetch](fetch.png)

Здесь:

- `status` — код HTTP‑ответа ([HTTP response status codes](https://developer.mozilla.org/en-US/docs/Web/HTTP/Status));
- `response` — тело HTTP‑ответа.

Теперь запросим данные с публичного сервиса [JSONPlaceholder](https://jsonplaceholder.typicode.com):

```sql
SELECT http.fetch(
  'https://jsonplaceholder.typicode.com/posts/1',
  'GET',
  content => null::text,
  type    => 'curl'
);
```

В поле `response` появится, например:

```json
{
  "userId": 1,
  "id": 1,
  "title": "delectus aut autem",
  "completed": false
}
```

> Параметр `type = 'curl'` используется для запросов к серверам, которые поддерживают только **HTTP/2**. В этом случае выполнение идёт через библиотеку [cURL](https://curl.se).

---

## HTTP‑сервер на стороне PostgreSQL

Кроме исходящих запросов, мы фиксируем и входящие HTTP‑запросы. Они логируются в таблицу `http.log`:

```sql
SELECT * FROM http.log ORDER BY id DESC;
```

![Log](log.png)

Для обработки входящих запросов используются две функции на PL/pgSQL:

- `http.get`
- `http.post`

Параметры:

- `path` — путь запроса;
- `headers` — HTTP‑заголовки;
- `params` — строка запроса (query string), преобразованная в JSON;
- `body` — тело запроса (для POST и похожих методов).

Функции возвращают результат в виде множества строк (`SETOF json`).

> Возврат множества (`SETOF`) позволяет эффективно обрабатывать результаты построчно. Подробнее — в [документации по PL/pgSQL](https://postgrespro.ru/docs/postgresql/16/plpgsql-control-structures).

Пример фрагмента из `http.get`, который возвращает содержимое лога:

```sql
WHEN 'log' THEN
 
  FOR r IN SELECT * FROM http.log ORDER BY id DESC
  LOOP
    RETURN NEXT row_to_json(r);
  END LOOP;
```

Вызвать этот код можно через:

```sql
SELECT http.fetch('http://localhost:8080/api/v1/log', content => null::text);
```

---

## Callback‑функции: обработка ответов в базе

Теперь — самое интересное. Обработку ответов мы делаем через callback‑функции в самой базе.

Разберём это на конкретной задаче: получение курсов валют, разбор ответа и сохранение в таблицу курсов.

Чтобы показать, как это увязано с HTTP‑сервером, добавим в `http.get` обработку пути `latest`:

```sql
WHEN 'latest' THEN

  FOR r IN
    SELECT * FROM jsonb_to_record(params) AS x(base text, symbols text)
  LOOP
    IF r.base = 'USD' THEN
      RETURN NEXT jsonb_build_object(
        'success',   true,
        'timestamp', trunc(extract(EPOCH FROM now())),
        'base',      r.base,
        'date',      to_char(now(), 'YYYY-MM-DD'),
        'rates',     jsonb_build_object(
                        'RUB', 96.245026,
                        'EUR', 0.946739,
                        'BTC', 0.000038
                      )
      );
    ELSIF r.base = 'BTC' THEN
      RETURN NEXT jsonb_build_object(
        'success',   true,
        'timestamp', trunc(extract(EPOCH FROM now())),
        'base',      r.base,
        'date',      to_char(now(), 'YYYY-MM-DD'),
        'rates',     jsonb_build_object(
                        'RUB', 2542803.2,
                        'EUR', 25012.95,
                        'USD', 26420.1
                      )
      );
    ELSE
      RETURN NEXT jsonb_build_object(
        'success', false,
        jsonb_build_object(
          'code',    400,
          'message', format('Base "%s" not supported.', r.base)
        )
      );
    END IF;
  END LOOP;
```

> Здесь на запрос «последних» (latest) курсов мы возвращаем **статический** ответ в формате [API курсов валют](https://exchangeratesapi.io/documentation). Если у вас есть доступ к реальному сервису курсов валют, вы можете запрашивать данные через их API.

Дальше подключаем callback‑функции:

- `public.exchange_rate_done` — обработка успешного ответа,
- `public.exchange_rate_fail` — обработка ошибок.

Код этих функций можно посмотреть в pgweb, поэтому здесь покажу только примеры вызовов.

### Вызовы из контейнера

```sql
SELECT http.fetch(
  'http://localhost:8080/api/v1/latest?base=USD',
  'GET',
  null,
  null,
  'public.exchange_rate_done',
  'public.exchange_rate_fail',
  'api.exchangerate.host',
  null,
  'latest'
);

SELECT http.fetch(
  'http://localhost:8080/api/v1/latest?base=BTC',
  'GET',
  null,
  null,
  'public.exchange_rate_done',
  'public.exchange_rate_fail',
  'api.exchangerate.host',
  null,
  'latest'
);
```

### Через наш сервер

```sql
SELECT http.fetch(
  'https://apostoldevel.com/api/v1/latest?base=USD',
  'GET',
  null,
  null,
  'public.exchange_rate_done',
  'public.exchange_rate_fail',
  'api.exchangerate.host',
  null,
  'latest'
);

SELECT http.fetch(
  'https://apostoldevel.com/api/v1/latest?base=BTC',
  'GET',
  null,
  null,
  'public.exchange_rate_done',
  'public.exchange_rate_fail',
  'api.exchangerate.host',
  null,
  'latest'
);
```

### Через внешний сервис курсов валют (если есть доступ)

```sql
SELECT http.fetch(
  'https://api.exchangerate.host/latest?base=USD&symbols=BTC,EUR,RUB',
  'GET',
  null,
  null,
  'public.exchange_rate_done',
  'public.exchange_rate_fail',
  'api.exchangerate.host',
  null,
  'latest',
  null,
  'curl'
);

SELECT http.fetch(
  'https://api.exchangerate.host/latest?base=BTC&symbols=USD,EUR,RUB',
  'GET',
  null,
  null,
  'public.exchange_rate_done',
  'public.exchange_rate_fail',
  'api.exchangerate.host',
  null,
  'latest',
  null,
  'curl'
);
```

Результат работы callback‑функций — заполненная таблица `public.rate`.

Посмотреть актуальные курсы можно через представление `public.rates`:

```sql
SELECT *
FROM public.rates
WHERE validFromDate <= now()
  AND validToDate   > now();
```

![Rates](rates.png)

---

## Как это работает: LISTEN/NOTIFY

Механизм асинхронного выполнения завязан на стандартные возможности PostgreSQL — `LISTEN/NOTIFY`.

Из [документации](https://postgrespro.ru/docs/postgresql/16/libpq-notify):

> **34.9. Асинхронное уведомление**  
> PostgreSQL предлагает асинхронное уведомление посредством команд `LISTEN` и `NOTIFY`. Клиентский сеанс регистрирует свою заинтересованность в конкретном канале уведомлений с помощью команды `LISTEN` (и может остановить прослушивание с помощью `UNLISTEN`). Все сеансы, прослушивающие конкретный канал, будут уведомляться в асинхронном режиме, когда в рамках любого сеанса команда `NOTIFY` выполняется с параметром, указывающим имя этого канала. Для передачи дополнительных данных прослушивающим сеансам может использоваться строка `payload`.

На этом механизме строится взаимодействие PostgreSQL с клиентским приложением, которое фактически и выполняет HTTP‑запросы.

---

## Жизненный цикл `http.fetch`

Разберёмся, что происходит после вызова:

```sql
SELECT http.fetch('http://localhost:8080/api/v1/time', content => null::text);
```

1. Функция PL/pgSQL `http.fetch` — это удобная оболочка над `http.create_request`.
2. `http.create_request` создаёт запись в таблице `http.request` с параметрами HTTP‑запроса.
3. На вставку в `http.request` срабатывает триггер.
4. Триггер выполняет `NOTIFY` на определённый канал, передавая идентификатор запроса в `payload`.

На этом участие PostgreSQL в данном шаге заканчивается.

![NOTIFY](pg_notify.png)

Далее:

- внешнее клиентское приложение, заранее подписанное на канал (`LISTEN`), получает уведомление;
- читает данные запроса из `http.request`;
- выполняет HTTP‑запрос (через HTTP‑клиент или cURL, в том числе с поддержкой HTTP/2);
- сохраняет результат в таблицу `http.response`;
- при необходимости вызывает указанные callback‑функции в базе.

---

## Клиентское приложение

Клиентское приложение — это отдельная программа, которая:

- поддерживает соединение с PostgreSQL;
- подписана на нужные каналы уведомлений (`LISTEN`);
- по событию `NOTIFY` достаёт из `http.request` новые задачи;
- выполняет HTTP‑запросы к внешним системам;
- сохраняет результаты в `http.response`;
- инициирует вызов callback‑функций.

Такое приложение можно реализовать на любом удобном языке.

Мы используем свою **open source** разработку — [Апостол](https://github.com/apostoldevel/apostol).

![Activity](activity.png)

---

## Где это применимо

Практический пример. В вашей системе формируется счёт на оплату, который нужно автоматически списать с заранее привязанной карты клиента.

Общий сценарий:

1. В PostgreSQL появляется новый счёт.
2. Триггер формирует запись в `http.request` и отправляет `NOTIFY`.
3. Клиентское приложение:
  - получает уведомление,
  - читает данные счёта,
  - формирует запрос к платёжной системе,
  - сохраняет результат в БД.
4. После успешного списания:
  - формируется электронный чек (вызов другого сервиса),
  - отправляются уведомления клиенту (e‑mail, SMS, push через [FCM](https://firebase.google.com/docs/cloud-messaging?hl=ru)).

Возникает цепочка интеграций с внешними системами, при этом все бизнес‑данные и логика оркестрации находятся в базе.

Если данные уже в СУБД, а у нас есть механизм общения с внешними API напрямую из PostgreSQL, логично формировать запросы к этим API там же, где и данные. Это не теоретическая конструкция, а практическая и рабочая схема.

Более наглядный пример: Telegram‑бот [Talking to AI](https://t.me/TalkingToAIBot) для общения с ChatGPT, который реализован на PL/pgSQL и использует описанный подход.

---

## Исходники

- Клиентское приложение: [Апостол](https://github.com/apostoldevel/apostol)
- Модуль для работы из PostgreSQL: [Postgres Fetch](https://github.com/apostoldevel/module-PGFetch)

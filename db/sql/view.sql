CREATE OR REPLACE VIEW http.fetch
AS
  SELECT t1.id, t1.datetime AS start, t1.state, t1.method, t1.resource, t1.headers AS request_headers,
         t1.content AS request, t2.status, t2.status_text, t2.headers AS response_headers, t2.content AS response,
         t1.error, t2.datetime AS stop, t2.runtime
    FROM http.request t1 LEFT JOIN http.response t2 ON t1.id = t2.request;

GRANT ALL ON http.fetch TO http;

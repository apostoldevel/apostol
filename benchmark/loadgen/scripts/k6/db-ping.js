import http from 'k6/http';
import { check } from 'k6';

const TARGET = __ENV.TARGET || 'http://bench-nginx';
const VERSION = __ENV.VERSION || 'v0';

export const options = {
  vus: parseInt(__ENV.VUS || '100'),
  duration: __ENV.DURATION || '60s',
  thresholds: {
    http_req_failed: ['rate<0.01'],
    http_req_duration: ['p(95)<500'],
  },
};

export default function () {
  const res = http.get(`${TARGET}/api/${VERSION}/db/ping`);
  check(res, {
    'status is 200': (r) => r.status === 200,
    'body has ok': (r) => r.json('ok') === true,
  });
}

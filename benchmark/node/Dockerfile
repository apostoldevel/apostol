FROM node:22-alpine

RUN npm install -g pm2

WORKDIR /app

COPY package.json .
RUN npm install --production

COPY app.js ecosystem.config.js ./

ENV WORKERS=4

CMD ["pm2-runtime", "ecosystem.config.js"]

EXPOSE 3000

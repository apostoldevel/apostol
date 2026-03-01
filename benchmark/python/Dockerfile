FROM python:3.12-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY app.py .

ENV WORKERS=4

CMD uvicorn app:app --host 0.0.0.0 --port 8000 --workers $WORKERS --no-access-log --backlog 4096

EXPOSE 8000

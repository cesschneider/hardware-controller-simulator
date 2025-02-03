FROM python:3.9-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY controller.py .

# Flask port
EXPOSE 7001

# Run python in unbuffered mode so logs appear immediately
CMD ["python", "-u", "controller.py"]

FROM python:3.9-slim-buster

COPY requirements.txt ./
RUN pip3 install --no-cache-dir -r requirements.txt

COPY *.py /usr/src/app/

ENTRYPOINT ["python3"]
CMD ["/usr/src/app/api_controller.py"]
# Habit Tracker

# Arduino

## Setup client certificate authentication
Add Certificate information to the `arduino/src/Provisioning/arduino_secrets.h` file.

Compile and upload the sketch to the arduino. Connect to the serial and copy the certificate signing request to `arduino/src/Provisioning/arduino.csr`.
```bash
cd arduino/src/Provisioning
make upload
screen /dev/tty.usbmodem* 9600
```

Now sign the CSR with the certificate authority (CA) and put the resulting CRT into `arduino/src/JustDoIt/arduino_secrets.h`.
For my personal ansible playbooks, run the following:
```bash
ansible-playbook client_certificate.yaml \
  --extra-var="cert_dir=../justdoit/arduino/src/Provisioning/ cert_name=arduino"
```

## Data Format
For dates, use ISO8601: `2020-11-09T14:23:45+0000`

Retrieving the date history from the server will yield something like this:
```json
{"history": [
    {"index": 0, "date": "25.11.2020", "done": 0},
    {"index": 2, "date": "24.11.2020", "done": 1},
    [...]
    {"index": 9, "date": "16.11.2020", "done": 1},
]}
```

## Run Backend

```bash
cd backend
docker kill habit-tracker
docker rm habit-tracker
docker build -t habit-tracker:latest -f Dockerfile .
docker run -d \
    -p 5555:5000 \
    -v $PWD:/data \
    --name habit-tracker \
    habit-tracker:latest
```

## Test Backend

With certificate auth:
```
curl --cert /tmp/user.crt --key /tmp/user.key
```

```bash
curl -X GET -H "Content-Type: application/json" \
    -d '{"startDate": "2020-11-15T10:14:43+01:00", "count": 60 }' \
    http://localhost:5555/habit/meditation
```

```bash
curl -X POST -H "Content-Type: application/json" \
    -d '{"dates": [{"date": "25.12.2020T14:23:45+00:00"}] }' \
    http://localhost:5555/habit/meditation
```

```bash
curl -X GET -H "Content-Type: application/json" \
    -d '{"startDate": "2020-11-15T10:14:43+01:00"}' \
    http://localhost:5555/habit/meditation/streak
```
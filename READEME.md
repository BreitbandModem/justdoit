# Habit Tracker

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
docker build -t habit-tracker:latest -f Dockerfile .
docker run -d \
    -p 80:5000 \
    --name habit-tracker \
    habit-tracker:latest
```

## Test Backend

```bash
curl -X POST -H "Content-Type: application/json" \
    -d '{"date": "22-11-2020"}' \
    http://localhost/habit/meditation
```
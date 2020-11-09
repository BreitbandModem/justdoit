# Habit Tracker

## Data Format
For dates, use ISO8601: `2018-08-25T14:23:45+0000`

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
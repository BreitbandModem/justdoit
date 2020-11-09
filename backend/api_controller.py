#!/usr/bin/env python

from habit_model import HabitModel

import logging
import json
from jsonschema import validate, ValidationError, SchemaError
from flask import Flask, Response, request, jsonify

app = Flask(__name__)
meditation_habit = None

@app.route('/habit/meditation', methods=['GET'])
def get_dates():
    """Get interpolated list of dates from last x days."""
    schema = {
        "type": "object",
        "properties": {
            "history": {"type": "number"}
        },
        "required": ["history"]

    }

    history_json = json.loads(request.data)
    if history_json is not None:
        try:
            validate(history_json, schema)
        except SchemaError as e:
            app.logger.error('Schema definition invalid.')
        except ValidationError as e:
            app.logger.warning(e)
        else:
            app.logger.info('Retreiving history for last %s days.', history_json['history'])

    return jsonify({'date': "blabla"})


@app.route('/habit/meditation', methods=['POST'])
def add_dates():
    """Submit a list of dates when the habit was done."""
    schema = {
        "type": "object",
        "definitions": {
            "DateEntry": {
                "properties": {"date": {"type": "string"}},
                "required": ["date"],
                "additionalProperties": False
            }
        },
        "properties": {
            "dates": {
                "type": "array",
                "items": {"$ref": "#/definitions/DateEntry"}
            }
        },
        "additionalProperties": False,
        "required": ["dates"]
    }

    dates = json.loads(request.data)
    if dates is not None:
        try:
            validate(dates, schema)
        except SchemaError as e:
            app.logger.error('Schema definition invalid.')
        except ValidationError as e:
            app.logger.warning(e)
        else:
            app.logger.info('Adding list of dates: %s.', dates['dates'][0])
            meditation_habit.add_dates(dates['dates'])

    return jsonify({'date': "9. Nov 2020"})


@app.route('/habit/meditation', methods=['DELETE'])
def delete_dates():
    """Delete a list of dates when a habit was not done."""
    date = json.loads(request.data)
    if date is not None:
        app.logger.info('Date to be deleted: %s', date)

    return jsonify({'result': 0})


if __name__ == '__main__':
    # Init Logger
    logging.basicConfig(level=logging.DEBUG)
    logging.basicConfig(filename='/logs/app.log', level=logging.DEBUG)
    app.logger.info('Hello Logger')

    meditation_habit = HabitModel(app.logger, '/data/meditation.csv')

    app.run(host='0.0.0.0', threaded=True, debug=False)

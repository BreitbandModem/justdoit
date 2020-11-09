#!/usr/bin/env python

from habit_model import HabitModel

import logging
import json
from jsonschema import validate, ValidationError, SchemaError
from flask import Flask, request, jsonify

app = Flask(__name__)
meditation_habit = None

date_list_schema = {
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


@app.route('/habit/meditation', methods=['GET'])
def get_dates():
    """Get interpolated list of dates from last x days."""
    schema = {
        "type": "object",
        "properties": {
            "startDate": {"type": "string"},
            "count": {"type": "number"}
        },
        "required": ["startDate", "count"]

    }

    history_json = json.loads(request.data)
    if history_json is not None:
        try:
            validate(history_json, schema)
        except SchemaError as e:
            app.logger.error('Schema definition invalid.')
            return jsonify({'history': []}), 500
        except ValidationError as e:
            app.logger.warning(e)
            return jsonify({'history': []}), 400
        else:
            app.logger.info('Retreiving history for last %s days.', history_json['count'])
            history = meditation_habit.get_history(history_json["startDate"], history_json["count"])
            return jsonify(history), 200

    return jsonify({'history': []}), 500


@app.route('/habit/meditation', methods=['POST'])
def add_dates():
    """Submit a list of dates when the habit was done."""
    dates = json.loads(request.data)
    if dates is not None:
        try:
            validate(dates, date_list_schema)
        except SchemaError as e:
            app.logger.error('Schema definition invalid.')
            return jsonify({'added': 0}), 500
        except ValidationError as e:
            app.logger.warning(e)
            jsonify({'added': 0}), 400
        else:
            app.logger.info('Adding list of dates: %s.', dates['dates'][0])
            try:
                added = meditation_habit.add_dates(dates['dates'])
            except Exception:
                return jsonify({'added': 0}), 500
            else:
                if added > 0:
                    return jsonify({'added': added}), 201
                else:
                    return jsonify({'added': added}), 200

    return jsonify({'added': 0}), 500



@app.route('/habit/meditation', methods=['DELETE'])
def delete_dates():
    """Delete a list of dates when a habit was not done."""
    dates = json.loads(request.data)
    if dates is not None:
        try:
            validate(dates, date_list_schema)
        except SchemaError as e:
            app.logger.error('Schema definition invalid.')
            return jsonify({'deleted': 0}), 500
        except ValidationError as e:
            app.logger.warning(e)
            return jsonify({'deleted': 0}), 400
        else:
            app.logger.info('Deleting list of dates: %s.', dates['dates'][0])
            try:
                deleted = meditation_habit.delete_dates(dates['dates'])
            except Exception:
                return jsonify({'deleted': 0}), 500
            else:
                return jsonify({'deleted': deleted}), 200

    return jsonify({'deleted': 0}), 500


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    app.logger.info('Starting Webserver')

    meditation_habit = HabitModel(app.logger, '/data/meditation.csv')

    app.run(host='0.0.0.0', threaded=True, debug=False)

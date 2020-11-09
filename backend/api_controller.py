#!/usr/bin/env python

from habit_model import HabitModel

import json
from jsonschema import validate, ValidationError, SchemaError
from flask import Flask, Response, request, jsonify

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
            return Response(jsonify({'history': []}), status=500, mimetype='application/json')
        except ValidationError as e:
            app.logger.warning(e)
            return Response(jsonify({'history': []}), status=400, mimetype='application/json')
        else:
            app.logger.info('Retreiving history for last %s days.', history_json['history'])

    return Response(jsonify({'history': []}), status=500, mimetype='application/json')


@app.route('/habit/meditation', methods=['POST'])
def add_dates():
    """Submit a list of dates when the habit was done."""
    dates = json.loads(request.data)
    if dates is not None:
        try:
            validate(dates, date_list_schema)
        except SchemaError as e:
            app.logger.error('Schema definition invalid.')
            return Response(jsonify({'added': 0}), status=500, mimetype='application/json')
        except ValidationError as e:
            app.logger.warning(e)
            return Response(jsonify({'added': 0}), status=400, mimetype='application/json')
        else:
            app.logger.info('Adding list of dates: %s.', dates['dates'][0])
            try:
                added = meditation_habit.add_dates(dates['dates'])
            except Exception:
                return Response(jsonify({'added': 0}), status=500, mimetype='application/json')
            else:
                if added > 0:
                    status = 201
                else:
                    status = 200
                return Response(jsonify({'added': added}), status=status, mimetype='application/json')

    return Response(jsonify({'added': 0}), status=500, mimetype='application/json')



@app.route('/habit/meditation', methods=['DELETE'])
def delete_dates():
    """Delete a list of dates when a habit was not done."""
    dates = json.loads(request.data)
    if dates is not None:
        try:
            validate(dates, date_list_schema)
        except SchemaError as e:
            app.logger.error('Schema definition invalid.')
            return Response(jsonify({'deleted': 0}), status=500, mimetype='application/json')
        except ValidationError as e:
            app.logger.warning(e)
            return Response(jsonify({'deleted': 0}), status=400, mimetype='application/json')
        else:
            app.logger.info('Deleting list of dates: %s.', dates['dates'][0])
            try:
                deleted = meditation_habit.delete_dates(dates['dates'])
            except Exception:
                return Response(jsonify({'deleted': 0}), status=500, mimetype='application/json')
            else:
                return Response(jsonify({'deleted': deleted}), status=200, mimetype='application/json')

    return Response(jsonify({'deleted': 0}), status=500, mimetype='application/json')


if __name__ == '__main__':
    app.logger.info('Starting Webserver')

    meditation_habit = HabitModel(app.logger, '/data/meditation.csv')

    app.run(host='0.0.0.0', threaded=True, debug=False)

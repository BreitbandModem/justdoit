from datetime import timedelta, datetime


def daterange(start_date, count):
    for days in range(count):
        yield start_date - timedelta(days)


class HabitModel:
    """A model class to store and retrieve habit data from csv."""

    def __init__(self, logger, dates_filename):
        """Init habit model."""
        self.logger = logger
        self.dates_filename = dates_filename

        # Create file if not exists
        try:
            with open(self.dates_filename, 'x') as new_dates_file:
                self.logger.info('Created new dates file %s', self.dates_filename)
        except FileExistsError:
            self.logger.info('Dates file exists: %s', self.dates_filename)

    def get_history(self, start_date, count):
        """Get single date by counting back from start_date"""
        history = {"history": []}

        start_date = datetime.fromisoformat(start_date)
        count_date = start_date - timedelta(count)

        done = 0
        with open(self.dates_filename, 'r') as dates_file:
            if count_date.strftime("%Y-%m-%d") in dates_file.read():
                done = 1
    
        date = {"date": count_date.isoformat(), "done": done}
        history["history"].append(date)
        
        return history

    def get_history_padded(self, start_date, count):
        """Get interpolated list of dates from last x days."""
        history = {"history": []}

        start_date = datetime.fromisoformat(start_date)

        with open(self.dates_filename, 'r') as dates_file:
            dates_file_read = dates_file.read()

        index = 0
        for date in daterange(start_date, count):
            # self.logger.info('Date: %s', date.strftime("%Y-%m-%d"))
            # self.logger.info("Date iso! %s", date.isoformat())

            done = 0

            if date.strftime("%Y-%m-%d") in dates_file_read:
                done = 1

            item = {"date": date.isoformat(), "done": done}
            history["history"].append(item)

            index += 1

        return history

    def get_streak(self, start_date):
        """Get number of consecutive dates found in the data store starting at start_date"""
        streak = 0

        start_date = datetime.fromisoformat(start_date)

        with open(self.dates_filename, 'r') as dates_file:
            dates_file_read = dates_file.read()

        # for last ten years...
        for date in daterange(start_date, 36500):
            if not date.strftime("%Y-%m-%d") in dates_file_read:
                return {"streak": streak}
            else:
                streak += 1
        
        return {"streak": streak}

    def add_dates(self, dates):
        """Store list of dates to csv file."""
        add_count = 0
        with open(self.dates_filename, 'a+') as dates_file:
            for date in dates:
                # validate time format: throws exception to api_controller
                datetime.fromisoformat(date["date"])
                self.logger.info("Adding date: %s", date)

                # if date (without time) doesn't exist in file, insert full date at end of file
                dates_file.seek(0)  # seek to file start
                if date["date"].split('T')[0] not in dates_file.read():
                    dates_file.seek(0, 2)  # seek to file end
                    dates_file.write(date["date"] + "\n")
                    add_count += 1
        return add_count

    def delete_dates(self, dates):
        """Delete list of dates from csv file."""
        delete_count = 0
        with open(self.dates_filename, 'r+') as dates_file:
            # cut file contents to memory
            dates_file.seek(0)
            dates_file_lines = dates_file.readlines()

            dates_file.seek(0)
            dates_file.truncate()

            # write all lines except for to be deleted back to file
            for line in dates_file_lines:
                date_found = False

                for date in dates:
                    self.logger.info('searching for date %s', date["date"])
                    if date["date"].split('T')[0] in line:
                        self.logger.info('found date in line: \n%s', line)
                        date_found = True
                        delete_count += 1

                if not date_found:
                    self.logger.info('writing line: \n%s', line)
                    dates_file.write(line)

        return delete_count

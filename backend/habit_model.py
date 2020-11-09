class HabitModel:
    """A model class to store and retrieve habit data from csv."""

    def __init__(self, logger, dates_filename):
        """Init habit model."""
        self.logger = logger
        self.dates_filename = dates_filename

        try:
            with open(self.dates_filename, 'x') as new_dates_file:
                self.logger.info('Created new dates file %s', self.dates_filename)
        except FileExistsError:
            self.logger.info('Dates file exists: %s', self.dates_filename)

    def get_history(self, history):
        """Get interpolated list of dates from last x days."""
        pass

    def add_dates(self, dates):
        """Store list of dates to csv file."""
        with open(self.dates_filename, 'a+') as dates_file:
            for date in dates:
                # if date (without time) doesn't exist in file, insert full date at end of file
                dates_file.seek(0)  # seek to file start
                if date["date"].split('T')[0] not in dates_file.read():
                    dates_file.seek(0, 2)  # seek to file end
                    dates_file.write(date["date"] + "\n")

    def delete_dates(self, dates):
        pass


import os


def ShowAllSensorData(sensor_data, sensor_names):
    """
        Prints all sensor data to the console.
    Args:
        sensor_data (dict): A dictionary where keys are sensor names and values are lists of sensor data vectors.
        sensor_names (list): A list of sensor data names corresponding to the values in the sensor data vectors.
    """

    for key, value in sensor_data.items():
        print(f"{key}:")
        print(sensor_names)
        ShowSensorData(value)

def ShowSensorData(sensor_data_vector):
    for value in sensor_data_vector:
        print(f"  {value}")

def WriteAllSensorData(dir_name, sensor_data, sensor_names):
    """
        Writes all sensor data to CSV files in the specified directory. Each sensor will have its own CSV file.
    Args:
        dir_name (str): The name of the directory where the CSV files will be saved.
        sensor_data (dict): A dictionary where keys are sensor names and values are lists of sensor data vectors.
        sensor_names (list): A list of sensor data names corresponding to the values in the sensor data vectors.
    """

    for key, value in sensor_data.items():
        filename = os.path.join(dir_name, f"sensor_{key}.csv")
        with open(filename, "w") as f:
            f.write(",".join(sensor_names) + "\n")
            WriteSensorData(f, value)


def WriteSensorData(file, sensor_data_vector):
    for value in sensor_data_vector:
        for v in value[0:-1]:
            file.write(f"{v}, ")
        file.write(f"{value[-1]}\n")

/**
 * @file sensor_dict.h
 * Dictionary of sensors for the cardiac tissue simulation.
 *
 */

#ifndef SENSOR_DICT_H
#define SENSOR_DICT_H
#include <map>
#include <string>
#include <vector>

// File for showing the contents of any container.
#include "prettyprint.hpp"

template <typename T>
class SensorDict
{
    std::vector<std::string> data_names;
    std::map<int, std::vector<T> > sensor_info;

public:
    SensorDict(std::vector<std::string> names) : data_names(names)
    {}

    /**
     * @brief Initialize the sensor dictionary.
     */
    void Init()
    {
        sensor_info.clear();
    }

    void AddData(int node_id, const T & data)
    {
        sensor_info[node_id].push_back(data);
    }

    /**
     * @brief Get the information of all sensors.
     * @return A map where the key is the node ID and the value is a vector of sensor data.
     */
    std::map<int, std::vector<T>> GetSensorInfo() const
    {
        return sensor_info;
    }

    /**
     * @brief Get the names of the data stored in the sensors.
     * @return A vector of strings containing the names of the data.
     */
    std::vector<std::string> GetDataNames() const
    {
        return data_names;
    }

    void Show() const
    {
        std::cout << "Sensor contents:\n (";
        for (unsigned i = 0; i < data_names.size(); ++i)
        {
            std::cout << data_names[i];
            if (i < data_names.size() - 1)
                std::cout << ", ";
        }
        std::cout << ")\n";
        for (const auto &pair : sensor_info)
        {
            std::cout << "Node ID: " << pair.first << "\n";
            for (auto const &data : pair.second)
            {
                std::cout << " " << data << "\n";
            }
        }
    }
};

#endif // SENSOR_DICT_H
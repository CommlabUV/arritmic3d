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
#include "utility.h"

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

    /**
     * Show the contents of a node of the sensor dictionary.
     *
     * @param pair Pair containing the node ID and its data to show.
     * @param os Output stream to write to.
     * @param separator Separator between data values.
     */
    void ShowNode(const std::pair<int, std::vector<T>>& pair, std::ostream& os = std::cout, std::string separator = ", ") const
    {
        for (auto const &data : pair.second)
        {
            print_tuple(os, separator, data);
            os << "\n";
        }
    }

    /**
     * Show the contents of the sensor dictionary.
     *
     * @param os Output stream to write to.
     * @param separator Separator between data values.
     */
    void Show(std::ostream& os = std::cout, std::string separator = ", ") const
    {
        os << "Sensor contents:\n ";
        for (unsigned i = 0; i < data_names.size(); ++i)
        {
            os << data_names[i];
            if (i < data_names.size() - 1)
                os << separator;
        }
        os << "\n";
        for (const auto &pair : sensor_info)
        {
            os << "Node ID: " << pair.first << "\n";
            ShowNode(pair, os, separator);
        }
    }
};

#endif // SENSOR_DICT_H
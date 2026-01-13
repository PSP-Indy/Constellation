#include "DataValues.hpp"

DataValues::DataValues()
{
}

void DataValues::setValueLock(std::mutex* valueLock)
{
	this->valueLock = valueLock;
}

void DataValues::InsertDataSnapshot(float time, DataValueSnapshot data)
{
    this->value_snapshots[time] = data;
}

DataValues::DataValueList DataValues::getDataValueList()
{
    DataValueList value_list;
    for (const auto& pair : value_snapshots) {
        value_list.t_values.push_back(pair.first);

        value_list.a_values.push_back(pair.second.a_value);
        value_list.v_values.push_back(pair.second.v_value);
        value_list.x_values.push_back(pair.second.x_value);
        value_list.y_values.push_back(pair.second.y_value);
        value_list.z_values.push_back(pair.second.z_value);
        value_list.x_rot_values.push_back(pair.second.x_rot_value);
        value_list.y_rot_values.push_back(pair.second.y_rot_value);
        value_list.z_rot_values.push_back(pair.second.z_rot_value);
    }
    return value_list;
}

DataValues::~DataValues()
{
}

DataValues *DataValues::Get()
{
	if (dataValues == nullptr)
		dataValues = new DataValues();
	return dataValues;
}

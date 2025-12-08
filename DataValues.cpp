#include "DataValues.hpp"

DataValues::DataValues()
{
}

void DataValues::InsertDataSnapshot(float time, DataValueSnapshot data)
{
    value_snapshots[time] = data;
}

void DataValues::FakeData(std::mutex* valueLock)
{
	std::time_t start_time = time(NULL);
	while (true)
	{
		valueLock->lock();
		DataValues::DataValueSnapshot snapshot;
		snapshot.a_value = 0;
		snapshot.v_value = 0;
		snapshot.x_value = 0;
		snapshot.y_value = 0;
		snapshot.z_value = 0;
		snapshot.x_rot_value = 0;
		snapshot.y_rot_value = 0;
		snapshot.z_rot_value = 0;
		this->InsertDataSnapshot(difftime(time(NULL), start_time), snapshot);
		valueLock->unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
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

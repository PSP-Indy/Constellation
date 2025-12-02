#pragma once

#include <windows.h>
#include <vector> 
#include <map>

class DataValueHandler {
public:
	struct DataValueList {
		std::vector<float> t_values = {0};
		std::vector<float> a_values = {0};
		std::vector<float> v_values = {0};
		std::vector<float> x_values = {0};
		std::vector<float> y_values = {0};
		std::vector<float> z_values = {0};
		std::vector<float> x_rot_values = {0};
		std::vector<float> y_rot_values = {0};
		std::vector<float> z_rot_values = {0};
	};

	struct DataValueSnapshot {
		float a_value;
		float v_value;
		float x_value;
		float y_value;
		float z_value;
		float x_rot_value;
		float y_rot_value;
		float z_rot_value;
	};

	struct DataValues {
		std::map<float, DataValueSnapshot> value_snapshots;

		float go_grid_values[5][5];

		std::time_t launch_time = NULL;
		std::time_t last_ping = NULL;
		std::time_t coundown_start_time = NULL;

		int fuse_delay = 2;
		int launch_altitude = 0;

		HANDLE hSerialSRAD;

		void (*prime_rocket)(HANDLE hSerial, DataValues* data) = NULL;
		void (*launch_rocket)(HANDLE hSerial, DataValues* data) = NULL;

		void InsertDataSnapshot(float time, DataValueSnapshot data) {
			value_snapshots[time] = data;
		}

		DataValueList getDataValueList() 
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
	};
};

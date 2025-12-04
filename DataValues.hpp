#pragma once

#include <windows.h>
#include <vector> 
#include <map>

#include <iostream>
#include <chrono>

class DataValues {
public:
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

	float go_grid_values[5][5];

	std::time_t launch_time = NULL;
	std::time_t last_ping = NULL;
	std::time_t coundown_start_time = NULL;

	int fuse_delay = 2;
	int launch_altitude = 0;

	HANDLE hSerialSRAD;

	void (*prime_rocket)(HANDLE hSerial) = NULL;
	void (*launch_rocket)(HANDLE hSerial) = NULL;
	
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

	DataValues();
	DataValues(const DataValues& obj) = delete;

	void InsertDataSnapshot(float time, DataValueSnapshot data);

	DataValues::DataValueList getDataValueList();
	
	~DataValues();
	static DataValues* Get();

private:
	std::map<float, DataValueSnapshot> value_snapshots;

	static DataValues* dataValues;
};

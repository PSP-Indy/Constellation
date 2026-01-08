#pragma once

#include "json.hpp"

#include <vector> 
#include <map>
#include <mutex>
#include <cstdint>
#include <thread>

#include <serial/serial.h>

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

		NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DataValueSnapshot, a_value, v_value, x_value, y_value, z_value, x_rot_value, y_rot_value, z_rot_value);
	};
	
	enum class TestingMode { NONE, ONEWAYTELEM_BPS, ONEWAYTELEM_PPS, TWOWAYTELEM, ALTACCURACY, POSACCURACY };

	static std::string StringFromTestingMode(TestingMode mode)
	{
		switch (mode){
			case (TestingMode::NONE): return "No Testing Mode Selected";
			case (TestingMode::ONEWAYTELEM_BPS): return "One Way Telemetry Testing (B/S)";
			case (TestingMode::ONEWAYTELEM_PPS): return "One Way Telemetry Testing (P/S)";
			case (TestingMode::TWOWAYTELEM): return "Two Way Telemetry Testing";
			case (TestingMode::ALTACCURACY): return "Altitude Accuracy Testing";
			case (TestingMode::POSACCURACY): return "Position Accuracy Testing";
			default: return "No Testing Mode Selected";
		}
	}

	TestingMode testingMode = TestingMode::NONE;

	std::string testingData;

	bool isSRADConnected = false;

	float go_grid_values[5][5] = {0.01};

	std::time_t launch_time = 0L;
	std::time_t last_ping = 0L;
	std::time_t coundown_start_time = 0L;

	int32_t fuse_delay = 5;
	int32_t launch_altitude = 0;

	serial::Serial* hSerialSRAD;

	bool (*prime_rocket)() = NULL;
	bool (*launch_rocket)() = NULL;
	
	struct DataValueList {
		std::vector<float> t_values = {0.0f};
		std::vector<float> a_values = {0.0f};
		std::vector<float> v_values = {0.0f};
		std::vector<float> x_values = {0.0f};
		std::vector<float> y_values = {0.0f};
		std::vector<float> z_values = {0.0f};
		std::vector<float> x_rot_values = {0.0f};
		std::vector<float> y_rot_values = {0.0f};
		std::vector<float> z_rot_values = {0.0f};
	};

	DataValues();
	DataValues(const DataValues& obj) = delete;

	void InsertDataSnapshot(float time, DataValueSnapshot data);
	void FakeData(std::mutex* valueLock);

	std::map<float, DataValueSnapshot>* getValueSnapshotMap()
	{
		return &value_snapshots;
	}

	DataValues::DataValueList getDataValueList();
	
	~DataValues();
	static DataValues* Get();

private:
	std::map<float, DataValueSnapshot> value_snapshots;

	static DataValues* dataValues;
};

#include "SerialHandling.hpp"

SerialHandling::SerialHandling()
{
}


void SerialHandling::ProcessSerialDataTeleBT(HANDLE hSerial)
{
	while (true) 
	{
		char headerSubPacket[6];
		DWORD bytesRead;

		if (!ReadFile(hSerial, headerSubPacket, 5, &bytesRead, NULL)) continue;
		if (bytesRead <= 0 || strcmp(headerSubPacket, "TELEM") != 0) continue;

		char sizeSubPacket[3];

		if (!ReadFile(hSerial, sizeSubPacket, 3, &bytesRead, NULL)) continue;
		if (bytesRead <= 0) continue;

		int dataPacketSize = atoi(sizeSubPacket);
		if (dataPacketSize <= 0) continue;

		char* dataSubPacket = new char[dataPacketSize];

		if (!ReadFile(hSerial, dataSubPacket, sizeof(dataPacketSize), &bytesRead, NULL)) continue;
		if (bytesRead <= 0) continue;

		char* flightData = new char[dataPacketSize - 3];
		memcpy(flightData, dataSubPacket, (dataPacketSize - 3));

		//Operate on data here according to spec outlined in TeleMetrum section of the below file
		//https://altusmetrum.org/AltOS/doc/telemetry.pdf

		if (flightData[4] == 0x0A) //TRIGGERS IF PACKET IS TeleMetrum v2 Sensor Data
		{
			valueLock->lock();
			
			DataValueHandler::DataValueSnapshot snapshot;
			float time = (float)(CharStringToUInt16(flightData, 2)) / 100.0f;
			snapshot.a_value = (float)(CharStringToUInt16(flightData, 14));
			snapshot.v_value = (float)(CharStringToUInt16(flightData, 16));
			snapshot.z_value = (float)(CharStringToUInt16(flightData, 18));

			data->InsertDataSnapshot(time, snapshot);

			data->go_grid_values[1][2] = (float)(CharStringToUInt16(flightData, 12)) / (100.0f); 
			data->go_grid_values[1][3] = (float)(CharStringToUInt16(flightData, 20)); 
			

			valueLock->unlock();
		}
		else if (flightData[4] == 0x0B) //TRIGGERS IF PACKET IS TeleMetrum v2 Calibration Data
		{

		}
		else 
		{
			std::cout << "PACKET TYPE NOT FOUND" << std::endl;
		}

		delete[] flightData;
    	flightData = nullptr;

		delete[] dataSubPacket;
    	dataSubPacket = nullptr;
	}
}


void SerialHandling::ProcessSerialDataSRAD(HANDLE hSerial) 
{
	while (true) 
	{
		char command_buffer[9];
		DWORD bytesRead;

		if (!ReadFile(hSerial, command_buffer, sizeof(command_buffer), &bytesRead, NULL)) 
		{
			continue;
		}

		if (bytesRead <= 0) 
		{
			continue;
		}

		std::string header;
		int message_size = 0;
		for (int i = 0; i < 4; i++){
			header += command_buffer[i];
		}
		memcpy(&message_size, command_buffer + 4, 4);

		char* dataPacket = new char[message_size];

		if (!ReadFile(hSerial, dataPacket, sizeof(dataPacket), &bytesRead, NULL)) 
		{
			continue;
		}

		if (bytesRead <= 0) 
		{
			continue;
		}

		if(header == std::string("C_SC")) 
		{
			valueLock->lock();

			data->go_grid_values[0][3] = 1;
			char data_to_send[5];
			strcpy(data_to_send, "C_SS");
			DWORD bytesWritten;
			WriteFile(hSerial, data_to_send, 5, &bytesWritten, NULL);
			data->last_ping = time(NULL);

			valueLock->unlock();
		}
		
		if(header == std::string("C_TS")) 
		{
			valueLock->lock();
			
			data->go_grid_values[0][0] = 1;
			data->last_ping = time(NULL);

			valueLock->unlock();
		}

		if(header == std::string("C_FI")) 
		{
			valueLock->lock();

			data->launch_time = time(NULL);
			data->go_grid_values[0][1] = 1;
			data->last_ping = time(NULL);

			valueLock->unlock();
		}

		if(header == std::string("C_FO")) 
		{
			valueLock->lock();

			data->coundown_start_time = NULL;
			data->go_grid_values[0][2] = 1;
			data->last_ping = time(NULL);

			valueLock->unlock();
		}

		if(header == std::string("C_UT") && message_size >= 44) 
		{			
			valueLock->lock();
			data->last_ping = time(NULL);

			data->go_grid_values[1][0] = CharStringToFloat(dataPacket, 36);
			data->go_grid_values[1][1] = CharStringToFloat(dataPacket, 40);

			DataValueHandler::DataValueSnapshot snapshot;

			float time = CharStringToFloat(dataPacket, 0);
			snapshot.a_value = CharStringToFloat(dataPacket, 4);
			snapshot.v_value = CharStringToFloat(dataPacket, 8);
			snapshot.x_value = CharStringToFloat(dataPacket, 12);
			snapshot.y_value = CharStringToFloat(dataPacket, 16);
			snapshot.z_value = CharStringToFloat(dataPacket, 20);
			snapshot.x_rot_value = CharStringToFloat(dataPacket, 24);
			snapshot.y_rot_value = CharStringToFloat(dataPacket, 28);
			snapshot.z_rot_value = CharStringToFloat(dataPacket, 32);

			data->InsertDataSnapshot(time, snapshot);


			valueLock->unlock();
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

SerialHandling::~SerialHandling()
{
}

SerialHandling *SerialHandling::Get()
{
	if (serialhandling == nullptr)
		serialhandling = new SerialHandling();
	return serialhandling;
}

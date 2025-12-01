#include "SerialHandling.hpp"

SerialHandling::SerialHandling()
{
}


void SerialHandling::ProcessSerialDataTeleBT(HANDLE hSerial, UI::data_values* data)
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

			data->t_values_teleBT.push_back((float)(CharStringToUInt16(flightData, 2)));
			data->go_grid_values[0][4] = (float)(CharStringToUInt16(flightData, 12)) / (100.0f * 50.0f); 
			data->a_values_teleBT.push_back((float)(CharStringToUInt16(flightData, 6)));

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


void SerialHandling::ProcessSerialDataSRAD(HANDLE hSerial, UI::data_values* data) 
{
	while (true) 
	{
		char readBuffer[40];
		DWORD bytesRead;

		if (!ReadFile(hSerial, readBuffer, sizeof(readBuffer), &bytesRead, NULL)) 
		{
			continue;
		}

		if (bytesRead <= 0) 
		{
			continue;
		}

		std::string header;
		for (int i = 0; i < 4; i++){
			header += readBuffer[i];
		}

        std::cout << header << std::endl;

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

		if(header == std::string("C_UT")) 
		{
			valueLock->lock();

			data->last_ping = time(NULL);
			data->t_values_SRAD.push_back(CharStringToFloat(readBuffer, 4));
			data->v_values.push_back(CharStringToFloat(readBuffer, 8));
			data->a_values_SRAD.push_back(CharStringToFloat(readBuffer, 12));
			data->x_values.push_back(CharStringToFloat(readBuffer, 16));
			data->y_values.push_back(CharStringToFloat(readBuffer, 20));
			data->z_values.push_back(CharStringToFloat(readBuffer, 24));
			data->x_rot_values.push_back(CharStringToFloat(readBuffer, 28));
			data->y_rot_values.push_back(CharStringToFloat(readBuffer, 32));
			data->z_rot_values.push_back(CharStringToFloat(readBuffer, 36));


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

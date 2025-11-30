#include "SerialHandling.hpp"

SerialHandling::SerialHandling()
{
}


void SerialHandling::ProcessSerialDataTeleBT(HANDLE hSerial, UI::data_values* data)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	while (true) 
	{
		
	}
}


void SerialHandling::ProcessSerialDataSRAD(HANDLE hSerial, UI::data_values* data) 
{
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

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
			data->go_grid_values[0][3] = 1;
			char data_to_send[5];
			strcpy(data_to_send, "C_SS");
			DWORD bytesWritten;
			WriteFile(hSerial, data_to_send, 5, &bytesWritten, NULL);
			data->last_ping = time(NULL);
		}
		
		if(header == std::string("C_TS")) 
		{
			data->go_grid_values[0][0] = 1;
			data->last_ping = time(NULL);
		}

		if(header == std::string("C_FI")) 
		{
			data->launch_time = time(NULL);
			data->go_grid_values[0][1] = 1;
			data->last_ping = time(NULL);
		}

		if(header == std::string("C_FO")) 
		{
			data->coundown_start_time = NULL;
			data->go_grid_values[0][2] = 1;
			data->last_ping = time(NULL);
		}

		if(header == std::string("C_UT")) 
		{
			data->last_ping = time(NULL);

			valueLock->lock();

			data->t_values.push_back(CharStringToFloat(readBuffer, 4));
			data->v_values.push_back(CharStringToFloat(readBuffer, 8));
			data->a_values.push_back(CharStringToFloat(readBuffer, 12));
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

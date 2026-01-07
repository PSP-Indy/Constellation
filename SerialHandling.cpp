#include "SerialHandling.hpp"

SerialHandling::SerialHandling()
{
}


void SerialHandling::ProcessSerialDataTeleBT(serial::Serial* hSerial)
{
	DataValues* data = DataValues::Get();

	while (true) 
	{
		std::string headerSubPacket;
		size_t headerSubPacketSize = 6;
		size_t bytesRead;

		bytesRead = hSerial->read(headerSubPacket, headerSubPacketSize);
		if (bytesRead <= 0 || headerSubPacket != "TELEM" || bytesRead != headerSubPacketSize) continue;

		std::string sizeSubPacket;
		size_t sizeSubPacketSize = 3;

		bytesRead = hSerial->read(sizeSubPacket, sizeSubPacketSize);
		if (bytesRead <= 0 || bytesRead != sizeSubPacketSize) continue;

		int dataPacketSize = std::stoi(sizeSubPacket);
		if (dataPacketSize <= 0) continue;

		std::string dataSubPacket;

		bytesRead = hSerial->read(dataSubPacket, dataPacketSize);
		if (bytesRead <= 0 || bytesRead != dataPacketSize) continue;

		std::string flightData = dataSubPacket.substr(0, dataPacketSize - 3);

		//Operate on data here according to spec outlined in TeleMetrum section of the below file
		//https://altusmetrum.org/AltOS/doc/telemetry.pdf

		if (flightData.at(4) == 0x0A) //TRIGGERS IF PACKET IS TeleMetrum v2 Sensor Data
		{
			valueLock->lock();
			
			DataValues::DataValueSnapshot snapshot;
			float time = (float)(StringToUInt16(flightData, 2)) / 100.0f;
			snapshot.a_value = (float)(StringToUInt16(flightData, 14));
			snapshot.v_value = (float)(StringToUInt16(flightData, 16));
			snapshot.z_value = (float)(StringToUInt16(flightData, 18));

			data->InsertDataSnapshot(time, snapshot);

			data->go_grid_values[1][2] = (float)(StringToUInt16(flightData, 12)) / 100.0f; 
			data->go_grid_values[1][3] = (float)(StringToUInt16(flightData, 20)); 
			

			valueLock->unlock();
		}
		else if (flightData.at(4) == 0x0B) //TRIGGERS IF PACKET IS TeleMetrum v2 Calibration Data
		{

		}
		else 
		{
			std::cout << "PACKET TYPE NOT FOUND" << std::endl;
		}
	}
}


void SerialHandling::ProcessSerialDataSRAD() 
{
	DataValues* data = DataValues::Get();
	
	valueLock->lock();
	serial::Serial* hSerial = data->hSerialSRAD;
	valueLock->unlock();

	while (true) 
	{
		std::string commandBuffer;
		size_t commandBufferSize = 9;
		size_t bytesRead;

		int message_size = 0;
		
		bytesRead = hSerial->read(commandBuffer, commandBufferSize);
		if (bytesRead <= 0 || bytesRead != commandBufferSize) continue;

		int messageSize = StringToUInt32(commandBuffer, 4);
		std::string header = commandBuffer.substr(0,4);

		std::string messageBuffer;

		bytesRead = hSerial->read(messageBuffer, (size_t)messageSize);
		if (bytesRead <= 0 || bytesRead != (size_t)messageSize) continue;

		if(header == "C_SC") 
		{
			valueLock->lock();

			data->isSRADConnected = true;
			std::string data_to_send = "C_SS";
			size_t bytesWritten = hSerial->write(data_to_send);
			data->last_ping = time(NULL);

			valueLock->unlock();
		}
		
		if(header == "C_TS") 
		{
			valueLock->lock();
			
			data->go_grid_values[0][0] = 1;
			data->last_ping = time(NULL);

			valueLock->unlock();
		}

		if(header == "C_FI") 
		{
			valueLock->lock();

			data->launch_time = time(NULL);
			data->go_grid_values[0][1] = 1;
			data->last_ping = time(NULL);

			valueLock->unlock();
		}

		if(header == "C_FO") 
		{
			valueLock->lock();

			data->coundown_start_time = NULL;
			data->go_grid_values[0][2] = 1;
			data->last_ping = time(NULL);

			valueLock->unlock();
		}

		if(header == "T_DP")
		{
			data->testingData = messageBuffer;
		}

		if(header == "C_UT" && message_size >= 44) 
		{			
			valueLock->lock();
			
			data->last_ping = time(NULL);

			data->go_grid_values[1][0] = StringToFloat(messageBuffer, 36);
			data->go_grid_values[1][1] = StringToFloat(messageBuffer, 40);

			if (message_size >= 45 && messageBuffer[45] != '\0')
			{
				data->go_grid_values[4][0] = static_cast<float>((bool)messageBuffer[45]);
			}

			DataValues::DataValueSnapshot snapshot;

			float time = StringToFloat(messageBuffer, 0);
			snapshot.a_value = StringToFloat(messageBuffer, 4);
			snapshot.v_value = StringToFloat(messageBuffer, 8);
			snapshot.x_value = StringToFloat(messageBuffer, 12);
			snapshot.y_value = StringToFloat(messageBuffer, 16);
			snapshot.z_value = StringToFloat(messageBuffer, 20);
			snapshot.x_rot_value = StringToFloat(messageBuffer, 24);
			snapshot.y_rot_value = StringToFloat(messageBuffer, 28);
			snapshot.z_rot_value = StringToFloat(messageBuffer, 32);
			

			data->InsertDataSnapshot(time, snapshot);


			valueLock->unlock();
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

bool SerialHandling::SendSerialData(serial::Serial* hSerial, const char* dataPacket)
{
	if (hSerial == nullptr) return false;

	return hSerial->write(dataPacket) == sizeof(dataPacket);
}

bool SerialHandling::SendSRADData(const char* dataPacket)
{
	return this->SendSerialData(DataValues::Get()->hSerialSRAD, dataPacket);
}

void SerialHandling::FindSerialLocations(std::string* sradloc, std::string* telebtloc)
{
	std::vector<serial::PortInfo> devices_found = serial::list_ports();

	std::vector<serial::PortInfo>::iterator iter = devices_found.begin();

	while( iter != devices_found.end() )
	{
		serial::PortInfo device = *iter++;

		serial::Serial port(device.port, 115200, serial::Timeout::simpleTimeout(1000));

		std::string regPacket;
		size_t regPacketSize = 32;

		size_t bytesWritten = port.read(regPacket, regPacketSize);

		if (bytesWritten != regPacketSize) continue;

		if (regPacket.at(4) == 0x01) *telebtloc = std::string(device.port.c_str());
		if (regPacket.at(4) == 0x06) *sradloc = std::string(device.port.c_str());
	}
}

bool SerialHandling::CreateSerialFile(serial::Serial* hSerial, std::string serialLoc)
{
	serial::Serial port(serialLoc, 11520, serial::Timeout::simpleTimeout(1000));
	hSerial = &port;
	return port.isOpen();
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

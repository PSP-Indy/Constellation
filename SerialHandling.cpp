#include "SerialHandling.hpp"

SerialHandling::SerialHandling()
{
}

void SerialHandling::FakeData()
{
	DataValues* data = DataValues::Get();
	std::mutex* valueLock = data->valueLock;
	const auto start_time = std::chrono::steady_clock::now();
	auto lastTime = std::chrono::steady_clock::now();
	while (true)
	{
		DataValues::DataValueSnapshot snapshot;
		DataValues::DataValueList currentValueList = data->getDataValueList();

		std::chrono::duration<double, std::milli> timeSinceStartDuration = std::chrono::steady_clock::now() - start_time;
		double timeSinceStart = timeSinceStartDuration.count() / 1000.0;

		std::chrono::duration<double, std::milli> dtDuration = std::chrono::steady_clock::now() - lastTime;
		double dt = dtDuration.count() / 1000.0;

		snapshot.a_value = timeSinceStart < 10 ? 9.81f : -9.81f;
		snapshot.v_value = currentValueList.v_values.back() + currentValueList.a_values.back() * dt;
		snapshot.x_value = 0;
		snapshot.y_value = 0;
		snapshot.z_value = currentValueList.z_values.back() + currentValueList.v_values.back() * dt;
		snapshot.x_rot_value = 0;
		snapshot.y_rot_value = 0;
		snapshot.z_rot_value = fmod(timeSinceStart, (2.0 * 3.1415926535));

		valueLock->lock();
		data->InsertDataSnapshot(timeSinceStart, snapshot);
		valueLock->unlock();

		lastTime = std::chrono::steady_clock::now();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void SerialHandling::ProcessSerialDataTeleBT(serial::Serial* hSerial)
{
	DataValues* data = DataValues::Get();
	std::mutex* valueLock = data->valueLock;

	while (true) 
	{
		size_t headerSubPacketSize = 6;
		uint8_t headerSubPacketBuffer[7];

		size_t bytesRead = hSerial->read(headerSubPacketBuffer, headerSubPacketSize);
		if (bytesRead != headerSubPacketSize) continue;
		std::string headerSubPacket(reinterpret_cast<const char*>(headerSubPacketBuffer), bytesRead);
		if (headerSubPacket != "TELEM") continue;

		size_t sizeSubPacketSize = 6;
		uint8_t sizeSubPacketBuffer[7];

		bytesRead = hSerial->read(sizeSubPacketBuffer, sizeSubPacketSize);
		if (bytesRead != sizeSubPacketSize) continue;
		std::string sizeSubPacket(reinterpret_cast<const char*>(sizeSubPacketBuffer), bytesRead);

		size_t dataPacketSize = (size_t)std::stoi(sizeSubPacket);
		if (dataPacketSize <= 0) continue;

		std::string dataSubPacket;

		bytesRead = hSerial->read(dataSubPacket, dataPacketSize);
		if (bytesRead != dataPacketSize) continue;

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

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void SerialHandling::ProcessSerialDataSRAD() 
{
	DataValues* data = DataValues::Get();
	std::mutex* valueLock = data->valueLock;

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	
	valueLock->lock();
	serial::Serial* hSerial = data->hSerialSRAD;
	valueLock->unlock();

	while (true) 
	{
		size_t bytesRead;
		uint8_t commandBuffer[7];

		while (hSerial->available() < 6);

		try {
			bytesRead = hSerial->read(commandBuffer, 6);
		} catch (const serial::IOException& e) { return; }

		if (bytesRead != 6) continue;
		std::string command(reinterpret_cast<const char*>(commandBuffer), bytesRead);

		int messageSize = StringToUInt16(command, 4);
		std::string header = command.substr(0,4);

		std::string messageBuffer;
		
		try {
			bytesRead = hSerial->read(messageBuffer, messageSize);
		} catch (const serial::IOException& e) { return; }

		if (bytesRead != messageSize) continue;

		if(header == "C_SC") 
		{
			valueLock->lock();

			if(messageBuffer == "C_LC")
			{
				std::cout << messageBuffer << std::endl;
				data->go_grid_values[1][4] = 1;
			}

			data->isSRADConnected = true;
			SendSRADData("C_SS");
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

			data->coundown_start_time = 0L;
			data->go_grid_values[0][2] = 1;
			data->last_ping = time(NULL);

			valueLock->unlock();
		}

		if(header == "T_DP")
		{
			data->testingData = messageBuffer;
		}


		if(header == "C_UT" && messageSize >= 44) 
		{			
			valueLock->lock();
			
			data->last_ping = time(NULL);

			data->go_grid_values[1][0] = StringToFloat(messageBuffer, 36);
			data->go_grid_values[1][1] = StringToFloat(messageBuffer, 40);

			if (messageSize >= 45 && messageBuffer[45] != '\0')
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

bool SerialHandling::SendRawSerialData(serial::Serial* hSerial, const uint8_t* dataPacket, size_t length)
{
	if (hSerial == nullptr) return false;

	return hSerial->write(dataPacket, length) == length;
}

bool SerialHandling::SendSRADData(const uint8_t* dataPacket, size_t length)
{
	return SendRawSerialData(DataValues::Get()->hSerialSRAD, dataPacket, length);
}

bool SerialHandling::SendSRADData(const char* dataPacket)
{
	return SendRawSerialData(DataValues::Get()->hSerialSRAD, reinterpret_cast<const uint8_t*>(dataPacket), strlen(dataPacket));
}

void SerialHandling::FindSerialLocations(std::string* sradloc, std::string* telebtloc)
{
	std::vector<serial::PortInfo> devices_found = serial::list_ports();

	std::vector<serial::PortInfo>::iterator iter = devices_found.begin();

	while ( iter != devices_found.end() )
	{
		serial::PortInfo device = *iter++;
		std::string regPacket;
		
		try 
		{
			serial::Serial port(device.port, 115200, serial::Timeout::simpleTimeout(1000));

			port.setDTR(true);
			port.setRTS(true);

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			port.flush();
			if (port.available()) {
                std::string garbage;
                port.read(garbage, port.available());
            }

            for (int i = 0; i < 15 && !port.available(); i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

			size_t bytesRead = port.read(regPacket, 8);
			port.close();

			if (bytesRead >= 5)
			{
				if (regPacket.at(4) == 0x01) *telebtloc = device.port;
				if (regPacket.at(0) == 'C') *sradloc = device.port;
			}
		}
		catch (const std::exception& e)
		{
			std::cout << "Exception during port scan: " << e.what() << " at port " << device.port << std::endl;
			continue;
		}
	}
}

bool SerialHandling::CreateSerialFile(serial::Serial* hSerial, std::string serialLoc)
{
	try
	{
		if (hSerial->isOpen())
    		hSerial->close();

		hSerial->setPort(serialLoc);
		hSerial->setBaudrate(115200);
		serial::Timeout timeout = serial::Timeout::simpleTimeout(1000);
		hSerial->setTimeout(timeout);
		hSerial->open();

		hSerial->setDTR(true);
		hSerial->setRTS(true);

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		hSerial->flush();
		if (hSerial->available()) {
			std::string garbage;
			hSerial->read(garbage, hSerial->available());
		}
		
		for (int i = 0; i < 15 && !hSerial->available(); i++) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
		
		return hSerial->isOpen();
	}
	catch (const serial::IOException& e)
	{
		return false;
	}
}

SerialHandling::~SerialHandling()
{
}
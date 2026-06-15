#include "SerialHandling.hpp"

SerialHandling::SerialHandling()
{
}

void SerialHandling::ProcessSerialData() 
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
		
		if (!hSerial->isOpen()) { continue; }

		while (hSerial->available() < 6) std::this_thread::sleep_for(std::chrono::milliseconds(1));

		try {
			bytesRead = hSerial->read(commandBuffer, 6);
		} catch (const serial::IOException& e) { continue; }

		if (bytesRead != 6) continue;
		std::string command(reinterpret_cast<const char*>(commandBuffer), bytesRead);

		uint16_t messageSize = StringToUInt16(command, 4);
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
			rocket_data->rocket_primed = true;
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

		if(header == "C_UT") 
		{
			if (messageSize >= 44)
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
		}
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
				if (regPacket.at(4) == 0x01)
				{
					*telebtloc = device.port;
				} 
				if (regPacket.at(0) == 'C') 
				{
					*sradloc = device.port;
					std::cout << "Found SRAD on port " << device.port << std::endl;
				}
				
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
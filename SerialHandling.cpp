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
		if (!hSerial->isOpen()) { continue; }

		size_t bytes_available = hSerial->available();

		if (bytes_available == 0) { continue; }

		hSerial->read(serial_input_buffer + bytes_written, bytes_available);
		bytes_written += bytes_available;

		int packet_start;
		int packet_end;

		bool packet_available = IsPacketAvailable(&packet_start, &packet_end);
		
		if (!packet_available) { continue; }
		
		std::string header;
		std::string message;

		ParsePacket(&header, &message, packet_start, packet_end);

		if(header == "C_SC")
		{
			valueLock->lock();

			if(message == "C_LC")
			{
				data->go_grid_values[1][4] = 1;
			}

			data->isSRADConnected = true;
			SendSRADSync();
			data->last_ping = time(NULL);

			valueLock->unlock();
		}
		
		if(header == "C_TS") 
		{
			valueLock->lock();
			
			data->go_grid_values[0][0] = 1;
			data->rocket_primed = true;
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
			if (message.size() >= 44)
			{
				valueLock->lock();
			
				data->last_ping = time(NULL);

				data->go_grid_values[1][0] = StringToFloat(message, 36);
				data->go_grid_values[1][1] = StringToFloat(message, 40);

				if (message.size() >= 45 && message[45] != '\0')
				{
					data->go_grid_values[4][0] = static_cast<float>((bool)message[45]);
				}

				DataValues::DataValueSnapshot snapshot;

				float time = StringToFloat(message, 0);
				snapshot.a_value = StringToFloat(message, 4);
				snapshot.v_value = StringToFloat(message, 8);
				snapshot.x_value = StringToFloat(message, 12);
				snapshot.y_value = StringToFloat(message, 16);
				snapshot.z_value = StringToFloat(message, 20);
				snapshot.x_rot_value = StringToFloat(message, 24);
				snapshot.y_rot_value = StringToFloat(message, 28);
				snapshot.z_rot_value = StringToFloat(message, 32);
				

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

bool SerialHandling::SendSRADData(int launch_altitude, bool pop_booster, bool pop_drogue, bool pop_main)
{
	const char header[] = "C_SD";
	if (!SendRawSerialData(DataValues::Get()->hSerialSRAD, reinterpret_cast<const uint8_t*>(header), 4))
	{
		return false;
	}

	uint8_t data_packet[12];
	memcpy(&data_packet[0], &launch_altitude, 4);

	return SendRawSerialData(DataValues::Get()->hSerialSRAD, data_packet, 12);
}

bool SerialHandling::SendSRADSync()
{
	const char header[] = "C_SS";
	return SendRawSerialData(DataValues::Get()->hSerialSRAD, reinterpret_cast<const uint8_t*>(header), 4);
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

bool SerialHandling::IsPacketAvailable(int* packet_start, int* packet_end)
{
	*packet_start = 0;
	*packet_end = 0;

	int i = 0;
	while (*packet_start == 0 || i < bytes_written - 1)
	{
		if (serial_input_buffer[i] == 'C' && serial_input_buffer[i + 1] == '_')
		{
			*packet_start = i;
		}

		i++;
	}

	if (i = bytes_written - 1) { return false; }

	i = *packet_start;
	while (*packet_end == 0 || i < bytes_written - 1)
	{
		if (serial_input_buffer[i] == 'C' && serial_input_buffer[i + 1] == '_')
		{
			*packet_end = i - 1;

			return true;
		}

		i++;
	}

	return false;
}

void SerialHandling::ParsePacket(std::string* header, std::string* message, int packet_start, int packet_end)
{
	size_t packet_length = packet_end - packet_start;

	std::string packet(reinterpret_cast<const char*>(serial_input_buffer + packet_start), packet_length);
	*header = packet.substr(0, 4);
	*message = packet.substr(4, packet_length - 4);

	std::copy(serial_input_buffer + packet_start, serial_input_buffer + bytes_written, serial_input_buffer);
	bytes_written = 0;
}

SerialHandling::~SerialHandling()
{
}
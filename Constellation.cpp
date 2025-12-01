#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <random>
#include <cmath>

#include <windows.h>

#include "UI.hpp"
#include "SerialHandling.hpp"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

std::mutex valueLock;

UI* UI::ui = new UI();
SerialHandling* SerialHandling::serialhandling = new SerialHandling();

void WriteDataToFile(std::vector<float> data, std::string label, std::ofstream* outputFile) 
{
	*outputFile << label.c_str() << ",";
	for (size_t i = 0; i < data.size(); ++i) {
		*outputFile << data[i];
		if (i < data.size() - 1) {
			*outputFile << ",";
		}
	}
	*outputFile << std::endl;
}

void PrimeRocket(HANDLE hSerial, UI::data_values* data)
{
	char header[5];
	strcpy(header, "C_ST");
	DWORD bytesWrittenHeader;
	WriteFile(hSerial, header, 5, &bytesWrittenHeader, NULL);
	
	char dataToSend[32];
	DWORD bytesWrittenData;
	memcpy(dataToSend, &(data->fuse_delay), 4);
	memcpy(dataToSend + 4, &(data->launch_altitude), 4);

	WriteFile(hSerial, dataToSend, 32, &bytesWrittenData, NULL);
}

void LaunchRocket(HANDLE hSerial, UI::data_values* data)
{
	data->coundown_start_time = time(NULL);

	char data_to_send[5];
	strcpy(data_to_send, "C_LR");
	DWORD bytesWritten;
	WriteFile(hSerial, data_to_send, 5, &bytesWritten, NULL);
}

int main()
{
	//GLOBAL USE VARIABLES
	UI::data_values data;

	//GUI INITIALIZATION
	UI* gui = UI::Get();
	SerialHandling* serialHandling = SerialHandling::Get();

	gui->AssignValues(&data);
	serialHandling->SetValueLock(&valueLock);

	//SERIAL INITIALIZATION
	HANDLE hSerialSRAD = CreateFile("\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	HANDLE hSerialTeleBT = CreateFile("\\\\.\\COM4", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	DCB dcbSerialParamsSRAD = {0};
	dcbSerialParamsSRAD.DCBlength = sizeof(dcbSerialParamsSRAD);

	DCB dcbSerialParamsTeleBT = {0};
	dcbSerialParamsTeleBT.DCBlength = sizeof(dcbSerialParamsTeleBT);

	if (!GetCommState(hSerialSRAD, &dcbSerialParamsSRAD) && !GetCommState(hSerialTeleBT, &dcbSerialParamsTeleBT)) {
		std::cout << "Failed to get comm state, aborting serial communication." << std::endl;
	} else {
		dcbSerialParamsSRAD.BaudRate = CBR_9600;
		dcbSerialParamsSRAD.ByteSize = 8;
		dcbSerialParamsSRAD.StopBits = ONESTOPBIT;
		dcbSerialParamsSRAD.Parity = NOPARITY;

		dcbSerialParamsTeleBT.BaudRate = CBR_9600;
		dcbSerialParamsTeleBT.ByteSize = 8;
		dcbSerialParamsTeleBT.StopBits = ONESTOPBIT;
		dcbSerialParamsTeleBT.Parity = NOPARITY;

		if (!SetCommState(hSerialSRAD, &dcbSerialParamsSRAD) && !SetCommState(hSerialTeleBT, &dcbSerialParamsTeleBT)) {
			std::cout << "Failed to set comm state, aborting serial communication." << std::endl;
		} else {
			COMMTIMEOUTS timeoutsSRAD = {0};
			timeoutsSRAD.ReadIntervalTimeout = 50;
			timeoutsSRAD.ReadTotalTimeoutConstant = 50;
			timeoutsSRAD.ReadTotalTimeoutMultiplier = 10;
			timeoutsSRAD.WriteTotalTimeoutConstant = 50;
			timeoutsSRAD.WriteTotalTimeoutMultiplier = 10;

			COMMTIMEOUTS timeoutsTeleBT = {0};
			timeoutsTeleBT.ReadIntervalTimeout = 50;
			timeoutsTeleBT.ReadTotalTimeoutConstant = 50;
			timeoutsTeleBT.ReadTotalTimeoutMultiplier = 10;
			timeoutsTeleBT.WriteTotalTimeoutConstant = 50;
			timeoutsTeleBT.WriteTotalTimeoutMultiplier = 10;

			if (!SetCommTimeouts(hSerialSRAD, &timeoutsSRAD) && !SetCommTimeouts(hSerialTeleBT, &timeoutsTeleBT)) {
				std::cout << "Failed to set timouts, aborting serial communication." << std::endl;
			} else {
				data.prime_rocket = PrimeRocket;
				data.launch_rocket = LaunchRocket;

				data.hSerialSRAD = hSerialSRAD;
				std::thread serial_thread_SRAD(&SerialHandling::ProcessSerialDataSRAD, serialHandling, hSerialSRAD, &data);
				std::thread serial_thread_TeleBT(&SerialHandling::ProcessSerialDataTeleBT, serialHandling, hSerialTeleBT, &data);

				serial_thread_SRAD.detach();
				serial_thread_TeleBT.detach();
			}
		}
	}
	
	if (!glfwInit())
		return 2;

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Constellation 0.1.1", NULL, NULL);

	if (window == NULL)
		return 2;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw("Unable to context to OpenGL");

	int screen_width, screen_height;
	glfwGetFramebufferSize(window, &screen_width, &screen_height);
	glViewport(0, 0, screen_width, screen_height);

	gui->Init(window, glsl_version);

	while (!glfwWindowShouldClose(window)) {
		if (std::abs(difftime(data.last_ping, time(NULL))) > 5)
		{
			data.go_grid_values[0][3] = 0;
		}

		auto start = std::chrono::high_resolution_clock::now();
		glfwPollEvents();
		gui->NewFrame();
		gui->Update();
		gui->Render(window);
	}

	gui->Shutdown();
	CloseHandle(hSerialSRAD);
	CloseHandle(hSerialTeleBT);

	std::ofstream outputFile;
	outputFile.open("DATA.csv", std::ios::out); 
	if (outputFile.is_open()) {
		char time_string[11];
		if (data.launch_time != NULL)
		{
			std::tm* tm_time = std::localtime(&(data.launch_time));
			std::strftime(time_string, sizeof(time_string), "%H:%M:%S", tm_time);
		}
		else
		{
			strncpy(time_string, "NO_LAUNCH", 11);
		}	

		char date_string[11];
		time_t current_time = time(NULL);
		std::tm *localTime = std::localtime(&current_time);
		std::strftime(date_string, sizeof(date_string), "%m/%d/%Y", localTime);

		WriteDataToFile(data.a_values_SRAD, std::string("acceleration (SRAD)"), &outputFile);
		WriteDataToFile(data.a_values_teleBT, std::string("acceleration (TeleBT)"), &outputFile);
		WriteDataToFile(data.v_values, std::string("velocity"), &outputFile);
		WriteDataToFile(data.a_values_SRAD, std::string("time (SRAD)"), &outputFile);
		WriteDataToFile(data.a_values_teleBT, std::string("time (TeleBT)"), &outputFile);
		WriteDataToFile(data.x_values, std::string("positionX"), &outputFile);
		WriteDataToFile(data.y_values, std::string("positionY"), &outputFile);
		WriteDataToFile(data.z_values, std::string("positionZ"), &outputFile);
		WriteDataToFile(data.x_rot_values, std::string("rotationX"), &outputFile);
		WriteDataToFile(data.y_rot_values, std::string("rotationY"), &outputFile);
		WriteDataToFile(data.z_rot_values, std::string("rotationZ"), &outputFile);

		outputFile << "launchTime," << time_string << std::endl;
		outputFile << "launchDate," << date_string << std::endl;
		outputFile << "fuseDelay," << data.fuse_delay << std::endl;
		outputFile << "launchAltitude," << data.launch_altitude << std::endl;

        outputFile.close();
    }

	delete gui;

	return 0;
}
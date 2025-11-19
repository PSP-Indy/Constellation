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
	HANDLE hSerial = CreateFile("\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	DCB dcbSerialParams = {0};
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	if (!GetCommState(hSerial, &dcbSerialParams)) {
		std::cout << "Failed to get comm state, aborting serial communication." << std::endl;
	} else {
		dcbSerialParams.BaudRate = CBR_9600;
		dcbSerialParams.ByteSize = 8;
		dcbSerialParams.StopBits = ONESTOPBIT;
		dcbSerialParams.Parity = NOPARITY;

		if (!SetCommState(hSerial, &dcbSerialParams)) {
			std::cout << "Failed to set comm state, aborting serial communication." << std::endl;
		} else {
			COMMTIMEOUTS timeouts = {0};
			timeouts.ReadIntervalTimeout = 50;
			timeouts.ReadTotalTimeoutConstant = 50;
			timeouts.ReadTotalTimeoutMultiplier = 10;
			timeouts.WriteTotalTimeoutConstant = 50;
			timeouts.WriteTotalTimeoutMultiplier = 10;

			if (!SetCommTimeouts(hSerial, &timeouts)) {
				std::cout << "Failed to set timouts, aborting serial communication." << std::endl;
			} else {
				data.prime_rocket = PrimeRocket;
				data.launch_rocket = LaunchRocket;

				data.hSerial = hSerial;
				std::thread serial_thread(&SerialHandling::ProcessSerialData, serialHandling, hSerial, &data);
				serial_thread.detach();
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
	CloseHandle(hSerial);

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

		WriteDataToFile(data.a_values, std::string("acceleration"), &outputFile);
		WriteDataToFile(data.v_values, std::string("velocity"), &outputFile);
		WriteDataToFile(data.t_values, std::string("time"), &outputFile);
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
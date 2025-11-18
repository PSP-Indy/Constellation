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

#include "glad/glad.h"
#include "GLFW/glfw3.h"

using namespace std::chrono;

std::mutex valueLock;

UI* UI::ui = new UI();

float CharStringToFloat(char* charString, int idx) {
	float cpy_flt;
	memcpy(&cpy_flt, charString, 4);
	return cpy_flt;
}

int CharStringToInt(char* charString, int idx) {
	int cpy_int;
	memcpy(&cpy_int, charString, 4);
	return cpy_int;
}

void ProcessSerialData(HANDLE hSerial, UI::data_values* data) {
	while (true) 
	{
		char readBuffer[27];
		DWORD bytesRead;

		if (!ReadFile(hSerial, readBuffer, sizeof(readBuffer), &bytesRead, NULL)) {
			std::cout << "Error reading serial buffer!" << std::endl;
		}

		std::string header;
		for (int i = 0; i < 4; i++){
			header += readBuffer[i];
		}
		
		if(header == std::string("C_TS")) 
		{
			data->coundown_start_time = time(NULL);

			data->go_grid_values[0][0] = 1;
		}

		if(header == std::string("C_FI")) 
		{
			data->launch_time = time(NULL);
			data->go_grid_values[0][1] = 1;
		}

		if(header == std::string("C_FO")) 
		{
			data->coundown_start_time = NULL;
			data->go_grid_values[0][2] = 1;
		}

		if(header == std::string("C_UT")) 
		{
			valueLock.lock();

			data->t_values.push_back(CharStringToFloat(readBuffer, 4));
			data->v_values.push_back(CharStringToFloat(readBuffer, 8));
			data->a_values.push_back(CharStringToFloat(readBuffer, 12));
			data->x_values.push_back(CharStringToFloat(readBuffer, 16));
			data->y_values.push_back(CharStringToFloat(readBuffer, 20));
			data->z_values.push_back(CharStringToFloat(readBuffer, 24));
			data->x_rot_values.push_back(CharStringToFloat(readBuffer, 28));
			data->y_rot_values.push_back(CharStringToFloat(readBuffer, 32));
			data->z_rot_values.push_back(CharStringToFloat(readBuffer, 36));


			valueLock.unlock();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

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

void LaunchRocket(HANDLE hSerial, UI::data_values* data)
{
	// FOR TESTING PURPOSES, COMMENT ON ACTUAL BUILDS:
	data->coundown_start_time = time(NULL);

	char dataToSend[32];
	DWORD bytesWritten;

	memcpy(dataToSend, &(data->fuse_delay), 4);
	memcpy(dataToSend + 4, &(data->launch_altitude), 4);

	//WriteFile(hSerial, dataToSend, 32, &bytesWritten, NULL);
}

int main()
{
	//GLOBAL USE VARIABLES
	UI::data_values data;

	data.launch_rocket = LaunchRocket;

	//SERIAL INITIALIZATION
	HANDLE hSerial = CreateFile("\\\\.\\COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

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
				data.launch_rocket = LaunchRocket;
				data.hSerial = hSerial;
				std::thread serial_thread(ProcessSerialData, hSerial, &data);
				serial_thread.detach();
			}
		}
	}

	//GUI INITIALIZATION
	UI* gui = UI::Get();

	gui->AssignValues(&data);
	
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
		auto start = high_resolution_clock::now();
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
		WriteDataToFile(data.a_values, std::string("acceleration"), &outputFile);
		WriteDataToFile(data.v_values, std::string("velocity"), &outputFile);
		WriteDataToFile(data.t_values, std::string("time"), &outputFile);
		WriteDataToFile(data.x_values, std::string("positionX"), &outputFile);
		WriteDataToFile(data.y_values, std::string("positionY"), &outputFile);
		WriteDataToFile(data.z_values, std::string("positionZ"), &outputFile);
		WriteDataToFile(data.x_rot_values, std::string("rotationX"), &outputFile);
		WriteDataToFile(data.y_rot_values, std::string("rotationY"), &outputFile);
		WriteDataToFile(data.z_rot_values, std::string("rotationZ"), &outputFile);

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

		outputFile << "launchTime" << "," << time_string << std::endl;

        outputFile.close();
    }

	delete gui;

	return 0;
}
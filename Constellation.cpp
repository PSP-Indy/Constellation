/**
 * @file Constellation.cpp
 * @brief This is the main entry point of the program.
 *
 * This file contains the main function and initializes all subsystems.
 */

#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <random>
#include <cmath>

#include <Windows.h>

#include "UI.hpp"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

using namespace std::chrono;

std::mutex valueLock;

UI* UI::ui = new UI();

float CharStringToFloat(char* charString, int idx) {
	union {
		float f;
		char b[4];
	} u;
	u.b[3] = charString[idx + 3];
	u.b[2] = charString[idx + 2];
	u.b[1] = charString[idx + 1];
	u.b[0] = charString[idx];
	return u.f;
}

void FakeSerialData(UI::data_values* data)
{
	std::random_device rd;
	std::mt19937 engine(rd());
	std::uniform_int_distribution<int> dist(1, 100);

	while (true) 
	{
		valueLock.lock();

		data->t_values.push_back(data->t_values.back() + 0.5f);
		data->v_values.push_back(dist(engine));
		data->a_values.push_back(dist(engine));
		data->x_values.push_back(dist(engine));
		data->y_values.push_back(dist(engine));
		data->z_values.push_back(dist(engine));
		data->x_rot_values.push_back(dist(engine));
		data->y_rot_values.push_back(dist(engine));
		data->z_rot_values.push_back(dist(engine));

		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				data->go_grid_values[i][j] = fmod((data->go_grid_values[i][j] + (dist(engine) / 1000.0f)), 1.0f);
			}
		}

		valueLock.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void ProcessSerialData(HANDLE hSerial, UI::data_values* data) {
	while (true) 
	{
		char readBuffer[27];
		DWORD bytesRead;

		if (!ReadFile(hSerial, readBuffer, sizeof(readBuffer), &bytesRead, NULL)) {
			std::cout << "Error reading serial buffer!" << std::endl;
		} else {
			valueLock.lock();

			data->t_values.push_back(CharStringToFloat(readBuffer, 0));
			data->v_values.push_back(CharStringToFloat(readBuffer, 4));
			data->a_values.push_back(CharStringToFloat(readBuffer, 8));
			data->x_values.push_back(CharStringToFloat(readBuffer, 12));
			data->y_values.push_back(CharStringToFloat(readBuffer, 16));
			data->z_values.push_back(CharStringToFloat(readBuffer, 20));
			data->x_rot_values.push_back(CharStringToFloat(readBuffer, 24));
			data->y_rot_values.push_back(CharStringToFloat(readBuffer, 28));
			data->z_rot_values.push_back(CharStringToFloat(readBuffer, 32));


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

}

int main()
{
	//GLOBAL USE VARIABLES

	UI::data_values data;

	std::thread serial_thread(FakeSerialData, &data);
	serial_thread.detach();

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

	const char* glsl_version = "#version 120";
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Constellation", NULL, NULL);

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
		WriteDataToFile(data.x_values, std::string("positionX"), &outputFile);
		WriteDataToFile(data.y_values, std::string("positionY"), &outputFile);
		WriteDataToFile(data.z_values, std::string("positionZ"), &outputFile);
		WriteDataToFile(data.x_rot_values, std::string("rotationX"), &outputFile);
		WriteDataToFile(data.y_rot_values, std::string("rotationY"), &outputFile);

        outputFile.close();
    }

	delete gui;

	return 0;
}


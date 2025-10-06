/**
 * @file Constellation.cpp
 * @brief This is the main entry point of the program.
 *
 * This file contains the main function and initializes all subsystems.
 */

#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

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

void ProcessSerialData(HANDLE hSerial, std::vector<float>* t_values, std::vector<float>* v_values, std::vector<float>* x_values, std::vector<float>* y_values, std::vector<float>* z_values, std::atomic<float>* x_rotation, std::atomic<float>* y_rotation) {
	while (true) 
	{
		char readBuffer[27];
		DWORD bytesRead;

		if (!ReadFile(hSerial, readBuffer, sizeof(readBuffer), &bytesRead, NULL)) {
			std::cout << "Error reading serial buffer!" << std::endl;
		} else {
			valueLock.lock();
			t_values->push_back(CharStringToFloat(readBuffer, 0));
			v_values->push_back(CharStringToFloat(readBuffer, 4));
			x_values->push_back(CharStringToFloat(readBuffer, 8));
			y_values->push_back(CharStringToFloat(readBuffer, 12));
			z_values->push_back(CharStringToFloat(readBuffer, 16));
			*x_rotation = CharStringToFloat(readBuffer, 20);
			*y_rotation = CharStringToFloat(readBuffer, 24);
			valueLock.unlock();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

int main()
{
	//GLOBAL USE VARIABLES

	std::vector<float> t_values({0, 1, 2, 3});
	std::vector<float> v_values({0, 10, 20, 30});
	std::vector<float> x_values({0, 0, 0, 0});
	std::vector<float> y_values({0, 0, 0, 0});
	std::vector<float> z_values({0, 10, 30, 60});
	std::atomic<float> x_rotation = 0.1f;
	std::atomic<float> y_rotation = 0.2f;

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
				std::thread serial_thread(ProcessSerialData, hSerial, &t_values, &v_values, &x_values, &y_values, &z_values, &x_rotation, &y_rotation);
				serial_thread.detach();
			}
		}
	}

	//GUI INITIALIZATION
	UI* gui = UI::Get();

	gui->assignValues(gui, &valueLock, &t_values, &v_values, &x_values, &y_values, &z_values, &x_rotation, &y_rotation);
	
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

	bool firstFrame = true;

	while (!glfwWindowShouldClose(window)) {
		auto start = high_resolution_clock::now();
		glfwPollEvents();
		gui->NewFrame();
		gui->Update();
		gui->Render(window);
	}

	gui->Shutdown();
	CloseHandle(hSerial);

	return 0;
}


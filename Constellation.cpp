#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <random>
#include <cmath>
#include <cstdint>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#if defined(_WIN32) || defined(WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#pragma comment(lib, "Dwmapi.lib")
#include <dwmapi.h>
#endif

#include "UI.hpp"
#include "SerialHandling.hpp"
#include "DataValues.hpp"
#include "ServerHandler.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glad/glad.h"

#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"


std::mutex valueLock;

UI* UI::ui = new UI();
SerialHandling* SerialHandling::serialhandling = new SerialHandling();
DataValues* DataValues::dataValues = new DataValues();

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

bool PrimeRocket()
{
	DataValues* data = DataValues::Get();
	if(data->isSRADConnected)
	{
		char dataToSend[9];
		uint32_t dataLength = (uint32_t)sizeof(dataToSend);
		char dataLengthArray[5];
		memcpy(&dataLengthArray, &dataLength, 4);

		std::string header = std::string("C_ST") + std::string(dataLengthArray);
		size_t bytesWrittenHeader;

		bytesWrittenHeader = data->hSerialSRAD->write(header);
		
		size_t bytesWrittenData;
		memcpy(dataToSend, &(data->fuse_delay), 4);
		memcpy(dataToSend + 4, &(data->launch_altitude), 4);

		bytesWrittenData = data->hSerialSRAD->write(std::string(dataToSend));
		return true;
	}	
	return false;
}

bool LaunchRocket()
{
	DataValues* data = DataValues::Get();
	if(data->isSRADConnected)
	{
		std::string dataToSend = "C_LR";
		size_t bytesWritten = data->hSerialSRAD->write(dataToSend);
		return bytesWritten == sizeof(dataToSend);
	}
	return false;
}

int main()
{
	//GUI INITIALIZATION
	UI* gui = UI::Get();
	SerialHandling* serialHandling = SerialHandling::Get();
	DataValues* data = DataValues::Get();

	ServerHandler serverHandler;

	std::thread webServerThread(&ServerHandler::Server, &serverHandler, &valueLock);
	webServerThread.detach();

	serialHandling->SetValueLock(&valueLock);

	//FInd correct serial locations
	std::string SRADSerialLoc = "";
	std::string TeleBtSerialLoc = "";
	serial::Serial* hSerialSRAD = nullptr;
	serial::Serial* hSerialTeleBT = nullptr;

	serialHandling->FindSerialLocations(&SRADSerialLoc, &TeleBtSerialLoc);

	//SERIAL INITIALIZATION
	if (SRADSerialLoc == "" || TeleBtSerialLoc == "") {
		std::cout << "Failed to find serial ports, aborting serial communication." << std::endl;
	} else {
		if (serialHandling->CreateSerialFile(hSerialSRAD, SRADSerialLoc))
		{
			data->prime_rocket = PrimeRocket;
			data->launch_rocket = LaunchRocket;

			data->hSerialSRAD = hSerialSRAD;

			std::thread serialThreadSRAD(&SerialHandling::ProcessSerialDataSRAD, serialHandling);

			serialThreadSRAD.detach();
		} 
		else
		{
			std::cout << "Failed to create SRAD serial communication, aborting." << std::endl;
		}

		if (serialHandling->CreateSerialFile(hSerialTeleBT, TeleBtSerialLoc))
		{
			std::thread serialThreadTeleBT(&SerialHandling::ProcessSerialDataTeleBT, serialHandling, hSerialTeleBT);

			serialThreadTeleBT.detach();
		} 
		else
		{
			std::cout << "Failed to create TeleBT serial communication, aborting." << std::endl;
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

	int width, height, channels;
	unsigned char* pixels = stbi_load("assets\\icon.png", &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels) {
		return 3;
	}

	GLFWimage icon_image;
	icon_image.width = width;
	icon_image.height = height;
	icon_image.pixels = pixels;
	glfwSetWindowIcon(window, 1, &icon_image);
	stbi_image_free(pixels);

	#ifdef _WIN32
		HWND hwnd = glfwGetWin32Window(window);
		COLORREF titleBarColor = 0x00000000;
		DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &titleBarColor, sizeof(titleBarColor));
		DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &titleBarColor, sizeof(titleBarColor));
	#endif

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw("Unable to context to OpenGL");

	int screen_width, screen_height;
	glfwGetFramebufferSize(window, &screen_width, &screen_height);
	glViewport(0, 0, screen_width, screen_height);

	gui->Init(window, glsl_version);

	while (!glfwWindowShouldClose(window)) {
		if (std::abs(difftime(data->last_ping, time(NULL))) > 5)
		{
			data->isSRADConnected = false;
		}
		
		data->go_grid_values[0][3] = data->isSRADConnected;

		auto start = std::chrono::high_resolution_clock::now();
		glfwPollEvents();
		gui->NewFrame();
		gui->Update();
		gui->Render();
	}

	gui->Shutdown();
	if (hSerialSRAD != nullptr) hSerialSRAD->close();
	if (hSerialTeleBT != nullptr) hSerialTeleBT->close();

	std::ofstream outputFile;
	outputFile.open("DATA.csv", std::ios::out); 
	if (outputFile.is_open()) {
		char time_string[11];
		if (data->launch_time != 0L)
		{
			std::tm* tm_time = std::localtime(&(data->launch_time));
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

		DataValues::DataValueList dataValueList = data->getDataValueList();
		
		WriteDataToFile(dataValueList.a_values, std::string("acceleration"), &outputFile);
		WriteDataToFile(dataValueList.v_values, std::string("velocity"), &outputFile);
		WriteDataToFile(dataValueList.a_values, std::string("time"), &outputFile);
		WriteDataToFile(dataValueList.x_values, std::string("positionX"), &outputFile);
		WriteDataToFile(dataValueList.y_values, std::string("positionY"), &outputFile);
		WriteDataToFile(dataValueList.z_values, std::string("positionZ"), &outputFile);
		WriteDataToFile(dataValueList.x_rot_values, std::string("rotationX"), &outputFile);
		WriteDataToFile(dataValueList.y_rot_values, std::string("rotationY"), &outputFile);
		WriteDataToFile(dataValueList.z_rot_values, std::string("rotationZ"), &outputFile);

		outputFile << "launchTime," << time_string << std::endl;
		outputFile << "launchDate," << date_string << std::endl;
		outputFile << "fuseDelay," << data->fuse_delay << std::endl;
		outputFile << "launchAltitude," << data->launch_altitude << std::endl;

        outputFile.close();
    }

	delete gui;

	return 0;
}
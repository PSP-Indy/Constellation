/**
 * @file Constellation.cpp
 * @brief This is the main entry point of the program.
 *
 * This file contains the main function and initializes all subsystems.
 */

#pragma once

#include <iostream>
#include <chrono>

#include "UI.hpp"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

using namespace std::chrono;

UI* UI::ui = new UI();

int main()
{
	UI* gui = UI::Get();

	std::vector<float> t_values = {0, 1, 2, 3};
	std::vector<float> v_values = {0, 10, 20, 30};
	std::vector<float> x_values = {0, 0, 0, 0};
	std::vector<float> y_values = {0, 0, 0, 0};
	std::vector<float> z_values = {0, 10, 30, 60};

	gui->assignValueGroups(gui, &t_values, &v_values, &x_values, &y_values, &z_values, 0.1f, 0.2f);

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
	
	return 0;
}

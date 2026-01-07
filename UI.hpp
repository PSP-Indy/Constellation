#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <sys/resource.h>
#include <unistd.h>
#endif

#include "imgui.h"
#include "implot.h"
#include "implot3d.h"
#include "imgui-knobs.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "DataValues.hpp"
#include "SerialHandling.hpp"

class UI {
public:
	UI();
	UI(const UI& obj) = delete;
	void Init(GLFWwindow* window, const char* glsl_version);
	void NewFrame();
	void Update();
	void Render();
	void Shutdown();
	void RotateModel(std::vector<ImPlot3DPoint> vertices, std::vector<ImPlot3DPoint>* rotated_vertices, float x_rotation, float y_rotation, float z_rotation);
	~UI();
	static UI* Get();

	const char go_grid_labels[5][5][5] = {
		{"C_TS", "BART", "c1", "d1", "e1"},
		{"C_FI", "IMUT", "c2", "d2", "e2"},
		{"C_FO", "AT_T", "c3", "d3", "e3"},
		{"C_SC", "AT_V", "c4", "d4", "e4"},
		{"GPS ", "b5", "c5", "d5", "e5"}
	};

private:
	GLFWmonitor* monitor;
	const GLFWvidmode* mode;
	GLFWwindow* window;

	void SetColorStyles();

	int FindClosestIndex(const std::vector<float>* values, int target)
	{
		float closestDist = FLT_MAX;
		int closestIndex = 0;
		for (size_t idx = 0; idx < values->size(); idx++)
		{
			float dist = std::fabs(values->at(idx) - target);
			if (dist < closestDist)
			{
				closestDist = dist;
				closestIndex = idx;
			}
		}

		return closestIndex;
	}

	std::vector<float> SubArray(const std::vector<float>* values, int start, int end)
	{
		std::vector<float> subArray;
		for(int i = 0; i < (end - start); i++) 
		{
			subArray.push_back(values->at(start + i));
		}
		return subArray;
	}

	static UI* ui;
};
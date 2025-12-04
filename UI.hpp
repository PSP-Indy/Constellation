#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include <windows.h>
#include <psapi.h>

#include "imgui.h"
#include "implot.h"
#include "implot3d.h"
#include "imgui-knobs.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "DataValues.hpp"

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
	void SetColorStyles();
	~UI();
	static UI* Get();

	void AssignValues(DataValueHandler::DataValues* data_values)
	{
		this->rocket_data = data_values;
	}

private:
	GLFWmonitor* monitor;
	const GLFWvidmode* mode;
	GLFWwindow* window;

	char go_grid_labels[5][5][5] = {
		{"C_TS", "BART", "c1", "d1", "e1"},
		{"C_FI", "IMUT", "c2", "d2", "e2"},
		{"C_FO", "AT_T", "c3", "d3", "e3"},
		{"C_SC", "AT_V", "c4", "d4", "e4"},
		{"a5", "b5", "c5", "d5", "e5"}
	};

	int FindClosestIndex(const std::vector<float>* values, int target)
	{
		float closest_dist = FLT_MAX;
		int closest_index = 0;
		for (size_t idx = 0; idx < values->size(); idx++)
		{
			float dist = std::fabs(values->at(idx) - target);
			if (dist < closest_dist)
			{
				closest_dist = dist;
				closest_index = idx;
			}
		}

		return closest_index;
	}

	std::vector<float> SubArray(const std::vector<float>* values, int start, int end)
	{
		std::vector<float> sub_array;
		for(int i = 0; i < (end-start); i++) 
		{
			sub_array.push_back(values->at(start + i));
		}
		return sub_array;
	}

	DataValueHandler::DataValues* rocket_data;

	static UI* ui;
};
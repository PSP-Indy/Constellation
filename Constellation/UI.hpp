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

class UI {
public:
	struct data_values {
		std::vector<float> v_values = {0};
		std::vector<float> a_values = {0};
		std::vector<float> t_values = {0};
		std::vector<float> x_values = {0};
		std::vector<float> y_values = {0};
		std::vector<float> z_values = {0};

		std::vector<float> x_rot_values = {0};
		std::vector<float> y_rot_values = {0};
	};

	UI();
	UI(const UI& obj) = delete;
	void Init(GLFWwindow* window, const char* glsl_version);
	void NewFrame();
	void Update();
	void Render(GLFWwindow* window);
	void Shutdown();
	void RotateModel(std::vector<ImPlot3DPoint> vertices, std::vector<ImPlot3DPoint>* rotated_vertices, float x_rotation, float y_rotation);
	~UI();
	static UI* Get();

	void AssignValues(data_values* data_values){
		this->rocket_data = data_values;
	}

	int FindClosestIndex(std::vector<float> values, float target)
	{
		float closest_dist = FLT_MAX;
		int closest_index = 0;
		for (int idx = 0; idx < values.size(); idx++)
		{
			float dist = abs(values.at(idx) - target);
			if (dist < closest_dist)
			{
				closest_dist = dist;
				closest_index = idx;
			}
		}

		return closest_index;
	}

	std::vector<float> SubArray(std::vector<float> values, int start, int end)
	{
		std::vector<float> sub_array;
		for(int i = 0; i < (end-start); i++) 
		{
			sub_array.push_back(values.at(start + i));
		}
		return sub_array;
	}

private:
	data_values* rocket_data;

	static UI* ui;
};
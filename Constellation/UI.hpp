#pragma once

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include "imgui.h"
#include "implot.h"
#include "implot3d.h"
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

		float x_rotation = 0;
		float y_rotation = 0;
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

	void assignValues(data_values* data_values){
		this->rocket_data = data_values;
	}

private:
	data_values* rocket_data;

	static UI* ui;
};
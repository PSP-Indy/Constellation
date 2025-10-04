#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <time.h>
#include <tuple>
#include <thread>
#include <map>

#include "imgui.h"
#include "implot.h"
#include "implot3d.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

class UI {
private:
	std::vector<float>* v_values;
	std::vector<float>* t_values;
	std::vector<float>* x_values;
	std::vector<float>* y_values;
	std::vector<float>* z_values;

	float x_rotation;
	float y_rotation;

	static UI* ui;

public:
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

	void assignValues(UI* ui, std::vector<float>* t_values, std::vector<float>* v_values, std::vector<float>* x_values, std::vector<float>* y_values, std::vector<float>* z_values, float* x_rotation, float* y_rotation){
		ui->t_values = t_values;
		ui->v_values = v_values;
		ui->x_values = x_values;
		ui->y_values = y_values;
		ui->z_values = z_values;
		ui->x_rotation = *x_rotation;
		ui->y_rotation = *y_rotation;
	}
};
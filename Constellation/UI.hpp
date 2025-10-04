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

	static UI* ui;

public:
	UI();
	UI(const UI& obj) = delete;
	void Init(GLFWwindow* window, const char* glsl_version);
	void NewFrame();
	void Update();
	void Render(GLFWwindow* window);
	void Shutdown();
	~UI();
	static UI* Get();

	void assignValueGroups(UI* ui, std::vector<float>* t_values, std::vector<float>* v_values, std::vector<float>* x_values, std::vector<float>* y_values, std::vector<float>* z_values){
		ui->t_values = t_values;
		ui->v_values = v_values;
		ui->x_values = x_values;
		ui->y_values = y_values;
		ui->z_values = z_values;
	}
};
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
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

/**
 * @class UI
 * @brief Handles the user interface of Constellation
 */
class UI {
private:
	int cameraWidth, cameraHeight = 0;
	int floatId = 0;
	int stillNum = 1;
	time_t start = time(0);
	GLuint cameraTexture;
	bool pauseCamera = false;
	bool mainCamera = true;
	bool statisticsOpen = false;
	bool stillsOpen = false;
	bool stillsTexturesLoaded = false;
	int selectedController = 0;
	int quality = 80;
	float deltaVals[64];
	float frameVals[64];
	float depthVals[128];
	float mainDeltaTime;

	std::vector<GLuint> stillsTextures;
	std::vector<std::string> output;
	std::map<std::string, std::string> telemetry;

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
};
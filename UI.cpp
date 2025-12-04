#include "UI.hpp"

#include "rocket_model.hpp"

UI::UI()
{
}

float scale_factor = 1.0f;

bool position_plot = true;
bool velocity_plot = false;
bool acceleration_plot = false;
bool rotation_plot = false;

bool auto_scale_slice_plot_X = true;
bool auto_scale_slice_plot_Y = true;

bool isFullscreen = false;

int time_start = 0;
int time_end = 1;

float apogee = -FLT_MAX;
float fastest_speed = -FLT_MAX;
float fastest_aceleration = -FLT_MAX;

bool diagnostics_open = false;

static int window_x, window_y, window_w, window_h;

static const char* rebuild_ui_config = NULL;

char date_string[11];

void UI::Init(GLFWwindow *window, const char *glsl_version)
{
	time_t current_time = time(NULL);
	std::tm *localTime = std::localtime(&current_time);
	std::strftime(date_string, sizeof(date_string), "%m/%d/%Y", localTime);

	this->window = window;
	monitor = glfwGetPrimaryMonitor();
	mode = glfwGetVideoMode(monitor);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImPlot3D::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(scale_factor);
	
	SetColorStyles();

	io.IniFilename = "Assets/imgui.ini";

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	ImFont* largeFont = io.Fonts->AddFontFromFileTTF("Assets/DuruSans-Regular.ttf", 16);
	style.ScaleAllSizes(main_scale);
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

	style.WindowRounding = 0.0f;
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	style.Colors[ImGuiCol_ChildBg].w = 1.0f;
	style.Colors[ImGuiCol_PopupBg].w = 1.0f;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);


	const ImVec4 colors[] = {ImVec4{219.0f / 256.0f, 66.0f / 256.0f, 66.0f / 256.0f, 1.0f}, ImVec4{219.0f / 256.0f, 140.0f / 256.0f, 66.0f / 256.0f, 1.0f}, ImVec4{104.0f / 256.0f, 173.0f / 256.0f, 92.0f / 256.0f, 1.0f}};
	const ImVec4* color_ptr = colors;
	ImPlot::AddColormap("go_grid_colors", color_ptr, 3);

	for (int i = 0; i < 5; i++) 
	{
		for (int j = 0; j < 5; j++)
		{
			rocket_data->go_grid_values[i][j] = 0;
		}
	}
}

void UI::NewFrame()
{
	if (rebuild_ui_config != NULL) 
	{
		ImGui::LoadIniSettingsFromDisk(rebuild_ui_config);
        rebuild_ui_config = NULL;
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void UI::Update()
{
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

	DataValueHandler::DataValueList dataValueList = rocket_data->getDataValueList();

	#pragma region MenuBar
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("Application Controls"))
	{
		if (ImGui::MenuItem("Application Diagnostics"))
		{
			diagnostics_open = !diagnostics_open;
		}
		if (ImGui::MenuItem("Fullscreen (Slightly buggy, expect weird things to happen)"))
		{
			isFullscreen = !isFullscreen;
			if(isFullscreen)
			{
				glfwGetWindowPos(window, &window_x, &window_y);
				glfwGetWindowSize(window, &window_w, &window_h);

				glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
				glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);
				glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);
				glfwSetWindowSize(window, mode->width, mode->height + 1);
				glfwSetWindowPos(window, 0, 0);
			}
			else
			{
				glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
				glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_TRUE);
				glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_FALSE);

				glfwSetWindowSize(window, window_w, window_h);
				glfwSetWindowPos(window, window_x, window_y);
				glfwMakeContextCurrent(window);
			}
		}
		if (ImGui::MenuItem("Exit"))
		{
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("UI Configuraitions")) {
		if (ImGui::MenuItem("Default Configuration"))
		{
			rebuild_ui_config = "Assets/default.ini";
		}
		if (ImGui::MenuItem("Pre Flight Configuration"))
		{
			rebuild_ui_config = "Assets/pre_flight.ini";
		}
		if (ImGui::MenuItem("Flight Configuration"))
		{
			rebuild_ui_config = "Assets/active_flight.ini";
		}
		if (ImGui::MenuItem("Post Flight Configuration"))
		{
			rebuild_ui_config = "Assets/post_flight.ini";
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
	#pragma endregion

	#pragma region Diagnostics
	if (diagnostics_open)
	{
		std::string working_set_size_str = "ERROR";
		std::string private_bytes_str = "ERROR";

		PROCESS_MEMORY_COUNTERS_EX pmc;
		if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
		{
			working_set_size_str = std::string("Working Set Size: ").append(std::to_string(pmc.WorkingSetSize / (1024 * 1024))).append(std::string(" MB"));
			private_bytes_str = std::string("Private Bytes: ").append(std::to_string(pmc.PrivateUsage / (1024 * 1024))).append(std::string(" MB"));
		}

		ImGui::Begin("Diagnostics");
		ImGui::Text(working_set_size_str.c_str());
		ImGui::Text(private_bytes_str.c_str());
		ImGui::End();
	}
	#pragma endregion

	#pragma region VelocityPlot
	ImGui::Begin("Velocity");
	if (ImPlot::BeginPlot("Velocity (ft/s) vs Time (s)", ImVec2(-1, -1)))
	{
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
		ImPlot::SetupAxis(ImAxis_Y1, "Velocity (ft/s)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("", dataValueList.t_values.data(), dataValueList.v_values.data(), static_cast<int>(dataValueList.t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region AccelerationPlot
	ImGui::Begin("Acceleration");
	if (ImPlot::BeginPlot("Acceleration (ft/s/s) vs Time (s)", ImVec2(-1, -1)))
	{
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
		ImPlot::SetupAxis(ImAxis_Y1, "Acceleration (ft/s/s)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("Acceleration", dataValueList.t_values.data(), dataValueList.a_values.data(), static_cast<int>(dataValueList.t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region PositionPlot
	ImGui::Begin("Position");
	if (ImPlot::BeginPlot("Position (ft) vs Time (s)", ImVec2(-1, -1)))
	{
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
		ImPlot::SetupAxis(ImAxis_Y1, "Position (ft)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("X Positition", dataValueList.t_values.data(), dataValueList.x_values.data(), static_cast<int>(dataValueList.t_values.size()));
		ImPlot::PlotLine("Y Positition", dataValueList.t_values.data(), dataValueList.y_values.data(), static_cast<int>(dataValueList.t_values.size()));
		ImPlot::PlotLine("Z Positition", dataValueList.t_values.data(), dataValueList.z_values.data(), static_cast<int>(dataValueList.t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region RotationPlot
	ImGui::Begin("Rotation");
	if (ImPlot::BeginPlot("Rotation (Rad) vs Time (s)", ImVec2(-1, -1)))
	{
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
		ImPlot::SetupAxis(ImAxis_Y1, "Rotation (Rad)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("X Rotation", dataValueList.t_values.data(), dataValueList.x_rot_values.data(), static_cast<int>(dataValueList.t_values.size()));
		ImPlot::PlotLine("Y Rotation", dataValueList.t_values.data(), dataValueList.y_rot_values.data(), static_cast<int>(dataValueList.t_values.size()));
		ImPlot::PlotLine("Z Rotation", dataValueList.t_values.data(), dataValueList.z_rot_values.data(), static_cast<int>(dataValueList.t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region TimeSlicedPlot
	ImGui::Begin("Time Sliced Plot");

	ImGui::Checkbox("Position", &position_plot);
	ImGui::SameLine();
	ImGui::Checkbox("Velocity", &velocity_plot);
	ImGui::SameLine();
	ImGui::Checkbox("Acceleration", &acceleration_plot);
	ImGui::SameLine();
	ImGui::Checkbox("Rotation", &rotation_plot);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::SliderInt("Time Start", &time_start, 0.0f, time_end - 0.01f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::SliderInt("Time End", &time_end, time_start + 0.01f, dataValueList.t_values.back());

	ImGui::Checkbox("Auto Scale Time Axis", &auto_scale_slice_plot_X);
	ImGui::SameLine();
	ImGui::Checkbox("Auto Scale Value Axis", &auto_scale_slice_plot_Y);

	int start_index = FindClosestIndex(&(dataValueList.t_values), time_start);
	int end_index = FindClosestIndex(&(dataValueList.t_values), time_end);

	if (end_index <= start_index)
	{
		end_index = start_index + 1;
	}
	if (start_index >= end_index)
	{
		start_index = end_index - 1;
	}

	if (ImPlot::BeginPlot("Time Sliced Plot", ImVec2(-1, -1)))
	{
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
		ImPlot::SetupAxis(ImAxis_Y1, "Value");

		if (auto_scale_slice_plot_X)
		{
			ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit);
		}
		if (auto_scale_slice_plot_Y)
		{
			ImPlot::SetupAxis(ImAxis_Y1, "Value", ImPlotAxisFlags_AutoFit);
		}

		std::vector<float> t_values_clipped = SubArray(&(dataValueList.t_values), start_index, end_index);

		if (velocity_plot)
		{
			ImPlot::PlotLine("Velocity", t_values_clipped.data(), SubArray(&(dataValueList.v_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size()));
		}
		if (acceleration_plot)
		{
			ImPlot::PlotLine("Acceleration", t_values_clipped.data(), SubArray(&(dataValueList.a_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size()));
		}
		if (position_plot)
		{
			ImPlot::PlotLine("X Postion", t_values_clipped.data(), SubArray(&(dataValueList.x_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size()));
		}
		if (position_plot)
		{
			ImPlot::PlotLine("Y Position", t_values_clipped.data(), SubArray(&(dataValueList.y_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size()));
		}
		if (position_plot)
		{
			ImPlot::PlotLine("Z Position", t_values_clipped.data(), SubArray(&(dataValueList.z_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size()));
		}
		if (rotation_plot)
		{
			ImPlot::PlotLine("X Rotation", t_values_clipped.data(), SubArray(&(dataValueList.x_rot_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size()));
		}
		if (rotation_plot)
		{
			ImPlot::PlotLine("Y Rotation", t_values_clipped.data(), SubArray(&(dataValueList.y_rot_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size()));
		}
		if (rotation_plot)
		{
			ImPlot::PlotLine("Y Rotation", t_values_clipped.data(), SubArray(&(dataValueList.z_rot_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size()));
		}

		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region 3DRotationVisualizer
	ImGui::Begin("3D Rotation");
	std::vector<ImPlot3DPoint> rocket_vertices_rotated;
	RotateModel(rocket_vertices, &rocket_vertices_rotated, 3.14f / 2.0f + dataValueList.x_rot_values.back(), dataValueList.y_rot_values.back(), dataValueList.z_rot_values.back());
	
	int x_region_avail = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x);
	int y_region_avail = (ImGui::GetContentRegionAvail().y - 230 - ImGui::GetStyle().ItemSpacing.y);
	int smallest_region = x_region_avail < y_region_avail ? x_region_avail : y_region_avail;

	if (ImPlot3D::BeginPlot("3D Rotation", ImVec2(smallest_region, smallest_region))) {
		ImPlot3D::SetupAxisLimits(ImAxis3D_X, 0, 1);
		ImPlot3D::SetupAxisLimits(ImAxis3D_Y, 0, 1);
		ImPlot3D::SetupAxisLimits(ImAxis3D_Z, 0, 1);
		ImPlot3D::SetupAxes("", "", "", ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax, ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax, ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax);
		ImPlot3D::PlotMesh("Orientation", rocket_vertices_rotated.data(), rocket_indices.data(), static_cast<int>(rocket_vertices_rotated.size()), static_cast<int>(rocket_indices.size()), ImPlot3DMeshFlags_NoLegend);
		ImPlot3D::EndPlot();
	}

	if (ImGui::BeginTable("Rotation Rotary Dial Table", 3, ImGuiTableFlags_None, ImVec2(-1, -1))) 
	{
		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Rotation X", &(dataValueList.x_rot_values.back()), -3.14159, 3.14159, 0.1f, "%.1f rads", ImGuiKnobVariant_WiperOnly);

		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Rotation Y", &(dataValueList.y_rot_values.back()), -3.14159, 3.14159, 0.1f, "%.1f rads", ImGuiKnobVariant_WiperOnly);

		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Rotation Z", &(dataValueList.z_rot_values.back()), -3.14159, 3.14159, 0.1f, "%.1f rads", ImGuiKnobVariant_WiperOnly);

		ImGui::EndTable();
	}
	ImGui::End();
	#pragma endregion

	#pragma region FlightData
	ImGui::Begin("Flight Data");

	if (ImGui::BeginTable("Flight Data Rotary Dial Table", 3, ImGuiTableFlags_None, ImVec2(-1, 0))) 
	{
		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Speed", &(dataValueList.v_values.back()), -100.0f, 100.0f, 0.1f, "%.1f", ImGuiKnobVariant_WiperOnly);

		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Acceleration", &(dataValueList.a_values.back()), -9.8f * 2, 9.8f * 2, 0.1f, "%.1f", ImGuiKnobVariant_WiperOnly);

		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Altitude", &(dataValueList.z_values.back()), 0.0f, 10000.0f, 0.1f, "%.1f", ImGuiKnobVariant_WiperOnly);

		ImGui::EndTable();
	}

	if (dataValueList.z_values.at(dataValueList.z_values.size() - 1) > apogee)
		apogee = dataValueList.z_values.at(dataValueList.z_values.size() - 1);
	ImGui::InputFloat("Apogee", &apogee, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

	if (dataValueList.v_values.at(dataValueList.v_values.size() - 1) > fastest_speed)
		fastest_speed = dataValueList.v_values.at(dataValueList.v_values.size() - 1);
	ImGui::InputFloat("Max Speed", &fastest_speed, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

	if (dataValueList.a_values.at(dataValueList.a_values.size() - 1) > fastest_aceleration)
		fastest_aceleration = dataValueList.a_values.at(dataValueList.a_values.size() - 1);
	ImGui::InputFloat("Max Acceleration", &fastest_aceleration, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

	ImGui::InputText("Date", date_string, 11, ImGuiInputTextFlags_ReadOnly);

	char time_string_input[11];
	if (rocket_data->launch_time != NULL)
	{
		std::tm *tm_time = std::localtime(&rocket_data->launch_time);
		std::strftime(time_string_input, sizeof(time_string_input), "%H:%M:%S", tm_time);
	}
	else
	{
		strncpy(time_string_input, "NO LAUNCH", 11);
	}
	ImGui::InputText("Launch Time", time_string_input, 11, ImGuiInputTextFlags_ReadOnly);

	char time_since_launch_string_input[11];
	if (rocket_data->launch_time != NULL)
	{
		std::time_t currentTime_t = time(NULL);
		int difference_in_seconds = difftime(currentTime_t, rocket_data->launch_time);
		strncpy(time_since_launch_string_input, std::to_string(difference_in_seconds).c_str(), 11);
	}
	else
	{
		strncpy(time_since_launch_string_input, "NO LAUNCH", 11);
	}
	ImGui::InputText("Seconds Since Launch", time_since_launch_string_input, 11, ImGuiInputTextFlags_ReadOnly);

	ImGui::End();
	#pragma endregion

	#pragma region LaunchManager
	ImGui::Begin("Launch Manager");

	x_region_avail = (ImGui::GetContentRegionAvail().x - 70 - ImGui::GetStyle().ItemSpacing.x);
	y_region_avail = (ImGui::GetContentRegionAvail().y - 70 - ImGui::GetStyle().ItemSpacing.y);
	smallest_region = x_region_avail < y_region_avail ? x_region_avail : y_region_avail;

	if (ImPlot::BeginPlot("Go Grid", ImVec2(smallest_region, smallest_region), ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText))
	{
		
		ImPlot::PushColormap(ImPlot::GetColormapIndex("go_grid_colors"));
		static float scale_min = 0.0f;
		static float scale_max = 1.0f;
		static ImPlotAxisFlags axes_flags = ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels;
		ImPlot::SetupAxes(nullptr, nullptr, axes_flags, axes_flags);
		ImPlot::PlotHeatmap("Heatmap", rocket_data->go_grid_values[0], 5, 5, scale_min, scale_max, nullptr, ImPlotPoint(0,0), ImPlotPoint(1,1));
		
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				ImPlot::PlotText(go_grid_labels[i][4 - j], (i / 5.0f) + (0.5f / 5.0f), (j / 5.0f) + (0.5f / 5.0f));
			}
		}

		ImPlot::EndPlot();

		ImGui::SameLine();
		ImPlot::ColormapScale("Heatmap Scale", scale_min, scale_max, ImVec2(-1, smallest_region));

		ImPlot::PopColormap();
	}

	if(rocket_data->coundown_start_time != NULL) 
	{
		time_t current_time_t = time(NULL);
		int time_since_countdown = difftime(rocket_data->coundown_start_time, current_time_t);
		int countdown = 5 - std::abs(time_since_countdown);

		ImFont* largeFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("Assets/DuruSans-Regular.ttf", 64);
		ImGui::PushFont(largeFont);
		ImGui::Text((std::string("LAUNCH: T-") + std::to_string(countdown)).c_str());
		ImGui::PopFont();
	}
	else
	{
		if (rocket_data->go_grid_values[0][0] != 1)
		{
			ImGui::SetNextItemWidth(300);
			ImGui::InputInt("Fuse Delay (s)", &(rocket_data->fuse_delay));

			if (rocket_data->fuse_delay < 0)
			{
				rocket_data->fuse_delay = 0;
			}

			ImGui::SetNextItemWidth(300);
			ImGui::InputInt("Launch Alitude (m above sea level)", &(rocket_data->launch_altitude));
			
			if (ImGui::Button("Prime Rocket", ImVec2(-1, 70)))
			{
				if (rocket_data->prime_rocket != NULL)
				{
					rocket_data->prime_rocket(rocket_data->hSerialSRAD, rocket_data);
				}
			}
		} 
		else if (rocket_data->coundown_start_time == NULL && rocket_data->launch_time == NULL)
		{
			if (ImGui::Button("Launch Rocket", ImVec2(-1, 70)))
			{
				if (rocket_data->launch_rocket != NULL)
				{
					rocket_data->launch_rocket(rocket_data->hSerialSRAD, rocket_data);
				}
			}
		}
	}

	ImGui::End();
	#pragma endregion
}

void UI::Render()
{
	ImGui::Render();
	glClearColor(1.00f, 1.00f, 1.00f, 1.00f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
	glfwMakeContextCurrent(window);
	glfwSwapBuffers(window);
}

void UI::Shutdown()
{
	ImGui::DestroyPlatformWindows();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot3D::DestroyContext();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

#include <vector>
#include <cmath>
#include "implot3d.h" // assuming ImPlot3DPoint is {float x,y,z;}

void UI::RotateModel(const std::vector<ImPlot3DPoint> vertices,
					 std::vector<ImPlot3DPoint> *rotated_vertices,
					 float x_rotation, float y_rotation, float z_rotation)
{
	rotated_vertices->clear();

	// Precompute trig values
	float cx = cosf(x_rotation), sx = sinf(x_rotation);
	float cy = cosf(y_rotation), sy = sinf(y_rotation);
	float cz = cosf(z_rotation), sz = sinf(z_rotation);

	// Combined rotation matrix R = Rz * Ry * Rx (right-handed)
	float R[3][3];

	R[0][0] = cz * cy;
	R[0][1] = cz * sy * sx - sz * cx;
	R[0][2] = cz * sy * cx + sz * sx;

	R[1][0] = sz * cy;
	R[1][1] = sz * sy * sx + cz * cx;
	R[1][2] = sz * sy * cx - cz * sx;

	R[2][0] = -sy;
	R[2][1] = cy * sx;
	R[2][2] = cy * cx;

	// Apply rotation to each vertex
	for (const auto &vert : vertices)
	{
		// Center around (0.5, 0.5, 0.5)
		float x = vert.x - 0.5f;
		float y = vert.y - 0.5f;
		float z = vert.z - 0.5f;

		// Apply rotation matrix
		ImPlot3DPoint r;
		r.x = R[0][0] * x + R[0][1] * y + R[0][2] * z;
		r.y = R[1][0] * x + R[1][1] * y + R[1][2] * z;
		r.z = R[2][0] * x + R[2][1] * y + R[2][2] * z;

		// Translate back
		r.x += 0.5f;
		r.y += 0.5f;
		r.z += 0.5f;

		rotated_vertices->push_back(r);
	}
}

void UI::SetColorStyles()
{
	ImGuiStyle& style = ImGui::GetStyle();

	ImVec4 PSP_Black = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	ImVec4 PSP_NightSky = ImVec4(37.0f / 255.0f, 37 / 255.0f, 38 / 255.0f, 1.00f);
	ImVec4 PSP_Rush = ImVec4(218.0f / 255.0f, 170 / 255.0f, 0.0f, 1.0f);
	ImVec4 PSP_Rush_Dark = ImVec4(218.0f / 255.0f / 2.0f, 170 / 255.0f/ 2.0f, 0.0f, 1.0f);
	ImVec4 PSP_Rush_Medium = ImVec4(218.0f / 255.0f * 0.75f, 170 / 255.0f * 0.75f, 0.0f, 1.0f);
	ImVec4 PSP_Moondust = ImVec4(242.0f / 255.0f, 239.0f / 255.0f, 233.0f / 255.0f, 1.0f);
	ImVec4 PSP_White = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	style.Colors[ImGuiCol_Text]                   = PSP_Moondust;
    style.Colors[ImGuiCol_TextDisabled]           = PSP_Moondust;
    style.Colors[ImGuiCol_WindowBg]               = PSP_Black;
    style.Colors[ImGuiCol_ChildBg]                = PSP_NightSky;
    style.Colors[ImGuiCol_PopupBg]                = PSP_NightSky;
    style.Colors[ImGuiCol_Border]                 = PSP_Rush_Dark;
    style.Colors[ImGuiCol_BorderShadow]           = PSP_NightSky;
    style.Colors[ImGuiCol_FrameBg]                = PSP_NightSky;
    style.Colors[ImGuiCol_FrameBgHovered]         = PSP_NightSky;
    style.Colors[ImGuiCol_FrameBgActive]          = PSP_NightSky;
    style.Colors[ImGuiCol_TitleBg]                = PSP_NightSky;
    style.Colors[ImGuiCol_TitleBgActive]          = PSP_NightSky;
    style.Colors[ImGuiCol_TitleBgCollapsed]       = PSP_NightSky;
    style.Colors[ImGuiCol_MenuBarBg]              = PSP_Black;
    style.Colors[ImGuiCol_ScrollbarBg]            = PSP_Black;
    style.Colors[ImGuiCol_ScrollbarGrab]          = PSP_NightSky;
    style.Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]              = PSP_Rush;
    style.Colors[ImGuiCol_SliderGrab]             = PSP_Rush_Medium;
    style.Colors[ImGuiCol_SliderGrabActive]       = PSP_Rush;
    style.Colors[ImGuiCol_Button]                 = PSP_NightSky;
    style.Colors[ImGuiCol_ButtonHovered]          = PSP_Rush_Medium;
    style.Colors[ImGuiCol_ButtonActive]           = PSP_Rush;
    style.Colors[ImGuiCol_Header]                 = PSP_Rush_Dark;
    style.Colors[ImGuiCol_HeaderHovered]          = PSP_Rush_Medium;
    style.Colors[ImGuiCol_HeaderActive]           = PSP_Rush;
    style.Colors[ImGuiCol_Separator]              = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_SeparatorHovered]       = PSP_Rush_Medium;
    style.Colors[ImGuiCol_SeparatorActive]        = PSP_Rush;
    style.Colors[ImGuiCol_ResizeGrip]             = PSP_Rush_Dark;
    style.Colors[ImGuiCol_ResizeGripHovered]      = PSP_Rush_Medium;
    style.Colors[ImGuiCol_ResizeGripActive]       = PSP_Rush;
    style.Colors[ImGuiCol_InputTextCursor]        = style.Colors[ImGuiCol_Text];
    style.Colors[ImGuiCol_TabHovered]             = style.Colors[ImGuiCol_HeaderHovered];
    style.Colors[ImGuiCol_Tab]                    = style.Colors[ImGuiCol_Header];
    style.Colors[ImGuiCol_TabSelected]            = style.Colors[ImGuiCol_HeaderActive];
    style.Colors[ImGuiCol_TabSelectedOverline]    = style.Colors[ImGuiCol_HeaderActive];
    style.Colors[ImGuiCol_TabDimmed]              = style.Colors[ImGuiCol_Tab];
    style.Colors[ImGuiCol_TabDimmedSelected]      = style.Colors[ImGuiCol_TabSelected];
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    style.Colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    style.Colors[ImGuiCol_TextLink]               = style.Colors[ImGuiCol_HeaderActive];
    style.Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_TreeLines]              = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_NavCursor]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

UI::~UI()
{
}

UI *UI::Get()
{
	if (ui == nullptr)
		ui = new UI();
	return ui;
}
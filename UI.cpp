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

int time_start = 0;
int time_end = 1;

float apogee = -FLT_MAX;
float fastest_speed = -FLT_MAX;
float fastest_aceleration = -FLT_MAX;

bool diagnostics_open = false;

std::string date_string = "";

void UI::Init(GLFWwindow* window, const char* glsl_version)
{
	auto now = std::chrono::system_clock::now();
    std::time_t currentTime_t = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime_t);
    std::stringstream ss;
    ss << std::put_time(localTime, "%Y-%m-%d");
	date_string = ss.str();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImPlot3D::CreateContext();

	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	ImFont* largeFont = io.Fonts->AddFontFromFileTTF("DuruSans-Regular.ttf", 16);
	style.ScaleAllSizes(main_scale);
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;

	style.WindowRounding = 0.0f;
	style.Colors[ImGuiCol_WindowBg].w = 1.0f;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImGui::StyleColorsDark();

	const ImVec4 colors[] = {ImVec4{219.0f / 256.0f, 66.0f / 256.0f, 66.0f / 256.0f, 1.0f}, ImVec4{219.0f / 256.0f, 140.0f / 256.0f, 66.0f / 256.0f, 1.0f}, ImVec4{104.0f / 256.0f, 173.0f / 256.0f, 92.0f / 256.0f, 1.0f}};
	const ImVec4* color_ptr = colors;
	ImPlot::AddColormap("go_grid_colors", color_ptr, 3);
}


void UI::NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void UI::Update()
{
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

	ImGui::BeginMainMenuBar();
	if (ImGui::MenuItem("Application Diagnostics")) {
		diagnostics_open = !diagnostics_open;
	}
	ImGui::EndMainMenuBar();

	#pragma region Diagnostics
	if (diagnostics_open)
	{
		std::string working_set_size_str = "ERROR";
		std::string private_bytes_str = "ERROR";

		PROCESS_MEMORY_COUNTERS_EX pmc;
		if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
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
	if (ImPlot::BeginPlot("Velocity (ft/s) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); 
        ImPlot::SetupAxis(ImAxis_Y1, "Velocity (ft/s)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("", rocket_data->t_values.data(),  rocket_data->v_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region AccelerationPlot
	ImGui::Begin("Acceleration");
	if (ImPlot::BeginPlot("Acceleration (ft/s/s) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); 
        ImPlot::SetupAxis(ImAxis_Y1, "Acceleration (ft/s/s)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("", rocket_data->t_values.data(),  rocket_data->a_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region PositionPlot
	ImGui::Begin("Position");
	if (ImPlot::BeginPlot("Position (ft) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); 
        ImPlot::SetupAxis(ImAxis_Y1, "Position (ft)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("X Positition", rocket_data->t_values.data(),  rocket_data->x_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::PlotLine("Y Positition", rocket_data->t_values.data(),  rocket_data->y_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::PlotLine("Z Positition", rocket_data->t_values.data(),  rocket_data->z_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region RotationPlot
	ImGui::Begin("Rotation");
	if (ImPlot::BeginPlot("Rotation (Rad) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); 
        ImPlot::SetupAxis(ImAxis_Y1, "Rotation (Rad)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("X Rotation", rocket_data->t_values.data(),  rocket_data->x_rot_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::PlotLine("Y Rotation", rocket_data->t_values.data(),  rocket_data->y_rot_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::PlotLine("Z Rotation", rocket_data->t_values.data(),  rocket_data->z_rot_values.data(), static_cast<int>(rocket_data->t_values.size()));
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
	ImGui::SliderInt("Time End", &time_end, time_start + 0.01f, rocket_data->t_values.back());

	ImGui::Checkbox("Auto Scale Time Axis", &auto_scale_slice_plot_X);
	ImGui::SameLine();
	ImGui::Checkbox("Auto Scale Value Axis", &auto_scale_slice_plot_Y);

	int start_index = FindClosestIndex(&(rocket_data->t_values), time_start);
	int end_index = FindClosestIndex(&(rocket_data->t_values), time_end);

	if (end_index <= start_index) { end_index = start_index + 1; }
	if (start_index >= end_index) { start_index = end_index - 1; }

	if (ImPlot::BeginPlot("Time Sliced Plot", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)"); 
        ImPlot::SetupAxis(ImAxis_Y1, "Value");

		if (auto_scale_slice_plot_X) { ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); }
		if (auto_scale_slice_plot_Y) { ImPlot::SetupAxis(ImAxis_Y1, "Value", ImPlotAxisFlags_AutoFit); }

		std::vector<float> t_values_clipped = SubArray(&(rocket_data->t_values), start_index, end_index);

		if (velocity_plot) { ImPlot::PlotLine("Velocity", t_values_clipped.data(), SubArray(&(rocket_data->v_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size())); }
		if (acceleration_plot) { ImPlot::PlotLine("Acceleration", t_values_clipped.data(), SubArray(&(rocket_data->a_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size())); }
		if (position_plot) { ImPlot::PlotLine("X Postion", t_values_clipped.data(), SubArray(&(rocket_data->x_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size())); }
		if (position_plot) { ImPlot::PlotLine("Y Position", t_values_clipped.data(), SubArray(&(rocket_data->y_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size())); }
		if (position_plot) { ImPlot::PlotLine("Z Position", t_values_clipped.data(), SubArray(&(rocket_data->z_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size())); }
		if (rotation_plot) { ImPlot::PlotLine("X Rotation", t_values_clipped.data(), SubArray(&(rocket_data->x_rot_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size())); }
		if (rotation_plot) { ImPlot::PlotLine("Y Rotation", t_values_clipped.data(), SubArray(&(rocket_data->y_rot_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size())); }
		if (rotation_plot) { ImPlot::PlotLine("Y Rotation", t_values_clipped.data(), SubArray(&(rocket_data->z_rot_values), start_index, end_index).data(), static_cast<int>(t_values_clipped.size())); }
		
		ImPlot::EndPlot();
	}
	ImGui::End();
	#pragma endregion

	#pragma region 3DRotationVisualizer
	std::vector<ImPlot3DPoint> rocket_vertices_rotated;
	RotateModel(rocket_vertices, &rocket_vertices_rotated, 3.14f / 2.0f + rocket_data->x_rot_values.back(), rocket_data->y_rot_values.back());
	
	ImGui::Begin("3D Rotation Visualizer");

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

	if (ImGui::BeginTable("Rotation Rotary Dial Table", 3, ImGuiTableFlags_None, ImVec2(-1, 0))) 
	{
		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Rotation X", &(rocket_data->x_rot_values.back()), -3.14159, 3.14159, 0.1f, "%.1f rads", ImGuiKnobVariant_WiperOnly);

		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Rotation Y", &(rocket_data->y_rot_values.back()), -3.14159, 3.14159, 0.1f, "%.1f rads", ImGuiKnobVariant_WiperOnly);

		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Rotation Z", &(rocket_data->z_rot_values.back()), -3.14159, 3.14159, 0.1f, "%.1f rads", ImGuiKnobVariant_WiperOnly);

		ImGui::EndTable();
	}
	ImGui::End();
	#pragma endregion

	#pragma region FlightData
	ImGui::Begin("Flight Data");

	if (ImGui::BeginTable("Flight Data Rotary Dial Table", 3, ImGuiTableFlags_None, ImVec2(-1, 0))) 
	{
		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Speed", &(rocket_data->v_values.back()), -100.0f, 100.0f, 0.1f, "%.1f", ImGuiKnobVariant_WiperOnly);

		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Acceleration", &(rocket_data->a_values.back()), -9.8f * 2, 9.8f * 2, 0.1f, "%.1f", ImGuiKnobVariant_WiperOnly);

		ImGui::TableNextColumn();
		ImGuiKnobs::Knob("Altitude", &(rocket_data->z_values.back()), 0.0f, 10000.0f, 0.1f, "%.1f", ImGuiKnobVariant_WiperOnly);

		ImGui::EndTable();
	}

	if (rocket_data->z_values.at(rocket_data->z_values.size() - 1) > apogee)
		apogee = rocket_data->z_values.at(rocket_data->z_values.size() - 1);
	ImGui::InputFloat("Apogee", &apogee, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

	if (rocket_data->v_values.at(rocket_data->v_values.size() - 1) > fastest_speed)
		fastest_speed = rocket_data->v_values.at(rocket_data->v_values.size() - 1);
	ImGui::InputFloat("Max Speed", &fastest_speed, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

	if (rocket_data->a_values.at(rocket_data->a_values.size() - 1) > fastest_aceleration)
		fastest_aceleration = rocket_data->a_values.at(rocket_data->a_values.size() - 1);
	ImGui::InputFloat("Max Acceleration", &fastest_aceleration, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);

	char date_string_input[11] = "";
	strncpy(date_string_input, date_string.c_str(), 11);
	ImGui::InputText("Date", date_string_input, 11, ImGuiInputTextFlags_ReadOnly);

	char time_string_input[11];
	if (rocket_data->launch_time != NULL)
	{
		std::tm* tm_time = std::localtime(&rocket_data->launch_time);
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
		auto now = std::chrono::system_clock::now();
		std::time_t currentTime_t = std::chrono::system_clock::to_time_t(now);
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

	if (rocket_data->launch_time == NULL)
	{
		if (ImGui::Button("Launch Rocket", ImVec2(-1, 70)))
		{
			auto now = std::chrono::system_clock::now();
			std::time_t currentTime_t = std::chrono::system_clock::to_time_t(now);
			rocket_data->launch_time = currentTime_t;

			if (rocket_data->launch_rocket != NULL)
			{	
				rocket_data->launch_rocket(rocket_data->hSerial, rocket_data);
			}
		}
	}
	ImGui::End();
	#pragma endregion
}

void UI::Render(GLFWwindow* window)
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

void UI::RotateModel(std::vector<ImPlot3DPoint> vertices, std::vector<ImPlot3DPoint>* rotated_vertices, float x_rotation, float y_rotation)
{
    for (ImPlot3DPoint vert : vertices) {
        ImPlot3DPoint centered_vert = {vert[0] - 0.5f, vert[1] - 0.5f, vert[2] - 0.5f};
		
		ImPlot3DPoint rotated_vert_x = {centered_vert[0], centered_vert[1] * cos(x_rotation) - centered_vert[2] * sin(x_rotation), centered_vert[1] * sin(x_rotation) + centered_vert[2] * cos(x_rotation)};
		ImPlot3DPoint rotated_vert_y = {rotated_vert_x[0] * cos(y_rotation) + rotated_vert_x[2] * sin(y_rotation), rotated_vert_x[1], -rotated_vert_x[0] * sin(y_rotation) + rotated_vert_x[2] * cos(y_rotation)};

		ImPlot3DPoint final_vert = {rotated_vert_y[0] + 0.5f, rotated_vert_y[1] + 0.5f, rotated_vert_y[2] + 0.5f};
		rotated_vertices->push_back(final_vert);
    }
}

UI::~UI()
{
}

UI* UI::Get()
{
	if (ui == nullptr)
		ui = new UI();
	return ui;
}


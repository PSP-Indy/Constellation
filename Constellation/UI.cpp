#include "UI.hpp"

#include "rocket_model.hpp"

UI::UI()
{
	
}

void UI::Init(GLFWwindow* window, const char* glsl_version)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImPlot3D::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	float scale_factor = 1.5;
	ImFont* largeFont = io.Fonts->AddFontFromFileTTF("DuruSans-Regular.ttf", roundf(16 * scale_factor));
	io.Fonts->Build();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(scale_factor);

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImGui::StyleColorsDark();
}

bool velocity_plot = true;
bool acceleration_plot = true;
bool position_plot = true;

float time_start = 0.0f;
float time_end = 1.0f;

void UI::NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void UI::Update()
{
	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

	ImGui::BeginMainMenuBar();
	if (ImGui::MenuItem("Hello There!")) {
		std::cout << "General Kenobi!" << std::endl;
	}
	ImGui::EndMainMenuBar();

	ImGui::Begin("Figure 1");
	if (ImPlot::BeginPlot("Velocity (ft/s) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); 
        ImPlot::SetupAxis(ImAxis_Y1, "Velocity (ft/s)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("", rocket_data->t_values.data(),  rocket_data->v_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();

	ImGui::Begin("Figure 2");
	if (ImPlot::BeginPlot("Acceleration (ft/s/s) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); 
        ImPlot::SetupAxis(ImAxis_Y1, "Acceleration (ft/s/s)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("", rocket_data->t_values.data(),  rocket_data->a_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();

	ImGui::Begin("Figure 3");
	if (ImPlot::BeginPlot("Position (ft) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); 
        ImPlot::SetupAxis(ImAxis_Y1, "Position (ft)", ImPlotAxisFlags_AutoFit);
		ImPlot::PlotLine("X Positition", rocket_data->t_values.data(),  rocket_data->x_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::PlotLine("Y Positition", rocket_data->t_values.data(),  rocket_data->y_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::PlotLine("Z Positition", rocket_data->t_values.data(),  rocket_data->z_values.data(), static_cast<int>(rocket_data->t_values.size()));
		ImPlot::EndPlot();
	}
	ImGui::End();

	ImGui::Begin("Figure 4");

	ImGui::Checkbox("Velocity", &velocity_plot);
	ImGui::SameLine();
	ImGui::Checkbox("Acceleration", &acceleration_plot);
	ImGui::SameLine();
	ImGui::Checkbox("Position", &position_plot);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::SliderFloat("Time Start", &time_start, 0.0f, rocket_data->t_values.back());
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	ImGui::SliderFloat("Time End", &time_end, 0.0f, rocket_data->t_values.back());

	if (ImPlot::BeginPlot("Time Sliced Plot", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)", ImPlotAxisFlags_AutoFit); 
        ImPlot::SetupAxis(ImAxis_Y1, "Value", ImPlotAxisFlags_AutoFit);
		if (velocity_plot) { ImPlot::PlotLine("Velocity", rocket_data->t_values.data(),  rocket_data->v_values.data(), static_cast<int>(rocket_data->t_values.size())); }
		if (acceleration_plot) { ImPlot::PlotLine("Acceleration", rocket_data->t_values.data(),  rocket_data->a_values.data(), static_cast<int>(rocket_data->t_values.size())); }
		if (position_plot) { ImPlot::PlotLine("X Postion", rocket_data->t_values.data(),  rocket_data->x_values.data(), static_cast<int>(rocket_data->t_values.size())); }
		if (position_plot) { ImPlot::PlotLine("Y Position", rocket_data->t_values.data(),  rocket_data->y_values.data(), static_cast<int>(rocket_data->t_values.size())); }
		if (position_plot) { ImPlot::PlotLine("Z Position", rocket_data->t_values.data(),  rocket_data->z_values.data(), static_cast<int>(rocket_data->t_values.size())); }
		ImPlot::EndPlot();
	}
	ImGui::End();

	std::vector<ImPlot3DPoint> rocket_vertices_rotated;

	RotateModel(rocket_vertices, &rocket_vertices_rotated, 3.14f / 2.0f + rocket_data->x_rotation, rocket_data->y_rotation);

	ImGui::Begin("Figure 5");
	if (ImPlot3D::BeginPlot("3D Rotation", ImVec2(-1, -1))) {
		ImPlot3D::SetupAxisLimits(ImAxis3D_X, 0, 1);
		ImPlot3D::SetupAxisLimits(ImAxis3D_Y, 0, 1);
		ImPlot3D::SetupAxisLimits(ImAxis3D_Z, 0, 1);
		ImPlot3D::SetupAxes("", "", "", ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax, ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax, ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax);
		ImPlot3D::PlotMesh("Orientation", rocket_vertices_rotated.data(), rocket_indices.data(), static_cast<int>(rocket_vertices_rotated.size()), static_cast<int>(rocket_indices.size()), ImPlot3DMeshFlags_NoLegend);
		ImPlot3D::EndPlot();
	}
	ImGui::End();
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


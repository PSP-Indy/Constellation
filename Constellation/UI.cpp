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
	if (ImGui::MenuItem("Exit"))
		exit(0);

	ImGui::Begin("Figure 1");
	if (ImPlot::BeginPlot("Velocity (ft/s) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::SetupAxis(ImAxis_X1, "Time (s)"); 
        ImPlot::SetupAxis(ImAxis_Y1, "Velocity (ft/s)"); 
		ImPlot::PlotLine("As", v_values->data(),  t_values->data(), t_values->size());
		ImPlot::EndPlot();
	}
	ImGui::End();

	ImGui::Begin("Figure 2");
	if (ImPlot::BeginPlot("Position (ft) vs Time (s)", ImVec2(-1, -1))) {
		ImPlot::PlotLine("X Positition", x_values->data(),  t_values->data(), t_values->size());
		ImPlot::PlotLine("Y Positition", y_values->data(),  t_values->data(), t_values->size());
		ImPlot::PlotLine("Z Positition", z_values->data(),  t_values->data(), t_values->size());
		ImPlot::EndPlot();
	}
	ImGui::End();

	ImGui::Begin("Figure 3");
	if (ImPlot3D::BeginPlot("3D Rotation", ImVec2(-1, -1))) {
		ImPlot3D::SetupAxisLimits(ImAxis3D_X, 0, 1);
		ImPlot3D::SetupAxisLimits(ImAxis3D_Y, 0, 1);
		ImPlot3D::SetupAxisLimits(ImAxis3D_Z, 0, 1);
		ImPlot3D::SetupAxes("", "", "", ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax, ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax, ImPlot3DAxisFlags_LockMin + ImPlot3DAxisFlags_LockMax);
		ImPlot3D::PlotMesh("Orientation", rocket_vertices.data(), rocket_indices.data(), rocket_vertices.size(), rocket_indices.size(), ImPlot3DMeshFlags_NoLegend);
		ImPlot3D::EndPlot();
	}
	ImGui::End();

	ImGui::EndMainMenuBar();
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

UI::~UI()
{
}

UI* UI::Get()
{
	if (ui == nullptr)
		ui = new UI();
	return ui;
}


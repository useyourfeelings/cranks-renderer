#include "imgui.h"
#include "core/api.h"
#include "tool/logger.h"

using namespace std::literals;

void InitCanvas() {
    ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Always);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    //ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 20, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

    ImGui::Begin("Canvas");

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    ImVec2 canvas_p1 = ImVec2(p.x + canvas_sz.x, p.y + canvas_sz.y);

    draw_list->AddRectFilled(p, canvas_p1, IM_COL32(60, 60, 60, 255));

    draw_list->AddCircle(ImVec2(22 +p.x , 22 + p.y), 10 * 0.5f, ImColor(0.8f, 1.0f, 0.4f), 12, 2);


    //for (int i = 20; i < 100; ++ i) {
    //    for (int j = 20; j < 100; ++ j) {
    //        //draw_list->AddLine(ImVec2(i + p.x, j + p.y), ImVec2(i + p.x + 1, j + p.y + 1), IM_COL32(i, j, 80, 255), 1);
    //        //draw_list->AddRectFilled(ImVec2(i + p.x, j + p.y), ImVec2(i + p.x + 1, j + p.y + 1), IM_COL32(i, j, 60, 255));
    //    }
    //    

    //}

    //draw_list->AddRectFilledMultiColor(ImVec2(33 + p.x, 33 + p.y), ImVec2(333 + p.x, 333 + p.y), IM_COL32(0, 0, 0, 255), IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255));


    ImGui::End();
}

int make_pbr_ui()
{
    static float f = 0.0f;
    static int counter = 0;

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);


    ImGui::StyleColorsLight();

    InitLogger();

    //InitCanvas();

    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Hello, pbr!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    //ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    //ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    if (ImGui::Button("Button2")) {
        
        counter++;

        PBR_API_print_scene();
    }

    if (ImGui::Button("Render", ImVec2(400, 120))) {

        PBR_API_add_sphere("wtfSphere"s, 6, 0, 0, 28);
        PBR_API_add_point_light("wtf Light"s, 0, 10, 0);
        PBR_API_render();
    }

    ImGui::End();

    return 0;
}
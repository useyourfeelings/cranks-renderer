#include "imgui.h"
#include "core/api.h"
#include "tool/logger.h"

using namespace std::literals;

void InitCanvas() {
    ImGuiWindowFlags window_flags = 0;

    ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_Always);
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    //ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 20, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);

    ImGui::Begin("Canvas");

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    static ImDrawList* saved_draw_list;
    
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
    ImVec2 canvas_p1 = ImVec2(p.x + canvas_sz.x, p.y + canvas_sz.y);

    static int has_render_task = 1;
    //draw_list->AddRectFilled(p, canvas_p1, IM_COL32(60, 60, 60, 255));

    //draw_list->AddCircle(ImVec2(22 +p.x , 22 + p.y), 10 * 0.5f, ImColor(0.8f, 1.0f, 0.4f), 12, 2);

    if (has_render_task == 1) {
        Log("has_render_task");
        has_render_task = 0;

        //IM_FREE()

        for (int i = 20; i < 100; ++i) {
            for (int j = 20; j < 100; ++j) {
                draw_list->AddLine(ImVec2(i + p.x, j + p.y), ImVec2(i + p.x + 1, j + p.y + 1), IM_COL32(i, j, 80, 255), 1);
                draw_list->AddRectFilled(ImVec2(i + p.x, j + p.y), ImVec2(i + p.x + 1, j + p.y + 1), IM_COL32(i, j, 60, 255));
            }
        }

        saved_draw_list = draw_list->CloneOutput();

        //draw_list->AddRectFilledMultiColor(ImVec2(33 + p.x, 33 + p.y), ImVec2(333 + p.x, 333 + p.y), IM_COL32(0, 0, 0, 255), IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255));
    }
    else {
        Log("render saved draw_list");

        //draw_list = saved_draw_list->CloneOutput();

        draw_list = saved_draw_list;

        //draw_list->CmdBuffer = saved_draw_list->CmdBuffer;          // Draw commands. Typically 1 command = 1 GPU draw call, unless the command is a callback.
        //draw_list->IdxBuffer = saved_draw_list->IdxBuffer;          // Index buffer. Each command consume ImDrawCmd::ElemCount of those
        //draw_list->VtxBuffer = saved_draw_list->VtxBuffer;          // Vertex buffer.
        //draw_list->Flags = saved_draw_list->Flags;
    }


    

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

    ImGui::Text("look at");               // Display some text (you can use a format strings too)

    static int pos[3] = {0, 0, 0};
    static int look[3] = { 0, 0, 10};
    static int up[3] = { 0, 1, 0};
    static int fov = 90;
    static float aspect_ratio = 1.2;

    //ImGui::Text("pos");
    //ImGui::SameLine();

    ImGui::PushItemWidth(400);

    bool cameraChanged = false;
    if (ImGui::SliderInt3("pos", pos, -100, 100)) {
        cameraChanged = true;
    }

    if (ImGui::SliderInt3("look", look, -100, 100)) {
        cameraChanged = true;
    }

    if (ImGui::SliderInt3("up", up, -100, 100)) {
        cameraChanged = true;
    }

    ImGui::PopItemWidth();

    ImGui::PushItemWidth(160);

    if (ImGui::SliderInt("fov", &fov, 0, 180)) {
        cameraChanged = true;
    }

    ImGui::SameLine();

    if (ImGui::SliderFloat("aspect_ratio", &aspect_ratio, 0.5f, 2, "%.1f")) {
        cameraChanged = true;
    }

    ImGui::PopItemWidth();

    

    if (cameraChanged) {
        Log("cameraChanged");
    }

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

        PBR_API_add_sphere("wtfSphere 1"s, 6, 0, 0, 28);
        PBR_API_add_sphere("wtfSphere 2"s, 10, -20, -30, 40);
        //PBR_API_add_sphere("wtfSphere 3"s, 20, 30, 30, 60);

        PBR_API_add_point_light("wtf Light"s, 0, 10, 0);
        PBR_API_render();
    }

    ImGui::End();

    return 0;
}
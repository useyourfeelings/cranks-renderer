#include "imgui.h"
#include "core/api.h"
#include "tool/logger.h"
#include "base/events.h"
#include "vulkan/vulkan_main.h"

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

int SetStyle() {
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ItemSpacing = ImVec2(8, 8);
    style.FrameRounding = 2;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;
    style.TabRounding = 2;
    style.WindowRounding = 1;
    style.ScrollbarSize = 16;
    style.WindowMenuButtonPosition = 1; // right
    style.GrabRounding = 1;

    return 0;
}

int RENDER_TASK_ID = -1;

int RendererUI(VulkanApp* app)
{
    static int registered = 0;

    if (registered == 0) {
        RENDER_TASK_ID = RegisterEvent(PBR_API_render);
        registered = 1;
    }

    static int init_status = 0;

    static float pos[3] = { 0, 0, 0 };
    static float look[3] = { 0, 0, 10 };
    static float up[3] = { 0, 1, 0 };
    static float fov = 90;
    static float aspect_ratio = 1.2f;
    static float near_far[2] = { 2, 200};
    static int resolution[2] = { 100, 100 };
    static int ray_sample_no = 1;

    if (init_status == 0) {
        PBR_API_get_camera_setting(pos, look, up, fov, aspect_ratio, near_far[0], near_far[1], resolution[0], resolution[1], ray_sample_no);
        PBR_API_set_perspective_camera(pos, look, up, fov, aspect_ratio, near_far[0], near_far[1], resolution[0], resolution[1], ray_sample_no);
        init_status = 1;
    }

    int progress_now, progress_total, render_status, has_new_photo;
    PBR_API_get_render_progress(&render_status, &progress_now, &progress_total, &has_new_photo);

    if (has_new_photo) {
        //app->BuildImage();
        //PBR_API_get_new_image
    }

    // ui

    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);

    LoggerUI();

    ImGui::SetNextWindowSize(ImVec2(480, 600), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(1);

    //ImGui::Begin("Hello, pbr!");
    if (!ImGui::Begin("Hello Crank!Ãñ¿Æ", nullptr, ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::End();
        return 0;
    }

    if (render_status == 1) {
        ImGui::BeginDisabled();
    }

    ImGui::Text("camera setting");
    ImGui::SameLine();
    if (ImGui::Button("load default")) {
        PBR_API_get_defualt_camera_setting(pos, look, up, fov, aspect_ratio, near_far[0], near_far[1], resolution[0], resolution[1], ray_sample_no);
        PBR_API_set_perspective_camera(pos, look, up, fov, aspect_ratio, near_far[0], near_far[1], resolution[0], resolution[1], ray_sample_no);
    }
    
    ImGui::SameLine();
    if (ImGui::Button("save setting")) {
        PBR_API_save_setting();
    }
    //ImGui::Text("pos");
    //ImGui::SameLine();

    ImGui::PushItemWidth(400);

    bool cameraChanged = false;
    ImGui::SliderFloat3("pos", pos, -100, 100);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraChanged = true;
    }

    ImGui::SliderFloat3("look", look, -100, 100);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraChanged = true;
    }

    ImGui::SliderFloat3("up", up, -100, 100);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraChanged = true;
    }

    ImGui::PopItemWidth();

    ImGui::PushItemWidth(160);

    ImGui::SliderFloat("fov", &fov, 0, 180);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraChanged = true;
    }

    ImGui::SameLine();

    ImGui::SliderFloat("aspect_ratio", &aspect_ratio, 0.5f, 2, "%.1f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraChanged = true;
    }

    ImGui::PopItemWidth();

    ImGui::PushItemWidth(400);

    ImGui::SliderFloat2("near_far", near_far, 0.001f, 1000);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraChanged = true;
    }

    ImGui::SliderInt2("resolution", resolution, 0, 1000);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraChanged = true;
    }

    ImGui::PopItemWidth();

    ImGui::SliderInt("ray_sample_no", &ray_sample_no, 1, 8);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        cameraChanged = true;
    }
    

    if (cameraChanged) {
        Log("cameraChanged");
        PBR_API_set_perspective_camera(pos, look, up, fov, aspect_ratio, near_far[0], near_far[1], resolution[0], resolution[1], ray_sample_no);
    }

    //ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    //ImGui::Checkbox("Another Window", &show_another_window);

    //ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    //ImGui::SameLine();
    //ImGui::Text("counter = %d", counter);

    //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    //ImGui::Dummy(ImVec2(30, 30));


    
    char buf[32];
    sprintf(buf, "%d/%d", progress_now, progress_total);
    ImGui::ProgressBar(float(progress_now) / progress_total, ImVec2(0.f, 0.f), buf);

    if (ImGui::Button("Render", ImVec2(200, 120))) {

        static int set_default_scene = 0;
        if (set_default_scene == 0) {
            set_default_scene = 1;

            PBR_API_add_sphere("wtfSphere 1", 6, 0, 0, 0);
            PBR_API_add_sphere("wtfSphere 2", 10, -20, 40, 20);
            PBR_API_add_sphere("wtfSphere 3", 20, 30, 40, 30);
            PBR_API_add_sphere("wtfSphere 4", 200, 0, 0, -210);
            PBR_API_add_sphere("wtfSphere 5", 2, 7.2, 0, -5);

            //PBR_API_add_point_light("wtf Light"s, 0, 30, 0);
            PBR_API_add_point_light("wtf Light", 0, 0, 30);
        }

        SendEvent(RENDER_TASK_ID);
    }

    if (render_status == 1) {
        ImGui::EndDisabled();
    }

    if (render_status == 0) {
        ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(200, 120))) {

    }

    if (render_status == 0) {
        ImGui::EndDisabled();
    }

    ImGui::End();

    //Log("resolution %d, %d", resolution[0], resolution[1]);
    ImGui::SetNextWindowSize(ImVec2(resolution[0] + 20, resolution[1] + 20));
    ImGui::SetNextWindowPos(ImVec2(20, 180));

    ImGui::Begin("Scene");

    //ImGui::Image((ImTextureID)app.renderImage.descriptorSet);
    //ImGui::Image((ImTextureID)renderImageID, ImVec2(80, 80), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    //Log("renderImageID %d", (ImTextureID)renderImageID);

    //std::cout << "renderImageID = " << renderImageID << std::endl;
    //ImGui::Image((ImTextureID)renderImageID, ImVec2(256, 256), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

    

    ImGui::End();

    return 0;
}
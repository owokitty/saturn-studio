#include "saturn_imgui_chroma.h"

#include <SDL2/SDL_opengl.h>
#include <string>
#include <iostream>

#include "saturn/imgui/saturn_imgui_dynos.h"
#include "saturn/libs/imgui/imgui.h"
#include "saturn/libs/imgui/imgui_internal.h"
#include "saturn/libs/imgui/imgui_impl_sdl.h"
#include "saturn/libs/imgui/imgui_impl_opengl3.h"
#include "saturn/saturn.h"
#include "saturn/saturn_colors.h"
#include "saturn_imgui.h"
#include "pc/controller/controller_keyboard.h"
#include "data/dynos.cpp.h"
#include <SDL2/SDL.h>
#include "icons/IconsForkAwesome.h"

extern "C" {
#include "pc/gfx/gfx_pc.h"
#include "pc/configfile.h"
#include "pc/pngutils.h"
#include "game/mario.h"
#include "game/camera.h"
#include "game/level_update.h"
#include "engine/level_script.h"
#include "engine/geo_layout.h"
#include "src/game/envfx_snow.h"
#include "src/game/object_list_processor.h"
}

using namespace std;

// Variables

ImVec4 uiChromaColor =              ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
bool renderFloor = false;
int currentChromaArea = 1;

bool use_imageref = false;
std::string imageref_filename = "";
struct ImageRef imageref = { .scale = 1 };

void set_chroma_color() {
    int r5 = ((int)(uiChromaColor.x * 255) * 31 / 255);
    int g5 = ((int)(uiChromaColor.y * 255) * 31 / 255);
    int b5 = ((int)(uiChromaColor.z * 255) * 31 / 255);
    int rShift = (int) r5 << 11;
    int bShift = (int) g5 << 6;
    int gShift = (int) b5 << 1;
    gChromaKeyColor = (int) (bShift | gShift | rShift | 1);

    chromaColor.red[0] = (int)(uiChromaColor.x * 255);
    chromaColor.green[0] = (int)(uiChromaColor.y * 255);
    chromaColor.blue[0] = (int)(uiChromaColor.z * 255);
    // No shading
    chromaColor.red[1] = chromaColor.red[0];
    chromaColor.green[1] = chromaColor.green[0];
    chromaColor.blue[1] = chromaColor.blue[0];
}

void schroma_imgui_init() {
    set_chroma_color();
}

void schroma_imgui_update() {
    currentChromaArea = (renderFloor & use_color_background) ? 1 : 2;

    if (autoChroma && gCurrLevelNum != LEVEL_SA) {
        ImGui::Text(ICON_FK_EYEDROPPER " Auto-chroma Enabled");
        ImGui::SameLine(); imgui_bundled_help_marker("Experimental: Allows CHROMA KEY to be automatically enabled from any stage; Useful for maintaining consistent camera angles.");
        ImGui::Checkbox("Show Level", &autoChromaLevel);
        ImGui::Checkbox("Show Objects", &autoChromaObjects);
        ImGui::Dummy(ImVec2(0, 5));
    }

    ImGui::Checkbox("Color Skybox", &use_color_background);
    saturn_keyframe_popout("k_skybox_mode");

    if (use_color_background) {
        if (ImGui::ColorEdit4("Chroma Key Color", (float*)&uiChromaColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions)) {
            set_chroma_color();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            ImGui::OpenPopup("###chromaColorPresets");
        if (ImGui::BeginPopup("###chromaColorPresets")) {
            if (ImGui::Selectable("Green")) uiChromaColor = ImVec4(0.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
            if (ImGui::Selectable("Blue")) uiChromaColor = ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
            if (ImGui::Selectable("Pink")) uiChromaColor = ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
            if (ImGui::Selectable("Black")) uiChromaColor = ImVec4(0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f);
            if (ImGui::Selectable("White")) uiChromaColor = ImVec4(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f);
            ImGui::EndPopup();
        }

        ImGui::SameLine(); ImGui::Text("Color");
        saturn_keyframe_popout("k_color");
        if (gCurrLevelNum == LEVEL_SA) {
            if (ImGui::Checkbox("Render Floor", &renderFloor))
                chromaRequireReload = true;
            ImGui::SameLine(); imgui_bundled_help_marker("Renders a floor object; Useful for animations that clip through the ground.");
            
            if (ImGui::Button("Reload###apply_chroma_color")) {
                using_chainer = false;
                chainer_index = 0;
                is_anim_playing = false;
                is_anim_paused = false;

                mario_loaded = false;
                chromaRequireReload = false;
                bool result = DynOS_Warp_ToLevel(gCurrLevelNum, (s32)currentChromaArea, gCurrActNum);
            } imgui_bundled_tooltip("WARNING: This will restart the level!");
        }
    } else {
        const char* mSkyboxSettings[] = { "Ocean Sky", "Flaming Sky", "Underwater City", "Below Clouds", "Snow Mountains", "Desert", "Haunted", "Green Sky", "Above Clouds", "Purple Sky" };
        ImGui::Combo("###skybox_background", (int*)&gChromaKeyBackground, mSkyboxSettings, IM_ARRAYSIZE(mSkyboxSettings));
        currentChromaArea = gCurrAreaIndex;
    }

    ImGui::Checkbox("Reference Image", &use_imageref);
    if (use_imageref) {
        if (ImGui::Button("Browse")) {
            std::vector<std::string> str = choose_file_dialog("Open Image", { "PNG image", "*.png", "All Files", "*" }, false);
            if (str.size() != 0) {
                fs::path dest = fs::path(sys_user_path()) / "res" / "gfx" / "_IMAGEREF.rgba32.png";
                if (fs::exists(dest)) fs::remove(dest);
                fs::copy_file(str[0], dest);
                imageref_filename = fs::path(str[0]).filename().string();
                imageref.loaded = true;
                int c;
                unsigned char* img = pngutils_read_png(str[0].c_str(), &imageref.width, &imageref.height, &c, 4);
                pngutils_free(img);
                gfx_precache_textures();
            }
        }
        ImGui::SameLine();
        if (!imageref.loaded) ImGui::Text("No file selected");
        else ImGui::Text("%s", imageref_filename.c_str());
        ImGui::SliderFloat("Scale", &imageref.scale, 0.1, 10, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::DragInt("Offset X", &imageref.x);
        ImGui::DragInt("Offset Y", &imageref.y);
    }

    ImGui::Dummy(ImVec2(0, 5));

    ImGui::Checkbox("Shadows###chroma_shadows", &enable_shadows);
    saturn_keyframe_popout("k_shadows");
    ImGui::Checkbox("Dust Particles###chroma_dust", &enable_dust_particles);
    imgui_bundled_tooltip("Displays dust particles when Mario moves; Better to leave off when chroma keying.");
    
    const char* mEnvSettings[] = { "Default", "None", "Snow", "Blizzard" };
    ImGui::Combo("Environment###env_dropdown", (int*)&gLevelEnv, mEnvSettings, IM_ARRAYSIZE(mEnvSettings));
}

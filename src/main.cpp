// TODO
/*
- verify with 2 more random input set
- cannot set already existing label
- LOAD_WORD1 label seems to match WRD1_LOOP
*/

#include <Windows.h>
#include <gl/GL.h>
#include <chrono>
#include <limits>
#include <regex>

#include "imgui.h"
#include "TextEditor.h"
#include "levels.h"
#include "http.h"
#include "help.h"

struct Instruction
{
    int inst;
    int addr;
    bool ind;
    int line;
    char label[LINE_MAX_CHAR];
    char jump_label[LINE_MAX_CHAR];
};

int resolution[2] = { 1016, 660 };
HWND window_handle = nullptr;
HGLRC rendering_context;
HDC device_context;
float dt;
char *code;
TextEditor* textEditor;
int inputs[INPUT_MAX] = {0};
int input_count = 0;
int outputs[OUTPUT_MAX];
int output_count = 0;
int input_cursor = 0;
int ram[RAM_SIZE] = { 0 };
bool used_ram[RAM_SIZE] = { false };
char ram_names[RAM_SIZE][5];
int ram_cursor = -1;
int cycles = 0;
int pc = 0;
int a = 0;
bool running = false;
bool compiled = false;
Instruction* instructions;
int instruction_count = 0;
bool need_save = false;
float save_delay = 0;
float gb_color[4] = { 0.198f, 0.108f, 0.178f, 1.000f };
bool show_win_dialog = false;
int verify_count = 0;
bool show_ram_edit_dialog = false;
bool show_about_dialog = false;
TextEditor::LanguageDefinition lmc_language_definition;
int compile_line;
int edit_addr_name = -1;
char addr_name[5] = { 0 };
float cycle_delay = 0.0f;
int cycle_speed = 1;
bool quit_game = false;
float running_update_delay = 0;
int best_score = -1;

int current_level = 0;

void main_loop();
void reset_gl_states();
void init_ui();
void update();
void render();
void render_ui();
void render_imgui();
void do_cycle();
void save();
void stop();

LRESULT CALLBACK WinProc(HWND handle, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_DESTROY ||
        msg == WM_CLOSE)
    {
        PostQuitMessage(0);
        quit_game = true;
        return 0;
    }
    else if (msg == WM_SIZE)
    {
        resolution[0] = (int)LOWORD(lparam);
        resolution[1] = (int)HIWORD(lparam);
        return 0;
    }
    else if (msg == WM_CHAR)
    {
        auto c = (char)wparam;
        auto& io = ImGui::GetIO();
        io.AddInputCharacter((ImWchar)c);
        return 0;
    }
    else if (msg == WM_KEYDOWN)
    {
        auto key = (int)wparam;
        if (key < 512)
        {
            auto& io = ImGui::GetIO();
            io.KeysDown[key] = true;
            return 0;
        }
    }
    else if (msg == WM_KEYUP)
    {
        auto key = (int)wparam;
        if (key < 512)
        {
            auto& io = ImGui::GetIO();
            io.KeysDown[key] = false;
            return 0;
        }
    }
    else if (msg == WM_MOUSEMOVE)
    {
        auto xPos = (int)LOWORD(lparam);
        auto yPos = (int)HIWORD(lparam);
        auto& io = ImGui::GetIO();
        io.MousePos.x = (float)xPos;
        io.MousePos.y = (float)yPos;
        return 0;
    }
    else if (msg == WM_MOUSELEAVE)
    {
        auto& io = ImGui::GetIO();
        io.MouseDown[0] = false;
        io.MouseDown[1] = false;
        io.MouseDown[2] = false;
        return 0;
    }
    else if (msg == WM_LBUTTONDOWN)
    {
        auto& io = ImGui::GetIO();
        io.MouseDown[0] = true;
        return 0;
    }
    else if (msg == WM_LBUTTONUP)
    {
        auto& io = ImGui::GetIO();
        io.MouseDown[0] = false;
        return 0;
    }
    else if (msg == WM_RBUTTONDOWN)
    {
        auto& io = ImGui::GetIO();
        io.MouseDown[1] = true;
        return 0;
    }
    else if (msg == WM_RBUTTONUP)
    {
        auto& io = ImGui::GetIO();
        io.MouseDown[1] = false;
        return 0;
    }
    else if (msg == WM_MBUTTONDOWN)
    {
        auto& io = ImGui::GetIO();
        io.MouseDown[2] = true;
        return 0;
    }
    else if (msg == WM_MBUTTONUP)
    {
        auto& io = ImGui::GetIO();
        io.MouseDown[2] = false;
        return 0;
    }
    else if (msg == WM_MOUSEWHEEL)
    {
        auto distance = (int)GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
        auto& io = ImGui::GetIO();
        io.MouseWheel = +(float)distance;
        return 0;
    }

    return DefWindowProc(handle, msg, wparam, lparam);
}

GLuint create_texture(unsigned char* pixels, int w, int h)
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    return texture;
}

#define HEX_TO_COLOR(__hex__) ImVec4((float)((__hex__ & 0xFF0000) >> 16) / 255.0f, (float)((__hex__ & 0x00FF00) >> 8) / 255.0f, (float)((__hex__ & 0x0000FF)) / 255.0f, 1.0f)

void init_ui()
{
    auto& io = ImGui::GetIO();
    unsigned char* pixels;
    int w, h, bpp;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h, &bpp);
    auto font_texture = create_texture(pixels, w, h);
    io.Fonts->SetTexID((ImTextureID)font_texture);
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';
    io.MouseDrawCursor = false;

    // Some stylin'
    auto& style = GUI::GetStyle();

    style.GrabRounding = 2;
    style.ChildWindowRounding = 2;
    style.FrameRounding = 2;
    style.ScrollbarRounding = 2;
    style.WindowRounding = 2;

    //style.Colors[ImGuiCol_Text] = HEX_TO_COLOR(0xf1f1f1);
    //style.Colors[ImGuiCol_WindowBg] = HEX_TO_COLOR(0x252526);
    //style.Colors[ImGuiCol_ChildWindowBg] = HEX_TO_COLOR(0x252526);
    //style.Colors[ImGuiCol_PopupBg] = HEX_TO_COLOR(0x252526);
    //style.Colors[ImGuiCol_Border] = HEX_TO_COLOR(0x3f3f46);
    //style.Colors[ImGuiCol_TitleBg] = HEX_TO_COLOR(0x2d2d30);
    style.Colors[ImGuiCol_TitleBgActive] = HEX_TO_COLOR(0xCE7101);
    style.Colors[ImGuiCol_Button] = HEX_TO_COLOR(0x2d2d30);
    style.Colors[ImGuiCol_ButtonHovered] = HEX_TO_COLOR(0x007acc);
    style.Colors[ImGuiCol_ButtonActive] = HEX_TO_COLOR(0xCE7101);
    style.Colors[ImGuiCol_Header] = HEX_TO_COLOR(0xCE7101);
    style.Colors[ImGuiCol_HeaderHovered] = HEX_TO_COLOR(0xCE7101);
    //style.Colors[ImGuiCol_ScrollbarBg] = HEX_TO_COLOR(0x3e3e42);
    //style.Colors[ImGuiCol_ScrollbarGrab] = HEX_TO_COLOR(0x686868);
    //style.Colors[ImGuiCol_ScrollbarGrabHovered] = HEX_TO_COLOR(0x9e9e9e);
    //style.Colors[ImGuiCol_ScrollbarGrabActive] = HEX_TO_COLOR(0xefebef);
    style.Colors[ImGuiCol_Separator] = ImVec4(1.000f, 0.491f, 0.228f, 0.315f);
    style.Colors[ImGuiCol_FrameBg] = {.05f, 0, .04f, 1};
    style.Colors[ImGuiCol_ModalWindowDarkening].w = .5f;
}

#include "Shlobj.h"

std::string get_save_path(std::string file)
{
#if defined(WIN32)
    CHAR szPath[MAX_PATH];
    auto ret = SHGetFolderPathA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath);
    if (ret != S_OK) return file;
    auto full = szPath + std::string("\\ScantAssembly\\");
    auto dir_handle = CreateDirectoryA(full.c_str(), NULL);
    full += file;
    auto handle = CreateFileA(full.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(handle);
    return full;
#else
    return file;
#endif
}

void save_progress()
{
    auto f = fopen(get_save_path("progress.bin").c_str(), "wb");
    uint32_t size = level_count;
    fwrite(&size, sizeof(uint32_t), 1, f);
    for (int i = 0; i < level_count; ++i)
    {
        auto& level = levels[i];
        uint32_t id = level.unique_id;
        fwrite(&id, 4, 1, f);
        uint8_t passed = level.passed ? 1 : 0;
        fwrite(&passed, 1, 1, f);
    }
    fclose(f);
}

void load_progress()
{
    auto f = fopen(get_save_path("progress.bin").c_str(), "rb");
    if (f)
    {
        uint32_t size;
        fread(&size, sizeof(uint32_t), 1, f);
        for (int i = 0; i < (int)size; ++i)
        {
            uint32_t id;
            fread(&id, 4, 1, f);
            uint8_t passed;
            fread(&passed, 1, 1, f);
            for (int j = 0; j < level_count; ++j)
            {
                auto& level = levels[j];
                if (level.unique_id == (int)id)
                {
                    level.passed = passed == 0 ? false : true;
                    break;
                }
            }
        }
        fclose(f);
    }
}

bool throw_error(int line, const char* error_message)
{
    textEditor->SetCursorPosition({ line, 0 });
    textEditor->SetErrorMarkers({ {line + 1, error_message} });
    running_update_delay = 2.0f;
    return false;
}

void def_lang(TextEditor::LanguageDefinition& langDef)
{
    // Addresses
    langDef.mTokenRegexStrings.push_back({ "\\[+[0-9]+\\]+|[0-9]+", TextEditor::TokenType::Number });
    langDef.mTokenRegexStrings.push_back({ "(LDA|STA|ADD|SUB|INC|DEC)\\s+\\[*[A-Z_][A-Z0-9_]*\\]*", TextEditor::TokenType::Keyword });
    langDef.mTokenRegexStrings.push_back({ "(JMP|JPE|JPL|JPG)\\s+[A-Z_][A-Z0-9_]*", TextEditor::TokenType::String });
    langDef.mTokenRegexStrings.push_back({ "(INP|OUT|SET|LDA|STA|ADD|SUB|INC|DEC|JMP|JPE|JPL|JPG|NOP)", TextEditor::TokenType::None });
    langDef.mTokenRegexStrings.push_back({ "[A-Z_][A-Z0-9_]*:", TextEditor::TokenType::String });
    langDef.mTokenRegexStrings.push_back({ "#.*", TextEditor::TokenType::Comment });

    langDef.mCaseSensitive = false;

    langDef.mCommentStart = "/*";
    langDef.mCommentEnd = "*/";

    langDef.mName = "lmc";
}

void init_game()
{
    code = new char[CODE_MAX];
    textEditor = new TextEditor();
    def_lang(lmc_language_definition);
    textEditor->SetLanguageDefinition(lmc_language_definition);
    memset(code, 0, CODE_MAX);
    instructions = new Instruction[PROGRAM_MAX];
    load_progress();
}

void regen_inputs()
{
    auto& level = levels[current_level];

    if (level.input_gen && verify_count < 4)
    {
        memset(level.inputs, 0, sizeof(int) * INPUT_MAX);
        level.input_count = 0;
        memset(level.outputs, 0, sizeof(int) * OUTPUT_MAX);
        level.output_count = 0;
        level.input_gen(level.inputs, level.input_count, level.outputs, level.output_count);
    }
    else
    {
        memcpy(level.inputs, level.safe_inputs, sizeof(int) * level.safe_input_count);
        level.input_count = level.safe_input_count;
        memcpy(level.outputs, level.safe_outputs, sizeof(int) * level.safe_output_count);
        level.output_count = level.safe_output_count;
    }
    memcpy(inputs, level.inputs, sizeof(int) * level.input_count);
    input_count = level.input_count;
}

void load_level(int level_id)
{
    save();
    current_level = level_id;
    running = false;
    pc = 0;
    a = 0;
    compiled = false;
    instruction_count = 0;
    auto& level = levels[level_id];
    memcpy(ram, level.ram, sizeof(int) * RAM_SIZE);
    memset(used_ram, 0, sizeof(bool) * RAM_SIZE);
    for (int i = 0; i < RAM_SIZE; ++i)
    {
        if (ram[i]) used_ram[i] = true;
    }
    memset(outputs, 0, sizeof(int) * RAM_SIZE);
    regen_inputs();
    memset(code, 0, CODE_MAX);
    char filename[100] = { 0 };
    sprintf(filename, "level_%i.bin", level.unique_id);
    memset(ram_names, 0, RAM_SIZE * 5);
    auto f = fopen(get_save_path(filename).c_str(), "rb");
    if (f)
    {
        uint32_t size;
        fread(ram_names, 1, RAM_SIZE * 5, f);
        fread(&size, sizeof(uint32_t), 1, f);
        fread(code, 1, size, f);
        fclose(f);
    }
    input_cursor = 0;
    output_count = 0;
    textEditor->SetText(code);
    best_score = -1;

    // Load best score from http
    auto level_real_id = level.unique_id;
    httpGetAsync("http://www.daivuk.com/pg_leaderboard_get.php", { {"token", "VK5OT3F35VhLxHqp7O7kXuisIdriSX7g"}, { "level_id", std::to_string(level_real_id) } }, 
        [=](std::string body)
    {
        if (level_real_id != levels[current_level].unique_id) return;
        try
        {
            best_score = std::stoi(body);
        }
        catch (...)
        {
            best_score = -1;
        }
    },
        [=](long error_code, std::string message)
    {
        if (level_real_id != levels[current_level].unique_id) return;
        best_score = -1;
    });

    stop();
}

void save()
{
    if (need_save)
    {
        auto& level = levels[current_level];
        char filename[100] = { 0 };
        sprintf(filename, "level_%i.bin", level.unique_id);

        auto f = fopen(get_save_path(filename).c_str(), "wb");
        fwrite(ram_names, 1, RAM_SIZE * 5, f);
        uint32_t size = strlen(code) + 1;
        fwrite(&size, sizeof(uint32_t), 1, f);
        fwrite(code, 1, size, f);
        fclose(f);
        need_save = false;
    }
}

void destroy_game()
{
    save();
    delete textEditor;
    delete[] code;
    delete[] instructions;
}

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    srand((unsigned int)time(nullptr));

    // Define window style
    WNDCLASSA wc = { 0 };
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WinProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "PGWindow";
    RegisterClassA(&wc);

    // Centered position
    auto screenW = GetSystemMetrics(SM_CXSCREEN);
    auto screenH = GetSystemMetrics(SM_CYSCREEN);

    // Window creationg
    auto posX = (screenW - resolution[0]) / 2;
    auto posY = (screenH - resolution[1]) / 2;
    window_handle = CreateWindowA("PGWindow", GAME_NAME,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        posX, posY, resolution[0], resolution[1],
        nullptr, nullptr, nullptr, nullptr);

    // Fix client rect to match desired resolution
    RECT clientRect;
    GetClientRect(window_handle, &clientRect);
    auto wDiff = resolution[0] - (clientRect.right - clientRect.left);
    auto hDiff = resolution[1] - (clientRect.bottom - clientRect.top);
    auto newW = resolution[0] + wDiff;
    auto newH = resolution[1] + hDiff;
    posX = (screenW - newW) / 2;
    posY = (screenH - newH) / 2;
    SetWindowPos(window_handle, nullptr, posX, posY, newW, newH, 0);
    //ShowCursor(FALSE);

    // Initialize OpenGL
    GLuint PixelFormat;
    static PIXELFORMATDESCRIPTOR pfd =  // pfd Tells Windows How We Want Things To Be
    {
        sizeof(PIXELFORMATDESCRIPTOR),  // Size Of This Pixel Format Descriptor
        1,                              // Version Number
        PFD_DRAW_TO_WINDOW |            // Format Must Support Window
        PFD_SUPPORT_OPENGL |            // Format Must Support OpenGL
        PFD_DOUBLEBUFFER,               // Must Support Double Buffering
        PFD_TYPE_RGBA,                  // Request An RGBA Format
        32,                             // Select Our Color Depth
        0, 0, 0, 0, 0, 0,               // Color Bits Ignored
        0,                              // No Alpha Buffer
        0,                              // Shift Bit Ignored
        0,                              // No Accumulation Buffer
        0, 0, 0, 0,                     // Accumulation Bits Ignored
        16,                             // 16Bit Z-Buffer (Depth Buffer)
        0,                              // No Stencil Buffer
        0,                              // No Auxiliary Buffer
        PFD_MAIN_PLANE,                 // Main Drawing Layer
        0,                              // Reserved
        0, 0, 0                         // Layer Masks Ignored
    };

    device_context = GetDC(window_handle);
    PixelFormat = ChoosePixelFormat(device_context, &pfd);
    SetPixelFormat(device_context, PixelFormat, &pfd);

    rendering_context = wglCreateContext(device_context);
    wglMakeCurrent(device_context, rendering_context);

    reset_gl_states();

    // Init imgui
    init_ui();

    init_game();
    load_level(0);

    // Main loop
    main_loop();

    destroy_game();

    // Destroy gl
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(rendering_context);
    ReleaseDC(window_handle, device_context);
}

void main_loop()
{
    auto prev_time = std::chrono::high_resolution_clock::now();

    while (!quit_game)
    {
        bool is_net_updating = false;
        net_mutex.lock();
        is_net_updating = request_count > 0 || !net_callbacks.empty();
        net_mutex.unlock();

        // Messaging
        MSG message;
        if (running || running_update_delay > 0.0f || is_net_updating)
        {
            if (running) running_update_delay = 2.0f;
            else running_update_delay -= dt;

            if (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
                if (message.message == WM_QUIT)
                {
                    break;
                }
            }
        }
        else
        {
            if (GetMessage(&message, 0, 0, 0) >= 0)
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
                if (message.message == WM_QUIT)
                {
                    break;
                }
            }
        }

        auto now_time = std::chrono::high_resolution_clock::now();
        auto delta_time = now_time - prev_time;
        prev_time = now_time;
        dt = (float)((double)std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count() / 1000000.0);

        // Update
        update();

        // Render
        reset_gl_states();
        render();

        // Prepare ui frame
        auto& io = ImGui::GetIO();
        io.DisplaySize.x = (float)resolution[0];
        io.DisplaySize.y = (float)resolution[1];
        io.DeltaTime = dt;
        io.KeyCtrl = GetKeyState(VK_LCONTROL) & 0x8000 || GetKeyState(VK_RCONTROL) & 0x8000;
        io.KeySuper = GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000;
        io.KeyAlt = GetKeyState(VK_LMENU) & 0x8000 || GetKeyState(VK_RMENU) & 0x8000;
        io.KeyShift = GetKeyState(VK_LSHIFT) & 0x8000 || GetKeyState(VK_RSHIFT) & 0x8000;
        
        ImGui::NewFrame();
        render_ui();
        render_imgui();

        // Swap
        SwapBuffers(device_context);
    }
}

void reset_gl_states()
{
    glClearColor(gb_color[0], gb_color[1], gb_color[2], gb_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, resolution[0], resolution[1]);
    glViewport(0, 0, resolution[0], resolution[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, resolution[0], resolution[1], 0, -999, 999);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void render_imgui()
{
    ImGui::Render();
    auto draw_data = ImGui::GetDrawData();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glPushAttrib(GL_SCISSOR_BIT);

    GLuint last_texture = 0;

    for (auto i = 0; i < draw_data->CmdListsCount; ++i)
    {
        auto cmd_list = draw_data->CmdLists[i];
        auto indices = cmd_list->IdxBuffer.Data;
        auto vertices = cmd_list->VtxBuffer.Data;

        glVertexPointer(2, GL_FLOAT, sizeof(decltype(*vertices)), &vertices->pos.x);
        glTexCoordPointer(2, GL_FLOAT, sizeof(decltype(*vertices)), &vertices->uv.x);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(decltype(*vertices)), &vertices->col);

        for (auto j = 0; j < cmd_list->CmdBuffer.Size; ++j)
        {
            auto& draw_cmd = cmd_list->CmdBuffer[j];

            int clip_w = (int)draw_cmd.ClipRect.z - (int)draw_cmd.ClipRect.x;
            int clip_h = (int)draw_cmd.ClipRect.w - (int)draw_cmd.ClipRect.y;
            glScissor((int)draw_cmd.ClipRect.x, resolution[1] - (int)draw_cmd.ClipRect.y - clip_h, clip_w, clip_h);
            if (last_texture != (GLuint)draw_cmd.TextureId)
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)draw_cmd.TextureId);
                last_texture = (GLuint)draw_cmd.TextureId;
            }
            glDrawElements(GL_TRIANGLES, draw_cmd.ElemCount, GL_UNSIGNED_SHORT, indices);
            indices += draw_cmd.ElemCount;
        }
    }

    glPopAttrib();

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

void update()
{
    update_net();
    auto& io = ImGui::GetIO();
    save_delay += dt;
    if (save_delay >= 5)
    {
        save();
        save_delay = 0;
    }
    if (running)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        const float speeds[] = { 1.0f / 10.0f, 1.0f / 30.0f, 1.0f / 60.0f, 1.0f / 10000.0f };
        const float speed_multipliers[] = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
        const float speed = speeds[cycle_speed] / speed_multipliers[verify_count];
        while (cycle_delay <= 0.0f)
        {
            cycle_delay += speed;
            do_cycle();
            if (!running) break;
            if (std::chrono::high_resolution_clock::now() - start_time >= std::chrono::microseconds(16667)) break; // Try to keep 60 fps
        }
        if (running) cycle_delay -= dt;
        else
        {
            // Check if output matches desired output, and pass the level if it's the case
            auto& level = levels[current_level];
            if (memcmp(outputs, level.outputs, sizeof(int) * level.output_count) == 0)
            {
                ++verify_count;
                if (verify_count < 5)
                {
                    regen_inputs();
                    auto prev = verify_count;
                    stop();
                    verify_count = prev;
                    running = true;
                }
                else
                {
                    verify_count = 0;
                    if (!level.passed)
                    {
                        level.passed = true;
                        save_progress();
                    }
                    show_win_dialog = true;
                    // Update score
                    int level_id = levels[current_level].unique_id;
                    httpGetAsync("http://www.daivuk.com/pg_leaderboard_set.php", { { "token", "VK5OT3F35VhLxHqp7O7kXuisIdriSX7g" },{ "level_id", std::to_string(level_id)},{ "instruction_count", std::to_string(instruction_count - 1) } },
                        [=](std::string body)
                    {
                        if (level_id != levels[current_level].unique_id) return;
                        try
                        {
                            best_score = std::stoi(body);
                        }
                        catch (...)
                        {
                            best_score = -1;
                        }
                    },
                        [=](long error_code, std::string message)
                    {
                        if (level_id != levels[current_level].unique_id) return;
                        best_score = -1;
                    });
                }
            }
        }
    }
    if (io.KeysDown[VK_ESCAPE])
    {
        running = false;
        stop();
    }
}

void render()
{

}

bool compile();

void change_addr_name(int addr, char* new_name)
{
    if (strcmp(ram_names[addr], new_name) == 0) return; // No change

    // We compile first to make sure errors are resolved
    //if (!compile()) return;

    char prev_name[5];
    memcpy(prev_name, ram_names[addr], 5);
    if (prev_name[0] == 0) sprintf(prev_name, "%i", addr);
    if (new_name[0] == 0) sprintf(new_name, "%i", addr);

    // Parse the code and change addresses reference
    std::string new_code = code;
    std::string prev_name_str = prev_name;
    std::string new_name_str = new_name;
    std::string replace = "$01" + new_name_str + "$3";
    std::regex reg("(\\s*LDA\\s*\\[*|\\s*STA\\s*\\[*|\\s*ADD\\s*\\[*|\\s*SUB\\s*\\[*|\\s*INC\\s*\\[*|\\s*DEC\\s*\\[*)(" + prev_name_str + ")(\\]*)");

    new_code = std::regex_replace(new_code, reg, replace);

    memcpy(code, new_code.c_str(), new_code.size() + 1);
    textEditor->SetText(new_code);
    if (new_name[0] >= '0' && new_name[0] <= '9') new_name[0] = 0;
    memcpy(ram_names[addr], new_name, 5);

    need_save = true;
}

void render_sequencial_data(const char* title, int* data, int count, int cursor, int* target = nullptr, int target_count = 0)
{
    if (GUI::Begin(title, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Columns(8, nullptr, true);
        for (int i = 0; i < 64; ++i)
        {
            if (i % 8 == 0)
            {
                ImGui::Separator();
            }
            if (i >= count)
            {
                if (target && i < target_count)
                {
                    if (IS_LETTER(target[i]))
                        GUI::TextColored({ .2f, .2f, .2f, 1 }, "%c", (char)TO_NUMBER(target[i]));
                    else
                        GUI::TextColored({ .2f, .2f, .2f, 1 }, "%i", target[i]);
                    ImGui::NextColumn();
                    continue;
                }
                GUI::Text(" ");
                ImGui::NextColumn();
                continue;
            }
            auto input = data[i];
            if (IS_LETTER(input))
            {
                input = TO_NUMBER(input);
                if (cursor == i) GUI::TextColored({ 1, 1, 0, 1 }, "%c", (char)input);
                else if (i < cursor) GUI::TextColored({ .6f, .0f, .6f, 1 }, "%c", (char)input);
                else GUI::TextColored({ 1, .4f, 1, 1 }, "%c", (char)input);
            }
            else
            {
                if (cursor == i) GUI::TextColored({ 1, 1, 0, 1 }, "%i", input);
                else if (i < cursor) GUI::TextColored({ .4f, .4f, .4f, 1 }, "%i", input);
                else GUI::TextColored({ .8f, .8f, .8f, 1 }, "%i", input);
            }
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::Separator();
    }
    GUI::End();
}

void render_dynamic_data(const char* title, int* data, int cursor)
{
    char text[6];
    if (GUI::Begin(title, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Columns(8, nullptr, true);
        for (int i = 0; i < 64; ++i)
        {
            if (i % 8 == 0)
            {
                ImGui::Separator();
            }
            if (ram_names[i][0] != 0) sprintf(text, "%s\n", ram_names[i]);// GUI::TextColored({ .4f, .4f, .4f, 1 }, "%s\n", ram_names[i]);
            else sprintf(text, "%i\n", i); //GUI::TextColored({ .4f, .4f, .4f, 1 }, "[%02i]\n", i);
            static bool selected = false;
            GUI::PushStyleColor(ImGuiCol_Text, (ram_names[i][0] != 0) ? ImVec4(.4f, .8f, 1, 1) : ImVec4(.4f, .4f, .4f, 1));
            if (GUI::Selectable(text, selected))
            {
                edit_addr_name = i;
                memcpy(addr_name, ram_names[i], 5);
                show_ram_edit_dialog = true;
                running_update_delay = 2.0f;
            }
            GUI::PopStyleColor();
            auto input = data[i];
            if (IS_LETTER(input))
            {
                input = TO_NUMBER(input);
                if (cursor == i) GUI::TextColored({ 1, 1, 0, 1 }, "%c", (char)input);
                else GUI::TextColored({ 1, .4f, 1, 1 }, "%c", (char)input);
            }
            else
            {
                if (cursor == i) GUI::TextColored({ 1, 1, 0, 1 }, "%i", input);
                else if (used_ram[i]) GUI::TextColored({ .8f, .8f, .8f, 1 }, "%i", input);
                else GUI::TextColored({ .2f, .2f, .2f, 1 }, "%i", input);
            }
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::Separator();
    }
    GUI::End();
}

void addressing_tooltip()
{
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("RAM address: JMP 15");
        ImGui::Text("or");
        ImGui::Text("Indirect address: JMP [15]");
        ImGui::EndTooltip();
    }
}

void stop()
{
    running = false;
    cycles = 0;
    pc = 0;
    a = 0;
    cycle_delay = 0.0f;
    input_cursor = 0;
    output_count = 0;
    ram_cursor = -1;
    verify_count = 0;
    memset(outputs, 0, sizeof(int) * OUTPUT_MAX);
    memcpy(ram, levels[current_level].ram, sizeof(int) * RAM_SIZE);
    memset(used_ram, 0, sizeof(bool) * RAM_SIZE);
    for (int i = 0; i < RAM_SIZE; ++i)
    {
        if (ram[i]) used_ram[i] = true;
    }

    textEditor->SetBreakpoints({});
    textEditor->SetErrorMarkers({});
}

void render_ui()
{
    //ImGui::ShowTestWindow();
    //return;

    auto& io = GUI::GetIO();

    if (show_win_dialog)
    {
        show_win_dialog = false;
        GUI::OpenPopup("Success!");
    }
    if (show_ram_edit_dialog)
    {
        GUI::OpenPopup("Edit Address Name");
    }
    if (show_about_dialog)
    {
        show_about_dialog = false;
        GUI::OpenPopup("About##Dialog");
    }

    if (GUI::BeginMainMenuBar())
    {
#if _DEBUG
        const char update_anim[] = { '/', '-', '\\', '|' };
        static int cur_frame = 0;
        ++cur_frame;
        GUI::TextDisabled("%c", update_anim[cur_frame % 4]);
#endif
        if (GUI::BeginMenu("File"))
        {
            if (GUI::MenuItem("Exit", "Alt+F4")) PostQuitMessage(0);
            GUI::EndMenu();
        }
        if (GUI::BeginMenu("Options"))
        {
            //if (GUI::BeginMenu("Program execution speed"))
            //{
                int prev = cycle_speed;
                if (GUI::MenuItem("Slow", nullptr, cycle_speed == 0)) cycle_speed = 0;
                if (GUI::MenuItem("Normal", nullptr, cycle_speed == 1)) cycle_speed = 1;
                if (GUI::MenuItem("Fast", nullptr, cycle_speed == 2)) cycle_speed = 2;
                if (GUI::MenuItem("Lightspeed", nullptr, cycle_speed == 3)) cycle_speed = 3;
                //GUI::EndMenu();
                if (prev != cycle_speed) running_update_delay = 2.0f;
            //}
            GUI::EndMenu();
        }
        if (GUI::BeginMenu("Levels"))
        {
            for (int i = 0; i < level_count; ++i)
            {
                auto& level = levels[i];
                if (GUI::MenuItem(level.name, "", level.passed))
                {
                    load_level(i);
                    running_update_delay = 2.0f;
                }
            }
            GUI::EndMenu();
        }
        if (GUI::MenuItem("About"))
        {
            show_about_dialog = true;
            running_update_delay = 2.0f;
        }

        GUI::TextDisabled(levels[current_level].name);
        GUI::TextColored({ 1, 1, 0, 1 }, levels[current_level].description);

        GUI::EndMainMenuBar();
    }

    //GUI::SetNextWindowPos({ 10, 32 });
    //GUI::SetNextWindowSize({ 316, 535 });
    GUI::SetNextWindowPos({ 2, 21 });
    GUI::SetNextWindowSize({ 316 + 40, 535 + 24 });
    if (GUI::Begin("Editor", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
    {
        textEditor->Render("Editor", { 300 + 40, 500 + 24 }, true);
        auto text = textEditor->GetText();
        if ((int)strlen(code) != (int)text.size() ||
            memcmp(code, text.c_str(), text.size()) != 0)
        {
            compiled = false;
            need_save = true;
            memcpy(code, text.c_str(), text.size() + 1);
        }
        //if (GUI::InputTextMultiline("##Code", code, CODE_MAX, { 200, 500 },
        //    ImGuiInputTextFlags_CharsUppercase |
        //    ImGuiInputTextFlags_AllowTabInput |
        //    ImGuiInputTextFlags_NoHorizontalScroll))
        //{
        //    compiled = false;
        //    need_save = true;
        //}
    }
    GUI::End();

    //GUI::SetNextWindowPos({ 669,451 });
    //GUI::SetNextWindowSize({ 301,118 });
    GUI::SetNextWindowPos({ 316 + 320 + 6 + 40,21 + 200 + 2 + 200 + 2 });
    GUI::SetNextWindowSize({ 300,118+37 });
    if (GUI::Begin("Debug", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
    { 
        if (GUI::Button("Stop"))
        {
            running_update_delay = 2.0f;
            stop();
        }
        if (GUI::IsItemHovered()) GUI::SetTooltip("Escape");
        GUI::SameLine();
        if (GUI::Button("Play") || (io.KeysDown[VK_F5] && !running))
        {
            running = true;
        }
        if (GUI::IsItemHovered()) GUI::SetTooltip("F5");
        GUI::SameLine();
        if (GUI::Button("Step")/* || io.KeysDown[VK_F10]*/)
        {
            running = false;
            running_update_delay = 2.0f;
            do_cycle();
        }
        GUI::SameLine();
        if (GUI::Button("Regenerate Inputs"))
        {
            stop();
            regen_inputs();
        }
        //if (GUI::IsItemHovered()) GUI::SetTooltip("F10");
        GUI::Separator();
        GUI::Columns(2);
        if (IS_LETTER(a))
        {
            GUI::Text("%c", (char)(TO_NUMBER(a)));
            GUI::NextColumn();
            GUI::Text("A");
        }
        else
        {
            GUI::Text("%i", a);
            GUI::NextColumn();
            GUI::Text("A");
        }
        GUI::NextColumn();
        GUI::Separator();
        GUI::Text("%i", cycles);
        GUI::NextColumn();
        GUI::Text("Cycles");
        GUI::NextColumn();
        GUI::Text("%i", pc);
        GUI::NextColumn();
        GUI::Text("PC");
        //if (compiled)
        {
            GUI::NextColumn();
            GUI::Text("%i", instruction_count - 1);
            GUI::NextColumn();
            GUI::Text("Instruction Count");
            GUI::NextColumn();
        }
        if (best_score == -1) GUI::Text("UNKNOWN");
        else GUI::Text("%i", best_score);
        GUI::NextColumn();
        GUI::Text("Best");
        GUI::NextColumn();
    }
    GUI::End();

    //GUI::SetNextWindowPos({ 668,32 });
    //GUI::SetNextWindowSize({ 300,200 });
    GUI::SetNextWindowPos({ 316 + 320 + 6 + 40,21 });
    GUI::SetNextWindowSize({ 300,200 });
    render_sequencial_data("Inputs", inputs, input_count, input_cursor);
    //GUI::SetNextWindowPos({ 669,241 });
    //GUI::SetNextWindowSize({ 300,200 });
    GUI::SetNextWindowPos({ 316 + 320 + 6 + 40,21 + 200 + 2 });
    GUI::SetNextWindowSize({ 300,200 });
    render_sequencial_data("Outputs", outputs, output_count, output_count - 1, levels[current_level].outputs, levels[current_level].output_count);
    //GUI::SetNextWindowPos({ 338,32 });
    //GUI::SetNextWindowSize({ 320,350 });
    GUI::SetNextWindowPos({ 316 + 4 + 40,21 });
    GUI::SetNextWindowSize({ 320,340 });
    render_dynamic_data("RAM", ram, ram_cursor);

    //GUI::SetNextWindowPos({ 338,394 });
    //GUI::SetNextWindowSize({ 320,174 });
    GUI::SetNextWindowPos({ 316 + 4 + 40,363 });
    GUI::SetNextWindowSize({ 320,174 + 43 });
    if (GUI::Begin("Documentation", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
    {
        display_help();
    }
    GUI::End();

    if (GUI::BeginPopupModal("Success!", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
    {
        GUI::Text("You have succeeded the level!\nCongratulation!");
        if (GUI::Button("Close"))
        {
            GUI::CloseCurrentPopup();
            stop();
            running_update_delay = 2.0f;
        }
        GUI::SameLine();
        if (GUI::Button("Next Level"))
        {
            GUI::CloseCurrentPopup();
            stop();
            if (current_level + 1 < level_count)
            {
                load_level(current_level + 1);
            }
            running_update_delay = 2.0f;
        }
        GUI::SameLine();
        if (GUI::Button("Next Unresolved Level"))
        {
            GUI::CloseCurrentPopup();
            stop();
            for (int i = current_level + 1; i < level_count; ++i)
            {
                auto& level = levels[i];
                if (!level.passed)
                {
                    load_level(i);
                    break;
                }
            }
            running_update_delay = 2.0f;
        }
        GUI::EndPopup();
    }
    if (GUI::BeginPopupModal("Edit Address Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
    {
        bool escKey = io.KeysDown[io.KeyMap[ImGuiKey_Escape]];
        //bool enterKey = io.KeysDown[io.KeyMap[ImGuiKey_Enter]];
        bool ret = GUI::InputText("Address name", addr_name, 5, ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
        if (show_ram_edit_dialog)
        {
            GUI::SetKeyboardFocusHere(0);
            show_ram_edit_dialog = false;
            ret = false;
            running_update_delay = 2.0f;
        }
        if (GUI::Button("    OK    ##EditAddrOk") || ret /*|| enterKey*/)
        {
            // Validate
            if (!std::regex_match(addr_name, std::regex("[A-Z][A-Z0-9_]*")))
            {
                addr_name[0] = 0;
            }
            change_addr_name(edit_addr_name, addr_name);
            GUI::CloseCurrentPopup();
            running_update_delay = 2.0f;
        }
        GUI::SameLine();
        if (GUI::Button("Cancel") || escKey)
        {
            GUI::CloseCurrentPopup();
            running_update_delay = 2.0f;
        }
        GUI::EndPopup();
    }
    if (GUI::BeginPopupModal("About##Dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
    {
        GUI::Text(GAME_NAME);
        GUI::SameLine();
        GUI::TextDisabled("Version %i.%i", VERSION_MAJOR, VERSION_MINOR);
        GUI::Spacing();
        GUI::Spacing();
        GUI::Spacing();
        GUI::TextDisabled("Create by");
        GUI::SameLine();
        GUI::Text("David St-Louis");
        GUI::SameLine();
        if (GUI::Selectable("@Daivuk"))
        {
#if WIN32
            ShellExecuteW(0, 0, L"https://twitter.com/Daivuk", 0, 0, SW_SHOW);
#else
            system("open https://twitter.com/Daivuk");
#endif
        }
        GUI::Spacing();
        GUI::Spacing();
        GUI::Spacing();
        GUI::Text("Special thanks to the creators of:");
        GUI::BulletText("Dear ImGui by Omar");
        GUI::SameLine();
        if (GUI::Selectable("@ocornut"))
        {
#if WIN32
            ShellExecuteW(0, 0, L"https://twitter.com/ocornut", 0, 0, SW_SHOW);
#else
            system("open https://twitter.com/ocornut");
#endif
        }
        GUI::BulletText("TextEditor by Balazs Jako");
        GUI::SameLine();
        if (GUI::Selectable("@Balazs_Jako"))
        {
#if WIN32
            ShellExecuteW(0, 0, L"https://twitter.com/Balazs_Jako", 0, 0, SW_SHOW);
#else
            system("open https://twitter.com/Balazs_Jako");
#endif
        }
        GUI::Spacing();
        GUI::Spacing();
        GUI::Spacing();
        if (GUI::Button("    OK    ##About"))
        {
            GUI::CloseCurrentPopup();
            running_update_delay = 2.0f;
        }
        GUI::EndPopup();
    }
}


bool trim(int& i, char*& ptr)
{
    while (i < CODE_MAX && (*ptr == ' ' || *ptr == '\t'))
    {
        ++i;
        ++ptr;
    }
    return (i >= CODE_MAX);
}

bool ignore_line(int& i, char*& ptr)
{
    while (i < CODE_MAX && *ptr != '\n')
    {
        ++i;
        ++ptr;
    }
    if (i >= CODE_MAX) return true;
    --i;
    --ptr;
    return false;
}

bool is_inst(const char* inst_name, int& i, char*& ptr)
{
    int j = 0;
    while (inst_name[j])
    {
        if (inst_name[j] != ptr[j]) return false;
        ++j;
    }
    if (ptr[j] != ' ' && ptr[j] != '\0' && ptr[j] != '\n') return false;
    i += j;
    ptr += j;
    return true;
}

bool parse_addr(Instruction& instruction, int& i, char*& ptr)
{
    if (ptr[0] == '-') return throw_error(compile_line, "Negative address");
    if (ptr[0] == '[')
    {
        instruction.ind = true;

        // Check if it's a name
        char name[7];
        snprintf(name, 7, "%s", ptr);
        std::smatch match;
        std::string name_str = name;
        if (std::regex_search(name_str, match, std::regex("\\[([A-Z][A-Z0-9_]*)\\]")))
        {
            // yay
            auto addr_name = match[1].str();
            auto len = addr_name.size() + 2;
            for (int n = 0; n < RAM_SIZE; ++n)
            {
                if (strcmp(addr_name.c_str(), ram_names[n]) == 0)
                {
                    instruction.addr = n;
                    i += len;
                    ptr += len;
                    return true;
                }
            }
            return throw_error(compile_line, "Invalid address name");
        }

        if (i + 2 >= CODE_MAX) return throw_error(compile_line, "Expected digit");
        ++ptr;
        ++i;
        if (ptr[0] < '0' || ptr[0] > '9') return throw_error(compile_line, "Expected digit");
        instruction.addr = (int)ptr[0] - '0';
        ++ptr;
        ++i;
        if (ptr[0] == ']')
        {
            ++ptr;
            ++i;
            return true;
        }
        if (i + 1 >= CODE_MAX) return throw_error(compile_line, "Expected digit");
        if (ptr[0] < '0' || ptr[0] > '9') return throw_error(compile_line, "Expected digit");
        if (instruction.addr == 0) return throw_error(compile_line, "Invalid prefix '0'");
        instruction.addr *= 10;
        instruction.addr += (int)ptr[0] - '0';
        ++ptr;
        ++i;
        if (ptr[0] == ']')
        {
            ++ptr;
            ++i;
            return true;
        }
        return throw_error(compile_line, "]");
    }
    else
    {
        instruction.ind = false;

        // Check if it's a name
        if (ptr[0] < '0' || ptr[0] > '9')
        {
            char name[5];
            snprintf(name, 5, "%s", ptr);
            std::smatch match;
            std::string name_str = name;
            if (std::regex_search(name_str, match, std::regex("([A-Z][A-Z0-9_]*)")))
            {
                // yay
                auto addr_name = match[1].str();
                auto len = addr_name.size();
                for (int n = 0; n < RAM_SIZE; ++n)
                {
                    if (strcmp(addr_name.c_str(), ram_names[n]) == 0)
                    {
                        instruction.addr = n;
                        i += len;
                        ptr += len;
                        return true;
                    }
                }
                return throw_error(compile_line, "Invalid address name");
            }
        }

        if (i + 1 >= CODE_MAX) return throw_error(compile_line, "Expected digit");
        if (ptr[0] < '0' || ptr[0] > '9') return throw_error(compile_line, "Expected digit");
        instruction.addr = (int)ptr[0] - '0';
        ++ptr;
        ++i;
        if (ptr[0] < '0' || ptr[0] > '9')
        {
            return true; // That's fine
        }
        instruction.addr *= 10;
        if (instruction.addr == 0) return throw_error(compile_line, "Invalid prefix '0'");
        instruction.addr += (int)ptr[0] - '0';
        ++ptr;
        ++i;
        return true;
    }
}

bool parse_value(Instruction& instruction, int& i, char*& ptr)
{
    bool negative = false;
    if (ptr[0] == '-')
    {
        negative = true;
        ++i;
        ++ptr;
    }
    if (ptr[0] >= 'A' && ptr[0] <= 'Z')
    {
        if (negative) return throw_error(compile_line, "Negative letter");
        instruction.addr = TO_LETTER(ptr[0]);
        ++i;
        ++ptr;
        return true;
    }
    else
    {
        instruction.addr = 0;
        int digit_count = 0;
        while (ptr[0] >= '0' && ptr[0] <= '9')
        {
            ++digit_count;
            instruction.addr *= 10;
            instruction.addr += (int)ptr[0] - (int)'0';
            ++i;
            ++ptr;
        }
        if (digit_count == 0) return throw_error(compile_line, "Expected digit");
        if (negative) instruction.addr = -instruction.addr;
        return true;
    }
}

bool parse_label(Instruction& instruction, int& i, char*& ptr)
{
    if ((ptr[0] < 'A' || ptr[0] > 'Z') && ptr[0] != '_') return throw_error(compile_line, "Expected label");

    int j = 0;
    for (; j < LINE_MAX_CHAR && i + j < CODE_MAX; ++j, ++ptr)
    {
        instruction.jump_label[j] = *ptr;
        if ((ptr[0] >= 'A' && ptr[0] <= 'Z') ||
            (ptr[0] >= '0' && ptr[0] <= '9') ||
            ptr[0] == '_') continue;
        break;
    }
    instruction.jump_label[j] = '\0';
    i += j;

    return true;
}

bool compile()
{
    memset(instructions, 0, sizeof(Instruction) * PROGRAM_MAX);

    instruction_count = 0;
    auto ptr = code;
    compile_line = 0;
    for (int i = 0; i < CODE_MAX; ++i, ++ptr)
    {
        if (*ptr == '\0') break;

        // White space
        if (trim(i, ptr)) break;
        if (*ptr == '\n')
        {
            ++compile_line;
            continue;
        }

        auto& instruction = instructions[instruction_count];
        instruction.line = compile_line;

        // Comment
        if (*ptr == '#')
        {
            if (ignore_line(i, ptr)) break;
            continue;
        }

        // Instruction
        if (is_inst("INP", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "INP shouldn't have arguments"); // Throw error
            instruction.inst = INST_INP;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("OUT", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "OUT shouldn't have arguments"); // Throw error
            instruction.inst = INST_OUT;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("SET", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_value(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_SET;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("LDA", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_addr(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_LDA;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("STA", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_addr(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_STA;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("ADD", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_addr(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_ADD;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("SUB", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_addr(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_SUB;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("INC", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_addr(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_INC;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("DEC", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_addr(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_DEC;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("JMP", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_label(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_JMP;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("JPE", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_label(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_JPE;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("JPL", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_label(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_JPL;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else if (is_inst("JPG", i, ptr))
        {
            if (trim(i, ptr)) break;
            if (!parse_label(instruction, i, ptr)) return false; // Throw error
            if (trim(i, ptr)) break;
            if (*ptr != '\n' && *ptr != '#' && *ptr != '\0') return throw_error(compile_line, "Expected end of line"); // Throw error
            instruction.inst = INST_JPG;
            ++instruction_count;
            if (ignore_line(i, ptr)) break;
            continue;
        }
        else
        {
            if ((ptr[0] < 'A' || ptr[0] > 'Z') && ptr[0] != '_') return throw_error(compile_line, "Expected label"); // Throw error
            // Check for label at this line
            int j = 0;
            for (; j < LINE_MAX_CHAR && i + j < CODE_MAX; ++j)
            {
                instruction.label[j] = *ptr;
                ++ptr;
                if ((ptr[0] >= 'A' && ptr[0] <= 'Z') ||
                    (ptr[0] >= '0' && ptr[0] <= '9') ||
                     ptr[0] == '_') continue;
                if (ptr[0] == ':') // Label!
                {
                    ++j;
                    instruction.label[j] = '\0';
                    break;
                }
                else
                {
                    return throw_error(compile_line, "Expected :"); // Throw error
                }
            }
            i += j;
        }
    }

    // Add empty inst at the end
    auto& instruction = instructions[instruction_count];
    instruction.inst = INST_NOP;
    ++instruction_count;

    // Resolve jump addresses
    for (auto i = 0; i < instruction_count; ++i)
    {
        auto& instruction = instructions[i];
        if (instruction.jump_label[0] != '\0')
        {
            for (auto j = 0; j < instruction_count; ++j)
            {
                auto& other_instruction = instructions[j];
                if (strcmp(instruction.jump_label, other_instruction.label) == 0)
                {
                    instruction.addr = j;
                    break;
                }
            }
        }
    }

    return true;
}

bool resolve_addr(const Instruction& instruction, int& addr)
{
    addr = instruction.addr;
    if (instruction.ind)
    {
        if (instruction.addr < 0 || instruction.addr >= RAM_SIZE)
        {
            running = false;
            return throw_error(pc, "Out of bound address");
        }
        addr = ram[instruction.addr];
    }
    if (addr < 0 || addr >= RAM_SIZE)
    {
        return throw_error(pc, "Out of bound address");
    }
    return true;
}

bool jump(const Instruction& instruction)
{
    if (instruction.addr < 0 || instruction.addr >= instruction_count)
    {
        running = false;
        return throw_error(instruction.line, "Address out of range"); // Throw error
    }
    pc = instruction.addr;
    return true;
}

bool exe_cycle()
{
    if (!compiled)
    {
        if (!compile())
        {
            running = false;
            return false;
        }
    }

    if (pc >= instruction_count)
    {
        // End program
        running = false;
        return true;
    }

    // Execute current instruction
    auto& instruction = instructions[pc];
    int addr;
    ++cycles;
    switch (instruction.inst)
    {
    case INST_INP:
    {
        if (input_cursor >= input_count)
        {
            running = false;
            return true;
        }
        a = inputs[input_cursor++];
        break;
    }
    case INST_OUT:
    {
        if (output_count >= OUTPUT_MAX)
        {
            running = false;
            return throw_error(instruction.line, "Outputs overflow"); // Throw error
        }
        outputs[output_count++] = a;
        break;
    }
    case INST_SET:
    {
        if (IS_LETTER(instruction.addr))
        {
            auto c = TO_NUMBER(instruction.addr);
            if (c < 'A' || c > 'Z') return throw_error(instruction.line, "Invalid value. Needs to be in the range [-99,99] or [A,Z]."); // Throw error
        }
        else
        {
            if (instruction.addr < -99 || instruction.addr > 99) return throw_error(instruction.line, "Invalid value. Needs to be in the range [-99,99] or [A,Z]."); // Throw error
        }
        a = instruction.addr;
        break;
    }
    case INST_LDA:
    {
        if (!resolve_addr(instruction, addr)) return false;
        ram_cursor = addr;
        a = ram[addr];
        used_ram[addr] = true;
        break;
    }
    case INST_STA:
    {
        if (!resolve_addr(instruction, addr)) return false;
        ram_cursor = addr;
        ram[addr] = a;
        used_ram[addr] = true;
        break;
    }
    case INST_ADD:
    {
        if (!resolve_addr(instruction, addr)) return false;
        if (IS_LETTER(a) || IS_LETTER(ram[addr]))
        {
            running = false;
            return throw_error(instruction.line, "Can't add letter"); // Throw error
        }
        ram_cursor = addr;
        a += ram[addr];
        used_ram[addr] = true;
        break;
    }
    case INST_SUB:
    {
        if (!resolve_addr(instruction, addr)) return false;
        ram_cursor = addr;
        a = TO_NUMBER(a) - TO_NUMBER(ram[addr]);
        used_ram[addr] = true;
        break;
    }
    case INST_INC:
    {
        if (!resolve_addr(instruction, addr)) return false;
        if (IS_LETTER(ram[addr]))
        {
            running = false;
            return throw_error(instruction.line, "Cannot increment letter"); // Throw error
        }
        ram_cursor = addr;
        a = ++ram[addr];
        used_ram[addr] = true;
        break;
    }
    case INST_DEC:
    {
        if (!resolve_addr(instruction, addr)) return false;
        if (IS_LETTER(ram[addr]))
        {
            running = false;
            return throw_error(instruction.line, "Cannot decrement letter"); // Throw error
        }
        ram_cursor = addr;
        a = --ram[addr];
        used_ram[addr] = true;
        break;
    }
    case INST_JMP:
    {
        if (!jump(instruction)) return false;
        return true;
    }
    case INST_JPE:
    {
        if (a == 0)
        {
            if (!jump(instruction)) return false;
            return true;
        }
        break;
    }
    case INST_JPL:
    {
        if (a < 0)
        {
            if (!jump(instruction)) return false;
            return true;
        }
        break;
    }
    case INST_JPG:
    {
        if (a >= 0)
        {
            if (!jump(instruction)) return false;
            return true;
        }
        break;
    }
    case INST_NOP:
    {
        break;
    }
    }

    // Move pc to next instruction
    ++pc;
    if (pc >= instruction_count)
    {
        // End program
        running = false;
    }

    return true;
}

void do_cycle()
{
    if (exe_cycle())
    {
        if (pc >= instruction_count) pc = instruction_count - 1;
        if (pc >= 0)
        {
            auto& inst = instructions[pc];
            textEditor->SetCursorPosition({ inst.line, 0 });
        }
    }
}

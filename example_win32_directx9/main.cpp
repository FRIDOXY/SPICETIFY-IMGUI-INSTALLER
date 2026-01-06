#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include <string>

#include "gui.hpp"
#include "hashes.hpp"

#include "blur.hpp"
#include "bytes.hpp"

using namespace ImGui;

// Forcer le sous-système Windows et garder le point d'entrée main()
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

#define ALPHA    ( ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )
#define NO_ALPHA ( ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoBorder )

IDirect3DTexture9* avatar{ };
IDirect3DTexture9* bg{ };

// Data
static LPDIRECT3D9 g_pD3D = nullptr;
static LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS g_d3dpp = {};

// État de l'application Spicetify
struct SpicetifyState {
    bool spotifyMSInstalled = false;
    bool spotifyWebInstalled = false;
    bool spicetifyInstalled = false;
} g_SpicetifyState;

// Forward declarations
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Fonctions utilitaires
static bool CheckSpotifyMS() {
    return (system("powershell -WindowStyle Hidden -Command \"if (Get-AppxPackage *Spotify*) { exit 0 } else { exit 1 }\"") == 0);
}

static bool CheckSpotifyWeb() {
    return (system("powershell -WindowStyle Hidden -Command \"if (Test-Path \\\"$env:APPDATA\\Spotify\\Spotify.exe\\\") { exit 0 } else { exit 1 }\"") == 0);
}

static bool CheckSpicetify() {
    return (system("powershell -WindowStyle Hidden -Command \"if (Get-Command spicetify -ErrorAction SilentlyContinue) { exit 0 } else { exit 1 }\"") == 0);
}

static void ExecutePowerShell(const char* command) {
    std::string fullCmd = "start powershell -NoExit -NoProfile -ExecutionPolicy Bypass -Command \"" + std::string(command) + "\"";
    system(fullCmd.c_str());
}

// Main code
int main(int, char**)
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Spicetify Manager", nullptr };
    ::RegisterClassExW(&wc);
    
    // Fenêtre sans bordures (WS_POPUP au lieu de WS_OVERLAPPEDWINDOW)
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Spicetify Manager", WS_POPUP, 100, 100, 800, 600, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    io.Fonts->AddFontFromMemoryTTF(museo500_binary, sizeof museo500_binary, 14);
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromMemoryTTF(&font_awesome_binary, sizeof font_awesome_binary, 13, &icons_config, icon_ranges);
    io.Fonts->AddFontFromMemoryTTF(museo900_binary, sizeof museo900_binary, 28);

    // Vérification initiale
    g_SpicetifyState.spotifyMSInstalled = CheckSpotifyMS();
    g_SpicetifyState.spotifyWebInstalled = CheckSpotifyWeb();
    g_SpicetifyState.spicetifyInstalled = CheckSpicetify();

    // Couleur de fond transparente pour voir uniquement ImGui
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (!avatar)
            D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &esliboganet, sizeof esliboganet, 30, 30, D3DX_DEFAULT, 0,
                D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &avatar);

        // SUPPRIMÉ : L'image de fond n'est plus chargée
        // if (!bg)
        //     D3DXCreateTextureFromFileExA(g_pd3dDevice, "C:\\Windows\\Web\\Screen\\img104.jpg", 1920, 1080, D3DX_DEFAULT, 0,
        //         D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &bg);

        blur::device = g_pd3dDevice;

        static bool bools[50]{};
        static int ints[50]{};

        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        // Permettre le déplacement de la fenêtre en supprimant NoDecoration ou en ajoutant NoTitleBar + NoResize
        ImGui::Begin("Spicetify Manager", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse); {

            auto window = GetCurrentWindow();
            auto draw = window->DrawList;
            auto pos = window->Pos;
            auto size = window->Size;
            auto style = GetStyle();

            gui.m_anim = ImLerp(gui.m_anim, 1.f, 0.045f);

            SetWindowSize(ImVec2(800, 600));

            // SUPPRIMÉ : L'image de fond n'est plus affichée
            // GetBackgroundDrawList()->AddImage(bg, ImVec2(0, 0), io.DisplaySize);

            // Fond sombre pour l'interface
            GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, ImColor(15, 20, 30));

            // Titre principal
            draw->AddText(io.Fonts->Fonts[1], io.Fonts->Fonts[1]->FontSize, pos + ImVec2(170 / 2 - io.Fonts->Fonts[1]->CalcTextSizeA(io.Fonts->Fonts[1]->FontSize, FLT_MAX, 0, "SPICETIFY").x / 2 + 1, 20), gui.accent_color.to_im_color(), "SPICETIFY");
            draw->AddText(io.Fonts->Fonts[1], io.Fonts->Fonts[1]->FontSize, pos + ImVec2(170 / 2 - io.Fonts->Fonts[1]->CalcTextSizeA(io.Fonts->Fonts[1]->FontSize, FLT_MAX, 0, "SPICETIFY").x / 2, 20), GetColorU32(ImGuiCol_Text), "SPICETIFY");

            // Pied de page avec status
            draw->AddLine(pos + ImVec2(0, size.y - 50), pos + ImVec2(170, size.y - 50), GetColorU32(ImGuiCol_WindowBg, 0.5f));
            draw->AddImageRounded(avatar, pos + ImVec2(15, size.y - 40), pos + ImVec2(45, size.y - 10), ImVec2(0, 0), ImVec2(1, 1), ImColor(1.f, 1.f, 1.f, 1.f), 100);
            draw->AddText(pos + ImVec2(50, size.y - 40), gui.text.to_im_color(), "Status:");

            if (g_SpicetifyState.spotifyWebInstalled) {
                draw->AddText(pos + ImVec2(50, size.y - 25), gui.accent_color.to_im_color(), "Web OK");
            }
            else if (g_SpicetifyState.spotifyMSInstalled) {
                draw->AddText(pos + ImVec2(50, size.y - 25), ImColor(255, 150, 0), "MS Store");
            }
            else {
                draw->AddText(pos + ImVec2(50, size.y - 25), gui.text_disabled.to_im_color(), "Not installed");
            }

            // Sidebar avec onglets
            SetCursorPos(ImVec2(10, 70));
            BeginChild("##tabs", ImVec2(150, size.y - 120));

            gui.group_title("Main");
            if (gui.tab(ICON_FA_HOME, "Dashboard", gui.m_tab == 0) && gui.m_tab != 0)
                gui.m_tab = 0, gui.m_anim = 0.f;

            if (gui.tab(ICON_FA_MUSIC, "Spotify", gui.m_tab == 1) && gui.m_tab != 1)
                gui.m_tab = 1, gui.m_anim = 0.f;

            if (gui.tab(ICON_FA_PALETTE, "Spicetify", gui.m_tab == 2) && gui.m_tab != 2)
                gui.m_tab = 2, gui.m_anim = 0.f;

            Spacing(), Spacing(), Spacing();

            gui.group_title("Options");
            if (gui.tab(ICON_FA_COG, "Settings", gui.m_tab == 3) && gui.m_tab != 3)
                gui.m_tab = 3, gui.m_anim = 0.f;

            if (gui.tab(ICON_FA_QUESTION, "Help", gui.m_tab == 4) && gui.m_tab != 4)
                gui.m_tab = 4, gui.m_anim = 0.f;

            EndChild();

            // Boutons d'action
            SetCursorPos(ImVec2(190, 20));
            if (Button(ICON_FA_SYNC " Refresh", ImVec2(110, 25))) {
                g_SpicetifyState.spotifyMSInstalled = CheckSpotifyMS();
                g_SpicetifyState.spotifyWebInstalled = CheckSpotifyWeb();
                g_SpicetifyState.spicetifyInstalled = CheckSpicetify();
            }

            SameLine();
            if (Button(ICON_FA_TIMES " Quit", ImVec2(80, 25))) {
                done = true;
            }

            PushStyleVar(ImGuiStyleVar_Alpha, gui.m_anim);
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

            SetCursorPos(ImVec2(185, 81 - (5 * gui.m_anim)));
            BeginChild("##childs", ImVec2(size.x - 200, size.y - 96));

            switch (gui.m_tab) {

                // DASHBOARD
            case 0:
                gui.group_box(ICON_FA_INFO " Status", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, 180)); {
                    Text("Spotify MS Store:");
                    SameLine();
                    TextColored(g_SpicetifyState.spotifyMSInstalled ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
                        g_SpicetifyState.spotifyMSInstalled ? "YES" : "NO");

                    Text("Spotify Web:");
                    SameLine();
                    TextColored(g_SpicetifyState.spotifyWebInstalled ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
                        g_SpicetifyState.spotifyWebInstalled ? "YES" : "NO");

                    Text("Spicetify:");
                    SameLine();
                    TextColored(g_SpicetifyState.spicetifyInstalled ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
                        g_SpicetifyState.spicetifyInstalled ? "YES" : "NO");

                    Separator();
                    TextWrapped("Recommended: Use Spotify Web for Spicetify compatibility.");
                } gui.end_group_box();

                SameLine();

                gui.group_box(ICON_FA_CHECK " Quick Actions", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {
                    if (Button("Install Spotify Web", ImVec2(GetWindowWidth(), 30))) {
                        ExecutePowerShell("winget install --id Spotify.Spotify");
                    }
                    if (Button("Install Spicetify", ImVec2(GetWindowWidth(), 30))) {
                        ExecutePowerShell("iwr -useb https://raw.githubusercontent.com/spicetify/cli/main/install.ps1 | iex");
                    }
                    if (Button("Open Discord", ImVec2(GetWindowWidth(), 30))) {
                        system("start https://discord.gg/yjxJCXCBMV");
                    }
                } gui.end_group_box();

                break;

                // SPOTIFY
            case 1:
                gui.group_box(ICON_FA_DOWNLOAD " Install", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() / 2 - GetStyle().ItemSpacing.y / 2)); {
                    if (Button(ICON_FA_CLOUD_DOWNLOAD " Install Web Version", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("winget install --id Spotify.Spotify");
                    }
                    if (Button(ICON_FA_EXCHANGE " Migrate to Web", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("Get-AppxPackage *Spotify* | Remove-AppxPackage; winget install --id Spotify.Spotify");
                    }
                    TextWrapped("Web version required for Spicetify modifications.");
                } gui.end_group_box();

                gui.group_box(ICON_FA_TRASH " Uninstall", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight() / 2 - GetStyle().ItemSpacing.y / 2)); {
                    if (Button(ICON_FA_TIMES_CIRCLE " Remove MS Store", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("Get-AppxPackage *Spotify* | Remove-AppxPackage");
                    }
                    if (Button(ICON_FA_TIMES_CIRCLE " Remove Web", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("winget uninstall --id Spotify.Spotify");
                    }
                } gui.end_group_box();

                SameLine(), SetCursorPosY(0);

                gui.group_box(ICON_FA_INFO " Information", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {
                    TextWrapped("Spotify Web is the recommended version for customization with Spicetify.");
                    Separator();
                    Text("Features:");
                    BulletText("Full Spicetify support");
                    BulletText("Theme customization");
                    BulletText("Extension support");
                    BulletText("Regular updates");
                } gui.end_group_box();

                break;

                // SPICETIFY
            case 2:
                gui.group_box(ICON_FA_TOOLS " Manage", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {
                    if (Button(ICON_FA_DOWNLOAD " Install Spicetify", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("iwr -useb https://raw.githubusercontent.com/spicetify/cli/main/install.ps1 | iex");
                    }
                    if (Button(ICON_FA_SYNC " Update Spicetify", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("spicetify update");
                    }
                    if (Button(ICON_FA_CHECK " Apply Changes", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("spicetify apply");
                    }
                    if (Button(ICON_FA_UNDO " Restore Spotify", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("spicetify restore");
                    }
                    if (Button(ICON_FA_FOLDER_OPEN " Config Folder", ImVec2(GetWindowWidth(), 35))) {
                        ExecutePowerShell("cd $env:USERPROFILE\\.spicetify; explorer .");
                    }
                } gui.end_group_box();

                SameLine();

                gui.group_box(ICON_FA_BOOK " Resources", ImVec2(GetWindowWidth() / 2 - GetStyle().ItemSpacing.x / 2, GetWindowHeight())); {
                    TextWrapped("Customize your Spotify experience with themes and extensions.");
                    Separator();
                    Text("Requirements:");
                    BulletText("Spotify Web");
                    BulletText("PowerShell");
                    Separator();
                    if (Button(ICON_FA_CODE " GitHub Repo", ImVec2(GetWindowWidth(), 30))) {
                        system("start https://github.com/spicetify/cli");
                    }
                    if (Button(ICON_FA_SHOPPING_CART " Marketplace", ImVec2(GetWindowWidth(), 30))) {
                        system("start https://github.com/spicetify/marketplace");
                    }
                } gui.end_group_box();

                break;

                // SETTINGS
            case 3:
                gui.group_box(ICON_FA_SLIDERS_H " Preferences", ImVec2(GetWindowWidth(), GetWindowHeight())); {
                    Checkbox("Always on top", &bools[0]);
                    Checkbox("Enable notifications", &bools[1]);
                    Checkbox("Auto-refresh on startup", &bools[2]);
                    Separator();
                    if (Button(ICON_FA_SAVE " Save Settings", ImVec2(200, 30))) {
                        // Save logic here
                    }
                } gui.end_group_box();
                break;

                // HELP
            case 4:
                gui.group_box(ICON_FA_QUESTION_CIRCLE " Help & Support", ImVec2(GetWindowWidth(), GetWindowHeight())); {
                    if (Button(ICON_FA_COMMENTS " Join Discord", ImVec2(GetWindowWidth(), 35))) {
                        system("start https://discord.com/channels/836135567518466078/1455388464114831380");
                    }
                    if (Button(ICON_FA_BOOK " Documentation", ImVec2(GetWindowWidth(), 35))) {
                        system("start https://github.com/FRIDOXY/SPICETIFY-IMGUI-INSTALLER");
                    }
                    Separator();
                    Text("Quick Guide:");
                    BulletText("1. Install Spotify Web");
                    BulletText("2. Install Spicetify");
                    BulletText("3. Apply customizations");
                    BulletText("4. Enjoy your custom Spotify!");
                } gui.end_group_box();
                break;
            }

            EndChild();

            PopStyleVar(2);

        } ImGui::End();

        PopStyleVar();

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

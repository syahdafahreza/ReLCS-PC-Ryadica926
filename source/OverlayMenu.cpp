#include "OverlayMenu.h"
#include "Font8x8.h"

namespace ReLCS
{
    bool OverlayMenu::s_bVisible = false;
    bool OverlayMenu::s_bShowFps = false;
    int OverlayMenu::s_activeCategory = 0;
    int OverlayMenu::s_selectedIndex = 0;
    float OverlayMenu::s_currentFps = 0.0f;
    int OverlayMenu::s_frameCount = 0;
    std::chrono::steady_clock::time_point OverlayMenu::s_lastFpsTime = std::chrono::steady_clock::now();

    IDirect3DTexture8* OverlayMenu::s_pFontTexture = nullptr;
    std::vector<std::string> OverlayMenu::s_categories = { "PLAYER", "VEHICLE", "WORLD", "VISUAL/GAME" };

    void OverlayMenu::Init()
    {
        s_bVisible = false;
        s_bShowFps = false;
        s_activeCategory = 0;
        s_selectedIndex = 0;
        s_lastFpsTime = std::chrono::steady_clock::now();
    }

    void OverlayMenu::CreateFontTexture(IDirect3DDevice8* pDevice)
    {
        if (s_pFontTexture || !pDevice)
            return;

        // 128x64 texture fits 16 columns x 6 rows = 96 glyphs (8x8 each)
        if (FAILED(pDevice->CreateTexture(128, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &s_pFontTexture)))
            return;

        D3DLOCKED_RECT lr;
        if (FAILED(s_pFontTexture->LockRect(0, &lr, nullptr, 0)))
            return;

        uint32_t* pBits = reinterpret_cast<uint32_t*>(lr.pBits);
        int pitchElements = lr.Pitch / 4;

        // Clear texture
        for (int y = 0; y < 64; y++)
        {
            for (int x = 0; x < 128; x++)
            {
                pBits[y * pitchElements + x] = 0x00000000;
            }
        }

        // Draw 96 glyphs
        for (int i = 0; i < 96; i++)
        {
            int col = i % 16;
            int row = i / 16;
            int startX = col * 8;
            int startY = row * 8;

            for (int py = 0; py < 8; py++)
            {
                uint8_t byteRow = g_font8x8_basic[i][py];
                for (int px = 0; px < 8; px++)
                {
                    if ((byteRow >> px) & 1)
                    {
                        pBits[(startY + py) * pitchElements + (startX + px)] = 0xFFFFFFFF;
                    }
                }
            }
        }

        s_pFontTexture->UnlockRect(0);
    }

    void OverlayMenu::OnLostDevice()
    {
        if (s_pFontTexture)
        {
            s_pFontTexture->Release();
            s_pFontTexture = nullptr;
        }
    }

    void OverlayMenu::OnResetDevice(IDirect3DDevice8* pDevice)
    {
        CreateFontTexture(pDevice);
    }

    void OverlayMenu::HandleInput()
    {
        static bool lastF9 = false;
        static bool lastF10 = false;
        static bool lastUp = false;
        static bool lastDown = false;
        static bool lastLeft = false;
        static bool lastRight = false;
        static bool lastEnter = false;
        static bool lastEsc = false;

        bool currF9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        bool currF10 = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;

        if (currF9 && !lastF9)
            s_bVisible = !s_bVisible;

        if (currF10 && !lastF10)
            s_bShowFps = !s_bShowFps;

        lastF9 = currF9;
        lastF10 = currF10;

        if (!s_bVisible)
            return;

        bool currUp = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
        bool currDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
        bool currLeft = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
        bool currRight = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
        bool currEnter = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
        bool currEsc = ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) || (GetAsyncKeyState(VK_BACK) & 0x8000));

        // Get cheats for current category
        std::vector<CheatInfo> currentCategoryCheats;
        std::string currentCatName = s_categories[s_activeCategory];
        for (const auto& cheat : CheatManager::GetCheats())
        {
            if (currentCatName == "VISUAL/GAME" && cheat.category == "Visual/Gameplay")
                currentCategoryCheats.push_back(cheat);
            else if (cheat.category == currentCatName)
                currentCategoryCheats.push_back(cheat);
        }

        int totalItems = static_cast<int>(currentCategoryCheats.size());
        if (s_activeCategory == 3) // Add FPS counter toggle as extra item in Visual/Game
            totalItems += 1;

        if (currLeft && !lastLeft)
        {
            s_activeCategory = (s_activeCategory - 1 + 4) % 4;
            s_selectedIndex = 0;
        }
        else if (currRight && !lastRight)
        {
            s_activeCategory = (s_activeCategory + 1) % 4;
            s_selectedIndex = 0;
        }

        if (currUp && !lastUp)
        {
            s_selectedIndex--;
            if (s_selectedIndex < 0)
                s_selectedIndex = totalItems - 1;
        }
        else if (currDown && !lastDown)
        {
            s_selectedIndex++;
            if (s_selectedIndex >= totalItems)
                s_selectedIndex = 0;
        }

        if (currEnter && !lastEnter)
        {
            if (s_activeCategory == 3 && s_selectedIndex == totalItems - 1)
            {
                s_bShowFps = !s_bShowFps;
            }
            else if (s_selectedIndex < static_cast<int>(currentCategoryCheats.size()))
            {
                CheatManager::ActivateCheat(currentCategoryCheats[s_selectedIndex].id);
            }
        }

        if (currEsc && !lastEsc)
        {
            s_bVisible = false;
        }

        lastUp = currUp;
        lastDown = currDown;
        lastLeft = currLeft;
        lastRight = currRight;
        lastEnter = currEnter;
        lastEsc = currEsc;
    }

    void OverlayMenu::DrawRect(IDirect3DDevice8* pDevice, float x, float y, float w, float h, DWORD color)
    {
        D3DVertex2D vertices[4] = {
            { x,     y,     0.0f, 1.0f, color, 0.0f, 0.0f },
            { x + w, y,     0.0f, 1.0f, color, 0.0f, 0.0f },
            { x,     y + h, 0.0f, 1.0f, color, 0.0f, 0.0f },
            { x + w, y + h, 0.0f, 1.0f, color, 0.0f, 0.0f },
        };

        pDevice->SetTexture(0, nullptr);
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);

        pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(D3DVertex2D));
    }

    void OverlayMenu::DrawTextString(IDirect3DDevice8* pDevice, float x, float y, const std::string& text, DWORD color, float scale)
    {
        if (!s_pFontTexture)
            CreateFontTexture(pDevice);
        if (!s_pFontTexture)
            return;

        pDevice->SetTexture(0, s_pFontTexture);
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

        float curX = x;
        float charW = 8.0f * scale;
        float charH = 8.0f * scale;

        for (char c : text)
        {
            if (c < 32 || c > 127)
                c = '?';

            int idx = c - 32;
            int col = idx % 16;
            int row = idx / 16;

            float u0 = (col * 8.0f) / 128.0f;
            float v0 = (row * 8.0f) / 64.0f;
            float u1 = ((col + 1) * 8.0f) / 128.0f;
            float v1 = ((row + 1) * 8.0f) / 64.0f;

            D3DVertex2D verts[4] = {
                { curX,         y,         0.0f, 1.0f, color, u0, v0 },
                { curX + charW, y,         0.0f, 1.0f, color, u1, v0 },
                { curX,         y + charH, 0.0f, 1.0f, color, u0, v1 },
                { curX + charW, y + charH, 0.0f, 1.0f, color, u1, v1 },
            };

            pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, verts, sizeof(D3DVertex2D));
            curX += charW + (1.0f * scale);
        }
    }

    void OverlayMenu::UpdateFps()
    {
        s_frameCount++;
        auto now = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastFpsTime).count();
        if (ms >= 500)
        {
            s_currentFps = (s_frameCount * 1000.0f) / ms;
            s_frameCount = 0;
            s_lastFpsTime = now;
        }
    }

    void OverlayMenu::Render(IDirect3DDevice8* pDevice)
    {
        if (!pDevice)
            return;

        UpdateFps();
        HandleInput();
        CheatManager::Update();

        // Backup render states
        DWORD zEnable, alphaBlend, srcBlend, destBlend, lighting, cullMode, fogEnable;
        pDevice->GetRenderState(D3DRS_ZENABLE, &zEnable);
        pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
        pDevice->GetRenderState(D3DRS_SRCBLEND, &srcBlend);
        pDevice->GetRenderState(D3DRS_DESTBLEND, &destBlend);
        pDevice->GetRenderState(D3DRS_LIGHTING, &lighting);
        pDevice->GetRenderState(D3DRS_CULLMODE, &cullMode);
        pDevice->GetRenderState(D3DRS_FOGENABLE, &fogEnable);

        pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        pDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);

        pDevice->SetVertexShader(D3DFVF_VERTEX2D);

        // 1. Draw FPS Counter (if enabled)
        if (s_bShowFps)
        {
            char fpsBuf[32];
            snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", s_currentFps);
            DrawRect(pDevice, 10.0f, 10.0f, 90.0f, 22.0f, 0xAA000000);
            DrawTextString(pDevice, 16.0f, 15.0f, fpsBuf, 0xFF10B981, 1.2f);
        }

        // 2. Draw Cheat Activation Toast Banner
        float notifAlpha = CheatManager::GetNotificationAlpha();
        if (notifAlpha > 0.01f)
        {
            DWORD a = static_cast<DWORD>(notifAlpha * 255.0f);
            DWORD bannerBg = (a << 24) | 0x001E293B; // Slate background
            DWORD bannerText = (a << 24) | 0x00F8FAFC;
            DWORD bannerAccent = (a << 24) | 0x003B82F6; // Blue border

            std::string notifStr = "[ CHEAT ACTIVATED: " + CheatManager::GetLastActivatedCheatName() + " ]";
            float bannerW = (notifStr.length() * 10.0f) + 40.0f;
            float bannerX = 400.0f - (bannerW / 2.0f);
            if (bannerX < 20.0f) bannerX = 20.0f;

            DrawRect(pDevice, bannerX - 2.0f, 28.0f, bannerW + 4.0f, 32.0f, bannerAccent);
            DrawRect(pDevice, bannerX, 30.0f, bannerW, 28.0f, bannerBg);
            DrawTextString(pDevice, bannerX + 20.0f, 38.0f, notifStr, bannerText, 1.25f);
        }

        // 3. Draw Menu Window (if visible)
        if (s_bVisible)
        {
            const float menuX = 35.0f;
            const float menuY = 45.0f;
            const float menuW = 460.0f;
            const float menuH = 430.0f;

            // Outer border and main panel
            DrawRect(pDevice, menuX - 2.0f, menuY - 2.0f, menuW + 4.0f, menuH + 4.0f, 0xFF3B82F6);
            DrawRect(pDevice, menuX, menuY, menuW, menuH, 0xEE0F172A);

            // Header bar
            DrawRect(pDevice, menuX, menuY, menuW, 36.0f, 0xFF1E293B);
            DrawTextString(pDevice, menuX + 16.0f, menuY + 11.0f, "ReLCS-Cleo v1.0", 0xFF60A5FA, 1.5f);
            DrawTextString(pDevice, menuX + menuW - 130.0f, menuY + 13.0f, "[F9] HIDE", 0xFF94A3B8, 1.1f);

            // Category Tab Bar
            float tabY = menuY + 40.0f;
            float tabW = menuW / 4.0f;
            for (int i = 0; i < 4; i++)
            {
                float tX = menuX + (i * tabW);
                bool isActive = (i == s_activeCategory);
                DWORD tabBg = isActive ? 0xFF2563EB : 0xFF1E293B;
                DWORD tabTextColor = isActive ? 0xFFFFFFFF : 0xFF94A3B8;

                DrawRect(pDevice, tX + 2.0f, tabY, tabW - 4.0f, 24.0f, tabBg);
                DrawTextString(pDevice, tX + 8.0f, tabY + 7.0f, s_categories[i], tabTextColor, 1.0f);
            }

            // Items list
            std::vector<CheatInfo> curCheats;
            std::string curCat = s_categories[s_activeCategory];
            for (const auto& c : CheatManager::GetCheats())
            {
                if (curCat == "VISUAL/GAME" && c.category == "Visual/Gameplay")
                    curCheats.push_back(c);
                else if (c.category == curCat)
                    curCheats.push_back(c);
            }

            int totalRows = static_cast<int>(curCheats.size());
            if (s_activeCategory == 3)
                totalRows += 1;

            float itemStartY = tabY + 34.0f;
            float rowH = 26.0f;

            for (int i = 0; i < totalRows; i++)
            {
                float rowY = itemStartY + (i * rowH);
                bool isSelected = (i == s_selectedIndex);

                if (isSelected)
                {
                    DrawRect(pDevice, menuX + 8.0f, rowY - 2.0f, menuW - 16.0f, rowH - 2.0f, 0xFF1D4ED8);
                }
                else if (i % 2 == 1)
                {
                    DrawRect(pDevice, menuX + 8.0f, rowY - 2.0f, menuW - 16.0f, rowH - 2.0f, 0x22FFFFFF);
                }

                if (s_activeCategory == 3 && i == totalRows - 1)
                {
                    // FPS counter item
                    DrawTextString(pDevice, menuX + 20.0f, rowY + 3.0f, "Toggle FPS Counter", isSelected ? 0xFFFFFFFF : 0xFFE2E8F0, 1.2f);
                    std::string stateTag = s_bShowFps ? "[ON]" : "[OFF]";
                    DrawTextString(pDevice, menuX + menuW - 75.0f, rowY + 3.0f, stateTag, s_bShowFps ? 0xFF10B981 : 0xFFEF4444, 1.2f);
                }
                else
                {
                    const auto& item = curCheats[i];
                    DrawTextString(pDevice, menuX + 20.0f, rowY + 3.0f, item.name, isSelected ? 0xFFFFFFFF : 0xFFE2E8F0, 1.2f);

                    // Status / Code tag
                    std::string tag = "";
                    DWORD tagColor = 0xFF60A5FA;

                    if (item.id == LCSCheat::NeverWanted)
                    {
                        tag = CheatManager::IsNeverWantedEnabled() ? "[ACTIVE]" : "[OFF]";
                        tagColor = CheatManager::IsNeverWantedEnabled() ? 0xFF10B981 : 0xFF94A3B8;
                    }
                    else if (item.id == LCSCheat::UpsideDownMode)
                    {
                        tag = CheatManager::IsUpsideDownEnabled() ? "[ACTIVE]" : "";
                        tagColor = 0xFFF59E0B;
                    }

                    if (!tag.empty())
                    {
                        DrawTextString(pDevice, menuX + menuW - 85.0f, rowY + 3.0f, tag, tagColor, 1.1f);
                    }
                }
            }

            // Footer instructions bar
            float footerY = menuY + menuH - 30.0f;
            DrawRect(pDevice, menuX, footerY, menuW, 30.0f, 0xFF0F172A);
            DrawTextString(pDevice, menuX + 16.0f, footerY + 8.0f,
                "[UP/DN] Navigate  [LEFT/RT] Tab  [ENTER] Apply  [ESC] Close", 0xFF94A3B8, 1.05f);
        }

        // Restore render states
        pDevice->SetRenderState(D3DRS_ZENABLE, zEnable);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
        pDevice->SetRenderState(D3DRS_SRCBLEND, srcBlend);
        pDevice->SetRenderState(D3DRS_DESTBLEND, destBlend);
        pDevice->SetRenderState(D3DRS_LIGHTING, lighting);
        pDevice->SetRenderState(D3DRS_CULLMODE, cullMode);
        pDevice->SetRenderState(D3DRS_FOGENABLE, fogEnable);
    }
}

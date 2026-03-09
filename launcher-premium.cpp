#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <winhttp.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <algorithm>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#include <dwmapi.h>
#include <uxtheme.h>
#include <windowsx.h>
#include <cmath>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace std;
using namespace Gdiplus;

// ═══════════════════════════════════════════════════════════════════════════════
// DESIGN SYSTEM - Premium Gaming Aesthetic
// ═══════════════════════════════════════════════════════════════════════════════

namespace Theme {
    // Core colors - Deep space dark theme
    namespace Bg {
        const Color Base(255, 8, 10, 15);           // Deepest black
        const Color Surface(255, 15, 18, 25);       // Main surface
        const Color Elevated(255, 22, 26, 35);      // Cards, panels
        const Color Overlay(255, 30, 35, 45);       // Hover states
        const Color Glass(180, 20, 24, 32);         // Glass panels
    }
    
    // Accent gradient system
    namespace Accent {
        const Color Primary(255, 99, 102, 241);     // Indigo
        const Color Secondary(255, 139, 92, 246);   // Purple  
        const Color Tertiary(255, 236, 72, 153);    // Pink
        const Color Success(255, 16, 185, 129);     // Emerald
        const Color Warning(255, 245, 158, 11);     // Amber
        const Color Cyan(255, 6, 182, 212);         // Cyan highlight
    }
    
    // Text hierarchy
    namespace Text {
        const Color Primary(255, 248, 250, 252);    // Pure white
        const Color Secondary(255, 148, 163, 184);  // Slate 400
        const Color Muted(255, 100, 116, 139);      // Slate 500
        const Color Disabled(255, 71, 85, 105);     // Slate 600
    }
    
    // Borders & dividers
    namespace Border {
        const Color Default(255, 51, 65, 85);       // Slate 700
        const Color Light(255, 71, 85, 105);        // Slate 600
        const Color Accent(80, 99, 102, 241);       // Indigo with alpha
    }
    
    // Spacing system (4px base)
    namespace Space {
        const int XS = 4;
        const int SM = 8;
        const int MD = 16;
        const int LG = 24;
        const int XL = 32;
        const int XXL = 48;
    }
    
    // Border radius
    namespace Radius {
        const int SM = 6;
        const int MD = 10;
        const int LG = 14;
        const int XL = 20;
        const int Full = 9999;
    }
    
    // Shadows
    namespace Shadow {
        const int SM = 4;
        const int MD = 8;
        const int LG = 16;
        const int XL = 24;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// DATA STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════════

struct Game {
    wstring name;
    wstring displayName;
    wstring exePath;
    wstring imagePath;
    bool hasImage;
    int playCount;      // For sorting by popularity
    DWORD lastPlayed;   // For recent sorting
};

// App state
vector<Game> games;
vector<Game> filteredGames;
HWND hMainWnd, hListArea, hPreviewArea, hSearchEdit;
HWND hPlayBtn, hRefreshBtn, hSettingsBtn;
HFONT hFontDisplay, hFontHeading, hFontBody, hFontSmall, hFontMono;
ULONG_PTR gdiplusToken;
wstring gamesFolder = L"E:\\games";
wstring cacheFolder;
wstring localImagesFolder = L"C:\\Users\\micha\\.openclaw\\workspace-moltbot\\game-library-manager-web\\public\\images";
int selectedIndex = -1;
int hoveredItem = -1;
int listScrollY = 0;
int itemHeight = 72;
bool searchFocused = false;
HCURSOR hHandCursor;

// Animation state
float hoverAnimProgress[1000] = {0};  // Per-item hover animation
UINT_PTR animTimerId = 0;

// ═══════════════════════════════════════════════════════════════════════════════
// DRAWING HELPERS - Premium quality
// ═══════════════════════════════════════════════════════════════════════════════

// Ease-out cubic for smooth animations
float EaseOutCubic(float t) {
    return 1 - powf(1 - t, 3);
}

// Draw premium rounded rectangle with optional gradient fill
void FillRoundedRect(Graphics& g, int x, int y, int w, int h, int r, const Brush* brush) {
    GraphicsPath path;
    int d = r * 2;
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + w - d, y, d, d, 270, 90);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.CloseFigure();
    g.FillPath(brush, &path);
}

void DrawRoundedRect(Graphics& g, int x, int y, int w, int h, int r, const Pen* pen) {
    GraphicsPath path;
    int d = r * 2;
    path.AddArc(x, y, d, d, 180, 90);
    path.AddArc(x + w - d, y, d, d, 270, 90);
    path.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    path.AddArc(x, y + h - d, d, d, 90, 90);
    path.CloseFigure();
    g.DrawPath(pen, &path);
}

// Premium soft shadow with blur simulation
void DrawSoftShadow(Graphics& g, int x, int y, int w, int h, int r, int blur, int offsetY = 4) {
    for (int i = blur; i > 0; i -= 2) {
        int alpha = max(0, 35 - (i * 3));
        SolidBrush shadowBrush(Color(alpha, 0, 0, 0));
        FillRoundedRect(g, x - i/2, y + offsetY + i/3, w + i, h + i/2, r + i/3, &shadowBrush);
    }
}

// Glass panel effect
void DrawGlassPanel(Graphics& g, int x, int y, int w, int h, int r, bool elevated = false) {
    // Outer shadow
    DrawSoftShadow(g, x, y, w, h, r, elevated ? Theme::Shadow::LG : Theme::Shadow::MD, elevated ? 8 : 4);
    
    // Glass background with noise-like gradient
    LinearGradientBrush glassBg(
        Point(x, y), Point(x, y + h),
        Color(elevated ? 220 : 200, 22, 26, 35),
        Color(elevated ? 240 : 220, 15, 18, 25)
    );
    FillRoundedRect(g, x, y, w, h, r, &glassBg);
    
    // Inner top highlight (simulates light from above)
    LinearGradientBrush topShine(
        Point(x, y), Point(x, y + 40),
        Color(20, 255, 255, 255),
        Color(0, 255, 255, 255)
    );
    GraphicsPath shinePath;
    shinePath.AddArc(x, y, r * 2, r * 2, 180, 90);
    shinePath.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    shinePath.AddLine(x + w - r, y + r, x + w - r, y + 35);
    shinePath.AddLine(x + w - r, y + 35, x + r, y + 35);
    shinePath.AddLine(x + r, y + 35, x + r, y + r);
    shinePath.CloseFigure();
    g.FillPath(&topShine, &shinePath);
    
    // Subtle border with gradient
    Pen borderPen(Color(40, 255, 255, 255), 1);
    DrawRoundedRect(g, x, y, w, h, r, &borderPen);
}

// Premium gradient button
void DrawPremiumButton(Graphics& g, int x, int y, int w, int h, const wchar_t* text, 
                       bool isPrimary, bool isHovered, bool isPressed) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int r = Theme::Radius::LG;
    
    if (isPrimary) {
        // Glow effect
        if (isHovered) {
            for (int i = 12; i > 0; i -= 2) {
                int alpha = 50 - i * 4;
                SolidBrush glowBrush(Color(max(0, alpha), 99, 102, 241));
                FillRoundedRect(g, x - i, y - i/2, w + i * 2, h + i, r + i/2, &glowBrush);
            }
        } else {
            DrawSoftShadow(g, x, y, w, h, r, Theme::Shadow::MD);
        }
        
        // Main gradient - horizontal for modern look
        LinearGradientBrush mainGrad(
            Point(x, y), Point(x + w, y),
            isPressed ? Color(255, 79, 82, 221) : (isHovered ? Color(255, 129, 132, 255) : Theme::Accent::Primary),
            isPressed ? Color(255, 119, 72, 226) : (isHovered ? Color(255, 169, 122, 255) : Theme::Accent::Secondary)
        );
        FillRoundedRect(g, x, y, w, h, r, &mainGrad);
        
        // Top shine
        LinearGradientBrush shine(
            Point(x, y), Point(x, y + h/2),
            Color(isPressed ? 20 : 40, 255, 255, 255),
            Color(0, 255, 255, 255)
        );
        GraphicsPath topPath;
        topPath.AddArc(x + 2, y + 2, (r - 2) * 2, (r - 2) * 2, 180, 90);
        topPath.AddArc(x + w - (r - 2) * 2 - 2, y + 2, (r - 2) * 2, (r - 2) * 2, 270, 90);
        topPath.AddLine(x + w - 4, y + r, x + w - 4, y + h/3);
        topPath.AddLine(x + w - 4, y + h/3, x + 4, y + h/3);
        topPath.AddLine(x + 4, y + h/3, x + 4, y + r);
        topPath.CloseFigure();
        g.FillPath(&shine, &topPath);
        
    } else {
        // Secondary button
        DrawSoftShadow(g, x, y, w, h, r, Theme::Shadow::SM);
        
        SolidBrush bgBrush(isPressed ? Theme::Bg::Surface : (isHovered ? Theme::Bg::Overlay : Theme::Bg::Elevated));
        FillRoundedRect(g, x, y, w, h, r, &bgBrush);
        
        Pen borderPen(isHovered ? Color(100, 99, 102, 241) : Theme::Border::Default, 1.5f);
        DrawRoundedRect(g, x, y, w, h, r, &borderPen);
    }
    
    // Text with subtle shadow
    Font font(L"Segoe UI Semibold", 11);
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentCenter);
    fmt.SetLineAlignment(StringAlignmentCenter);
    
    if (isPrimary) {
        SolidBrush shadowBrush(Color(50, 0, 0, 0));
        RectF shadowRect((REAL)(x + 1), (REAL)(y + 1), (REAL)w, (REAL)h);
        g.DrawString(text, -1, &font, shadowRect, &fmt, &shadowBrush);
    }
    
    SolidBrush textBrush(Theme::Text::Primary);
    RectF textRect((REAL)x, (REAL)y, (REAL)w, (REAL)h);
    g.DrawString(text, -1, &font, textRect, &fmt, &textBrush);
}

// Premium search box
void DrawSearchBox(Graphics& g, int x, int y, int w, int h, bool focused, const wchar_t* placeholder) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    
    int r = Theme::Radius::LG;
    
    // Glow when focused
    if (focused) {
        for (int i = 8; i > 0; i -= 2) {
            int alpha = 40 - i * 5;
            SolidBrush glowBrush(Color(max(0, alpha), 99, 102, 241));
            FillRoundedRect(g, x - i, y - i/2, w + i * 2, h + i, r + i/2, &glowBrush);
        }
    } else {
        DrawSoftShadow(g, x, y, w, h, r, Theme::Shadow::SM, 2);
    }
    
    // Background
    SolidBrush bgBrush(Theme::Bg::Elevated);
    FillRoundedRect(g, x, y, w, h, r, &bgBrush);
    
    // Inner shadow (inset effect)
    LinearGradientBrush innerShadow(
        Point(x, y), Point(x, y + 10),
        Color(15, 0, 0, 0),
        Color(0, 0, 0, 0)
    );
    FillRoundedRect(g, x + 2, y + 2, w - 4, 8, r - 2, &innerShadow);
    
    // Border
    Pen borderPen(focused ? Theme::Accent::Primary : Theme::Border::Default, focused ? 2.0f : 1.0f);
    DrawRoundedRect(g, x, y, w, h, r, &borderPen);
    
    // Search icon
    Font iconFont(L"Segoe UI Symbol", 13);
    SolidBrush iconBrush(focused ? Theme::Accent::Primary : Theme::Text::Muted);
    g.DrawString(L"🔍", -1, &iconFont, PointF((REAL)(x + 14), (REAL)(y + h/2 - 11)), &iconBrush);
}

// ═══════════════════════════════════════════════════════════════════════════════
// UI COMPONENTS
// ═══════════════════════════════════════════════════════════════════════════════

// Premium header with gradient accent
void DrawHeader(Graphics& g, int width, int height) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    // Subtle gradient background
    LinearGradientBrush headerBg(
        Point(0, 0), Point(0, height),
        Color(255, 12, 15, 22),
        Theme::Bg::Surface
    );
    g.FillRectangle(&headerBg, 0, 0, width, height);
    
    // Premium gradient accent bar (4px)
    LinearGradientBrush accentBar(
        Point(0, 0), Point(width, 0),
        Theme::Accent::Primary,
        Theme::Accent::Tertiary
    );
    g.FillRectangle(&accentBar, 0, 0, width, 4);
    
    // Glow under accent
    for (int i = 0; i < 12; i++) {
        int alpha = 25 - i * 2;
        LinearGradientBrush glow(
            Point(0, 4 + i), Point(width, 4 + i),
            Color(max(0, alpha), 99, 102, 241),
            Color(max(0, alpha), 236, 72, 153)
        );
        g.FillRectangle(&glow, 0, 4 + i, width, 1);
    }
    
    // Logo/Icon area with glow
    int logoX = 28, logoY = 28;
    
    // Icon glow
    for (int i = 6; i > 0; i--) {
        SolidBrush iconGlow(Color(20 - i * 3, 99, 102, 241));
        g.FillEllipse(&iconGlow, logoX - i, logoY - i, 40 + i * 2, 40 + i * 2);
    }
    
    // Icon background
    LinearGradientBrush iconBg(
        Point(logoX, logoY), Point(logoX, logoY + 40),
        Color(255, 35, 40, 52),
        Color(255, 25, 30, 42)
    );
    g.FillEllipse(&iconBg, logoX, logoY, 40, 40);
    
    // Icon
    Font iconFont(L"Segoe UI Emoji", 18);
    SolidBrush iconBrush(Theme::Accent::Primary);
    StringFormat iconFmt;
    iconFmt.SetAlignment(StringAlignmentCenter);
    iconFmt.SetLineAlignment(StringAlignmentCenter);
    RectF iconRect((REAL)logoX, (REAL)logoY, 40, 40);
    g.DrawString(L"🎮", -1, &iconFont, iconRect, &iconFmt, &iconBrush);
    
    // Title
    Font titleFont(L"Segoe UI", 22, FontStyleBold);
    SolidBrush titleBrush(Theme::Text::Primary);
    g.DrawString(L"Game Library", -1, &titleFont, PointF(82, 30), &titleBrush);
    
    // Subtitle with accent
    Font subFont(L"Segoe UI", 10);
    SolidBrush subBrush(Theme::Text::Muted);
    wstring subText = to_wstring(games.size()) + L" games in collection";
    g.DrawString(subText.c_str(), -1, &subFont, PointF(84, 56), &subBrush);
    
    // Accent dot
    SolidBrush dotBrush(Theme::Accent::Success);
    g.FillEllipse(&dotBrush, 82, 62, 6, 6);
}

// Premium game card
void DrawGameCard(Graphics& g, int x, int y, int w, int h, const Game& game, 
                  bool isSelected, bool isHovered, int index, float animProgress) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int r = Theme::Radius::MD;
    int padding = 12;
    int thumbSize = h - 24;
    int thumbX = x + padding + 6;
    int thumbY = y + 12;
    
    // Animated transform for hover
    float scale = 1.0f + animProgress * 0.01f;
    int offsetX = (int)(animProgress * 3);
    
    // Selection glow
    if (isSelected) {
        for (int i = 10; i > 0; i -= 2) {
            int alpha = 35 - i * 3;
            SolidBrush selectGlow(Color(max(0, alpha), 99, 102, 241));
            FillRoundedRect(g, x + 4 - i + offsetX, y + 3 - i/2, w - 8 + i * 2, h - 6 + i, r + i/2, &selectGlow);
        }
    }
    
    // Card background
    if (isSelected) {
        LinearGradientBrush selectBg(
            Point(x, y), Point(x + w, y + h),
            Color(50, 99, 102, 241),
            Color(30, 139, 92, 246)
        );
        FillRoundedRect(g, x + 4 + offsetX, y + 3, w - 8, h - 6, r, &selectBg);
        
        // Left accent bar
        LinearGradientBrush accentBar(
            Point(x + 4, y + 10), Point(x + 4, y + h - 10),
            Theme::Accent::Primary,
            Theme::Accent::Secondary
        );
        g.FillRectangle(&accentBar, x + 4 + offsetX, y + 14, 3, h - 28);
        
    } else if (isHovered) {
        SolidBrush hoverBg(Color(255, 28, 32, 42));
        FillRoundedRect(g, x + 4 + offsetX, y + 3, w - 8, h - 6, r, &hoverBg);
        
        // Subtle border on hover
        Pen hoverBorder(Color(40, 99, 102, 241), 1);
        DrawRoundedRect(g, x + 4 + offsetX, y + 3, w - 8, h - 6, r, &hoverBorder);
    }
    
    // Thumbnail with premium styling
    thumbX += offsetX;
    
    // Thumbnail shadow
    if (isSelected || isHovered) {
        DrawSoftShadow(g, thumbX - 2, thumbY - 2, thumbSize + 4, thumbSize + 4, 8, 6, 3);
    }
    
    // Thumbnail container
    GraphicsPath thumbPath;
    int thumbR = 8;
    thumbPath.AddArc(thumbX, thumbY, thumbR * 2, thumbR * 2, 180, 90);
    thumbPath.AddArc(thumbX + thumbSize - thumbR * 2, thumbY, thumbR * 2, thumbR * 2, 270, 90);
    thumbPath.AddArc(thumbX + thumbSize - thumbR * 2, thumbY + thumbSize - thumbR * 2, thumbR * 2, thumbR * 2, 0, 90);
    thumbPath.AddArc(thumbX, thumbY + thumbSize - thumbR * 2, thumbR * 2, thumbR * 2, 90, 90);
    thumbPath.CloseFigure();
    
    if (game.hasImage) {
        Image* img = Image::FromFile(game.imagePath.c_str());
        if (img && img->GetLastStatus() == Ok) {
            Region oldClip;
            g.GetClip(&oldClip);
            g.SetClip(&thumbPath);
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            g.DrawImage(img, thumbX, thumbY, thumbSize, thumbSize);
            g.SetClip(&oldClip);
            delete img;
        }
    } else {
        // Premium placeholder
        LinearGradientBrush placeBg(
            Point(thumbX, thumbY), Point(thumbX, thumbY + thumbSize),
            Color(255, 28, 32, 42),
            Color(255, 20, 24, 34)
        );
        g.FillPath(&placeBg, &thumbPath);
        
        Font placeFont(L"Segoe UI Emoji", 20);
        SolidBrush placeBrush(Theme::Text::Disabled);
        StringFormat placeFmt;
        placeFmt.SetAlignment(StringAlignmentCenter);
        placeFmt.SetLineAlignment(StringAlignmentCenter);
        RectF placeRect((REAL)thumbX, (REAL)thumbY, (REAL)thumbSize, (REAL)thumbSize);
        g.DrawString(L"🎮", -1, &placeFont, placeRect, &placeFmt, &placeBrush);
    }
    
    // Thumbnail border (subtle reflection effect)
    Pen thumbBorder(Color(30, 255, 255, 255), 1);
    g.DrawPath(&thumbBorder, &thumbPath);
    
    // Game title
    int textX = thumbX + thumbSize + 16;
    int textW = w - textX - padding - 50 + x;
    
    Font nameFont(L"Segoe UI", 12, isSelected ? FontStyleBold : FontStyleRegular);
    SolidBrush nameBrush(isSelected ? Theme::Text::Primary : (isHovered ? Theme::Text::Primary : Theme::Text::Secondary));
    
    StringFormat nameFmt;
    nameFmt.SetTrimming(StringTrimmingEllipsisCharacter);
    nameFmt.SetFormatFlags(StringFormatFlagsNoWrap);
    nameFmt.SetLineAlignment(StringAlignmentCenter);
    
    RectF nameRect((REAL)textX, (REAL)(y + 8), (REAL)textW, (REAL)(h - 16));
    g.DrawString(game.displayName.c_str(), -1, &nameFont, nameRect, &nameFmt, &nameBrush);
    
    // Play indicator for selected
    if (isSelected) {
        int indicatorX = x + w - 44;
        int indicatorY = y + h/2 - 12;
        
        // Glow
        for (int i = 5; i > 0; i--) {
            SolidBrush indGlow(Color(35 - i * 7, 16, 185, 129));
            g.FillEllipse(&indGlow, indicatorX - i, indicatorY - i, 24 + i * 2, 24 + i * 2);
        }
        
        // Circle
        SolidBrush indBg(Color(60, 16, 185, 129));
        g.FillEllipse(&indBg, indicatorX, indicatorY, 24, 24);
        
        // Play triangle
        SolidBrush playBrush(Theme::Accent::Success);
        PointF triangle[3] = {
            PointF((REAL)(indicatorX + 9), (REAL)(indicatorY + 6)),
            PointF((REAL)(indicatorX + 9), (REAL)(indicatorY + 18)),
            PointF((REAL)(indicatorX + 18), (REAL)(indicatorY + 12))
        };
        g.FillPolygon(&playBrush, triangle, 3);
    }
}

// Premium preview panel
void DrawPreviewPanel(Graphics& g, int x, int y, int w, int h) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int r = Theme::Radius::XL;
    int padding = 24;
    
    // Main glass panel
    DrawGlassPanel(g, x, y, w, h, r, true);
    
    // Corner accents
    Pen cornerPen(Color(50, 99, 102, 241), 2);
    int cornerLen = 20;
    // Top-left
    g.DrawLine(&cornerPen, x + 12, y + 12, x + 12, y + 12 + cornerLen);
    g.DrawLine(&cornerPen, x + 12, y + 12, x + 12 + cornerLen, y + 12);
    // Top-right
    g.DrawLine(&cornerPen, x + w - 12, y + 12, x + w - 12, y + 12 + cornerLen);
    g.DrawLine(&cornerPen, x + w - 12, y + 12, x + w - 12 - cornerLen, y + 12);
    // Bottom-left
    g.DrawLine(&cornerPen, x + 12, y + h - 12, x + 12, y + h - 12 - cornerLen);
    g.DrawLine(&cornerPen, x + 12, y + h - 12, x + 12 + cornerLen, y + h - 12);
    // Bottom-right
    g.DrawLine(&cornerPen, x + w - 12, y + h - 12, x + w - 12, y + h - 12 - cornerLen);
    g.DrawLine(&cornerPen, x + w - 12, y + h - 12, x + w - 12 - cornerLen, y + h - 12);
    
    int imgX = x + padding;
    int imgY = y + padding;
    int imgW = w - padding * 2;
    int imgH = h - padding * 2 - 100;
    
    if (selectedIndex >= 0 && selectedIndex < (int)filteredGames.size()) {
        const Game& game = filteredGames[selectedIndex];
        
        if (game.hasImage) {
            Image* img = Image::FromFile(game.imagePath.c_str());
            if (img && img->GetLastStatus() == Ok) {
                // Calculate aspect-fit
                float imgRatio = (float)img->GetWidth() / img->GetHeight();
                float areaRatio = (float)imgW / imgH;
                
                int drawW, drawH, drawX, drawY;
                if (imgRatio > areaRatio) {
                    drawW = imgW;
                    drawH = (int)(imgW / imgRatio);
                    drawX = imgX;
                    drawY = imgY + (imgH - drawH) / 2;
                } else {
                    drawH = imgH;
                    drawW = (int)(imgH * imgRatio);
                    drawX = imgX + (imgW - drawW) / 2;
                    drawY = imgY;
                }
                
                // Image shadow
                DrawSoftShadow(g, drawX, drawY, drawW, drawH, Theme::Radius::LG, Theme::Shadow::XL, 10);
                
                // Clip and draw
                GraphicsPath imgPath;
                int imgR = Theme::Radius::LG;
                imgPath.AddArc(drawX, drawY, imgR * 2, imgR * 2, 180, 90);
                imgPath.AddArc(drawX + drawW - imgR * 2, drawY, imgR * 2, imgR * 2, 270, 90);
                imgPath.AddArc(drawX + drawW - imgR * 2, drawY + drawH - imgR * 2, imgR * 2, imgR * 2, 0, 90);
                imgPath.AddArc(drawX, drawY + drawH - imgR * 2, imgR * 2, imgR * 2, 90, 90);
                imgPath.CloseFigure();
                
                Region oldClip;
                g.GetClip(&oldClip);
                g.SetClip(&imgPath);
                g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                g.DrawImage(img, drawX, drawY, drawW, drawH);
                g.SetClip(&oldClip);
                
                // Subtle border
                Pen imgBorder(Color(30, 255, 255, 255), 1);
                g.DrawPath(&imgBorder, &imgPath);
                
                delete img;
            }
        } else {
            // Placeholder
            SolidBrush placeBg(Theme::Bg::Base);
            FillRoundedRect(g, imgX, imgY, imgW, imgH, Theme::Radius::LG, &placeBg);
            
            Font placeFont(L"Segoe UI Emoji", 48);
            SolidBrush placeBrush(Theme::Text::Disabled);
            StringFormat placeFmt;
            placeFmt.SetAlignment(StringAlignmentCenter);
            placeFmt.SetLineAlignment(StringAlignmentCenter);
            RectF placeRect((REAL)imgX, (REAL)imgY, (REAL)imgW, (REAL)imgH);
            g.DrawString(L"🎮", -1, &placeFont, placeRect, &placeFmt, &placeBrush);
        }
        
        // Game info section
        int infoY = y + h - 90;
        
        // Gradient fade background
        LinearGradientBrush fadeBg(
            Point(x, infoY - 30), Point(x, y + h - 12),
            Color(0, 15, 18, 25),
            Color(220, 15, 18, 25)
        );
        g.FillRectangle(&fadeBg, x + 12, infoY - 30, w - 24, h - infoY + 18);
        
        // Title with shadow
        Font titleFont(L"Segoe UI", 16, FontStyleBold);
        StringFormat titleFmt;
        titleFmt.SetAlignment(StringAlignmentCenter);
        titleFmt.SetTrimming(StringTrimmingEllipsisCharacter);
        
        SolidBrush shadowBrush(Color(100, 0, 0, 0));
        RectF shadowRect((REAL)(x + padding + 1), (REAL)(infoY + 1), (REAL)(w - padding * 2), 30);
        g.DrawString(game.displayName.c_str(), -1, &titleFont, shadowRect, &titleFmt, &shadowBrush);
        
        SolidBrush titleBrush(Theme::Text::Primary);
        RectF titleRect((REAL)(x + padding), (REAL)infoY, (REAL)(w - padding * 2), 30);
        g.DrawString(game.displayName.c_str(), -1, &titleFont, titleRect, &titleFmt, &titleBrush);
        
        // Status badge
        int badgeX = x + w/2 - 55;
        int badgeY = infoY + 38;
        
        // Badge background
        SolidBrush badgeBg(Color(40, 16, 185, 129));
        FillRoundedRect(g, badgeX, badgeY, 110, 26, Theme::Radius::Full, &badgeBg);
        
        // Pulsing dot
        for (int i = 3; i > 0; i--) {
            SolidBrush dotGlow(Color(30 - i * 10, 16, 185, 129));
            g.FillEllipse(&dotGlow, badgeX + 10 - i, badgeY + 8 - i/2, 10 + i * 2, 10 + i);
        }
        SolidBrush dotBrush(Theme::Accent::Success);
        g.FillEllipse(&dotBrush, badgeX + 10, badgeY + 8, 10, 10);
        SolidBrush dotShine(Color(80, 255, 255, 255));
        g.FillEllipse(&dotShine, badgeX + 12, badgeY + 9, 4, 4);
        
        // Status text
        Font statusFont(L"Segoe UI", 9);
        SolidBrush statusBrush(Theme::Text::Secondary);
        g.DrawString(L"Ready to play", -1, &statusFont, PointF((REAL)(badgeX + 26), (REAL)(badgeY + 5)), &statusBrush);
        
    } else {
        // Empty state
        Font emptyFont(L"Segoe UI", 14);
        SolidBrush emptyBrush(Theme::Text::Disabled);
        StringFormat emptyFmt;
        emptyFmt.SetAlignment(StringAlignmentCenter);
        emptyFmt.SetLineAlignment(StringAlignmentCenter);
        RectF emptyRect((REAL)x, (REAL)y, (REAL)w, (REAL)h);
        g.DrawString(L"Select a game", -1, &emptyFont, emptyRect, &emptyFmt, &emptyBrush);
    }
}

// Status bar
void DrawStatusBar(Graphics& g, int x, int y, int w, int h) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    // Background
    SolidBrush bgBrush(Color(255, 10, 12, 18));
    g.FillRectangle(&bgBrush, x, y, w, h);
    
    // Top gradient divider
    LinearGradientBrush divider(
        Point(x, y), Point(x + w, y),
        Color(60, 99, 102, 241),
        Color(60, 139, 92, 246)
    );
    g.FillRectangle(&divider, x, y, w, 1);
    
    // Stats
    Font statFont(L"Segoe UI", 10);
    SolidBrush accentBrush(Theme::Accent::Primary);
    g.DrawString(L"📁", -1, &statFont, PointF((REAL)(x + 20), (REAL)(y + 12)), &accentBrush);
    
    SolidBrush statBrush(Theme::Text::Muted);
    wstring statText = to_wstring(filteredGames.size()) + L" of " + to_wstring(games.size()) + L" games";
    g.DrawString(statText.c_str(), -1, &statFont, PointF((REAL)(x + 44), (REAL)(y + 13)), &statBrush);
    
    // Keyboard hint
    Font hintFont(L"Segoe UI", 9);
    SolidBrush hintBrush(Theme::Text::Disabled);
    StringFormat rightFmt;
    rightFmt.SetAlignment(StringAlignmentFar);
    RectF hintRect((REAL)x, (REAL)(y + 13), (REAL)(w - 20), 20);
    g.DrawString(L"Double-click or Enter to play", -1, &hintFont, hintRect, &rightFmt, &hintBrush);
}

// Premium scrollbar
void DrawScrollbar(Graphics& g, int x, int y, int w, int h, int contentH, int viewH, int scrollPos) {
    if (contentH <= viewH) return;
    
    int trackX = x + w - 10;
    int trackY = y + 8;
    int trackH = h - 16;
    int trackW = 6;
    
    // Track
    SolidBrush trackBrush(Color(30, 255, 255, 255));
    FillRoundedRect(g, trackX, trackY, trackW, trackH, 3, &trackBrush);
    
    // Thumb
    int thumbH = max(40, (viewH * trackH) / contentH);
    int maxScroll = contentH - viewH;
    int thumbY = trackY + (scrollPos * (trackH - thumbH)) / max(1, maxScroll);
    
    LinearGradientBrush thumbBrush(
        Point(trackX, thumbY), Point(trackX, thumbY + thumbH),
        Color(180, 99, 102, 241),
        Color(180, 139, 92, 246)
    );
    FillRoundedRect(g, trackX, thumbY, trackW, thumbH, 3, &thumbBrush);
    
    // Thumb shine
    SolidBrush shineBrush(Color(40, 255, 255, 255));
    g.FillRectangle(&shineBrush, trackX + 1, thumbY + 3, trackW - 2, 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GAME LOGIC
// ═══════════════════════════════════════════════════════════════════════════════

bool CopyLocalImage(const wstring& gameName, const wstring& savePath) {
    wstring searchName;
    for (size_t i = 0; i < gameName.length(); i++) {
        wchar_t c = gameName[i];
        if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9')) {
            if (c >= L'A' && c <= L'Z') searchName += (c + 32);
            else searchName += c;
        }
    }
    
    // Try exact match first
    wstring localPath = localImagesFolder + L"\\" + searchName + L".png";
    if (GetFileAttributesW(localPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (CopyFileW(localPath.c_str(), savePath.c_str(), FALSE)) return true;
    }
    
    // Try partial match
    WIN32_FIND_DATAW findData;
    wstring searchPattern = localImagesFolder + L"\\*" + searchName + L"*.png";
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        wstring foundPath = localImagesFolder + L"\\" + findData.cFileName;
        FindClose(hFind);
        if (CopyFileW(foundPath.c_str(), savePath.c_str(), FALSE)) return true;
    }
    
    return false;
}

void LoadGameImage(Game& game) {
    wstring safeFileName = game.name;
    for (wchar_t& c : safeFileName) {
        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || 
            c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
            c = L'_';
        }
    }
    game.imagePath = cacheFolder + L"\\" + safeFileName + L".jpg";
    
    if (GetFileAttributesW(game.imagePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        game.hasImage = true;
        return;
    }
    if (CopyLocalImage(game.name, game.imagePath)) {
        game.hasImage = true;
        return;
    }
    game.hasImage = false;
}

void ScanGames() {
    games.clear();
    
    WIN32_FIND_DATAW findData;
    wstring searchPath = gamesFolder + L"\\*";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) return;
    
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            wstring name = findData.cFileName;
            if (name != L"." && name != L"..") {
                wstring gamePath = gamesFolder + L"\\" + name;
                
                WIN32_FIND_DATAW exeData;
                wstring exeSearch = gamePath + L"\\*.exe";
                HANDLE hExe = FindFirstFileW(exeSearch.c_str(), &exeData);
                
                if (hExe != INVALID_HANDLE_VALUE) {
                    Game game;
                    game.name = name;
                    game.displayName = name;
                    game.exePath = gamePath + L"\\" + exeData.cFileName;
                    game.hasImage = false;
                    game.playCount = 0;
                    game.lastPlayed = 0;
                    LoadGameImage(game);
                    games.push_back(game);
                    FindClose(hExe);
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);
    
    sort(games.begin(), games.end(), [](const Game& a, const Game& b) {
        return _wcsicmp(a.displayName.c_str(), b.displayName.c_str()) < 0;
    });
    
    filteredGames = games;
}

void FilterGames(const wstring& query) {
    filteredGames.clear();
    
    wstring lowerQuery = query;
    transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& game : games) {
        wstring lowerName = game.displayName;
        transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerQuery.empty() || lowerName.find(lowerQuery) != wstring::npos) {
            filteredGames.push_back(game);
        }
    }
    
    selectedIndex = -1;
    listScrollY = 0;
}

void LaunchGame(int index) {
    if (index >= 0 && index < (int)filteredGames.size()) {
        ShellExecuteW(NULL, L"open", filteredGames[index].exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// WINDOW PROCEDURES
// ═══════════════════════════════════════════════════════════════════════════════

LRESULT CALLBACK ListProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int dragStartY = 0, dragStartScroll = 0;
    static bool isDragging = false;
    
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            // Double buffer
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBmp);
            
            Graphics g(memDC);
            g.SetSmoothingMode(SmoothingModeHighQuality);
            
            // Background
            SolidBrush bgBrush(Theme::Bg::Surface);
            g.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
            
            // Draw visible items
            int y = -listScrollY;
            for (size_t i = 0; i < filteredGames.size(); i++) {
                if (y + itemHeight > 0 && y < rc.bottom) {
                    DrawGameCard(g, 0, y, rc.right - 12, itemHeight, filteredGames[i],
                        (int)i == selectedIndex, (int)i == hoveredItem, (int)i, 
                        hoverAnimProgress[i]);
                }
                y += itemHeight;
            }
            
            // Scrollbar
            int contentH = (int)filteredGames.size() * itemHeight;
            DrawScrollbar(g, 0, 0, rc.right, rc.bottom, contentH, rc.bottom, listScrollY);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int newHover = (pt.y + listScrollY) / itemHeight;
            if (newHover < 0 || newHover >= (int)filteredGames.size()) newHover = -1;
            
            if (newHover != hoveredItem) {
                hoveredItem = newHover;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hWnd, 0};
            TrackMouseEvent(&tme);
            
            if (isDragging) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                int delta = dragStartY - pt.y;
                int maxScroll = max(0, (int)((int)filteredGames.size() * itemHeight - rc.bottom));
                listScrollY = max(0, min(maxScroll, dragStartScroll + delta));
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_MOUSELEAVE:
            if (hoveredItem >= 0) {
                hoveredItem = -1;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        
        case WM_LBUTTONDOWN: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int clicked = (pt.y + listScrollY) / itemHeight;
            if (clicked >= 0 && clicked < (int)filteredGames.size()) {
                selectedIndex = clicked;
                InvalidateRect(hWnd, NULL, FALSE);
                InvalidateRect(hPreviewArea, NULL, FALSE);
            }
            dragStartY = pt.y;
            dragStartScroll = listScrollY;
            isDragging = true;
            SetCapture(hWnd);
            return 0;
        }
        
        case WM_LBUTTONUP:
            isDragging = false;
            ReleaseCapture();
            return 0;
        
        case WM_LBUTTONDBLCLK: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int clicked = (pt.y + listScrollY) / itemHeight;
            if (clicked >= 0 && clicked < (int)filteredGames.size()) {
                LaunchGame(clicked);
            }
            return 0;
        }
        
        case WM_MOUSEWHEEL: {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int maxScroll = max(0, (int)((int)filteredGames.size() * itemHeight - rc.bottom));
            listScrollY = max(0, min(maxScroll, listScrollY - delta / 2));
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        
        case WM_KEYDOWN:
            if (wParam == VK_UP && selectedIndex > 0) {
                selectedIndex--;
                // Scroll into view
                if (selectedIndex * itemHeight < listScrollY) {
                    listScrollY = selectedIndex * itemHeight;
                }
                InvalidateRect(hWnd, NULL, FALSE);
                InvalidateRect(hPreviewArea, NULL, FALSE);
            } else if (wParam == VK_DOWN && selectedIndex < (int)filteredGames.size() - 1) {
                selectedIndex++;
                RECT rc;
                GetClientRect(hWnd, &rc);
                if ((selectedIndex + 1) * itemHeight > listScrollY + rc.bottom) {
                    listScrollY = (selectedIndex + 1) * itemHeight - rc.bottom;
                }
                InvalidateRect(hWnd, NULL, FALSE);
                InvalidateRect(hPreviewArea, NULL, FALSE);
            } else if (wParam == VK_RETURN && selectedIndex >= 0) {
                LaunchGame(selectedIndex);
            }
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK PreviewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBmp);
            
            Graphics g(memDC);
            
            SolidBrush bgBrush(Theme::Bg::Surface);
            g.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
            
            DrawPreviewPanel(g, 0, 0, rc.right, rc.bottom);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK BtnProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static bool isHovered = false;
    static bool isPressed = false;
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBmp);
            
            Graphics g(memDC);
            SolidBrush bgBrush(Theme::Bg::Surface);
            g.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
            
            wchar_t text[64];
            GetWindowTextW(hWnd, text, 64);
            
            bool isPrimary = (dwRefData == 1);
            DrawPremiumButton(g, 0, 0, rc.right, rc.bottom, text, isPrimary, isHovered, isPressed);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_MOUSEMOVE:
            if (!isHovered) {
                isHovered = true;
                InvalidateRect(hWnd, NULL, FALSE);
                TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hWnd, 0};
                TrackMouseEvent(&tme);
            }
            break;
        
        case WM_MOUSELEAVE:
            isHovered = false;
            isPressed = false;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        
        case WM_LBUTTONDOWN:
            isPressed = true;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        
        case WM_LBUTTONUP:
            isPressed = false;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Enable dark mode
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(hWnd, 20, &darkMode, sizeof(darkMode));
            
            // Try Mica effect (Windows 11)
            int micaValue = 2;
            DwmSetWindowAttribute(hWnd, 38, &micaValue, sizeof(micaValue));
            
            hHandCursor = LoadCursor(NULL, IDC_HAND);
            
            // Register list class
            WNDCLASSW listClass = {0};
            listClass.lpfnWndProc = ListProc;
            listClass.hInstance = GetModuleHandle(NULL);
            listClass.lpszClassName = L"PremiumGameList";
            listClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            listClass.style = CS_DBLCLKS | CS_OWNDC;
            RegisterClassW(&listClass);
            
            // Register preview class
            WNDCLASSW prevClass = {0};
            prevClass.lpfnWndProc = PreviewProc;
            prevClass.hInstance = GetModuleHandle(NULL);
            prevClass.lpszClassName = L"PremiumPreview";
            prevClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            RegisterClassW(&prevClass);
            
            // Search edit
            hSearchEdit = CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                90, 95, 320, 30, hWnd, (HMENU)1, NULL, NULL);
            
            HFONT hEditFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            SendMessageW(hSearchEdit, WM_SETFONT, (WPARAM)hEditFont, TRUE);
            
            // List area
            hListArea = CreateWindowW(L"PremiumGameList", L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                24, 150, 420, 430, hWnd, NULL, NULL, NULL);
            
            // Preview area
            hPreviewArea = CreateWindowW(L"PremiumPreview", L"",
                WS_CHILD | WS_VISIBLE,
                465, 95, 415, 415, hWnd, NULL, NULL, NULL);
            
            // Buttons
            hPlayBtn = CreateWindowW(L"BUTTON", L"▶  Play Now",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                465, 525, 195, 56, hWnd, (HMENU)2, NULL, NULL);
            SetWindowSubclass(hPlayBtn, BtnProc, 0, 1);
            
            hRefreshBtn = CreateWindowW(L"BUTTON", L"↻  Refresh",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                680, 525, 200, 56, hWnd, (HMENU)3, NULL, NULL);
            SetWindowSubclass(hRefreshBtn, BtnProc, 0, 0);
            
            // Setup
            cacheFolder = L"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\cache";
            CreateDirectoryW(cacheFolder.c_str(), NULL);
            
            // Sync images silently
            STARTUPINFOW si = {sizeof(si)};
            PROCESS_INFORMATION pi = {0};
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            wchar_t cmdLine[] = L"powershell.exe -ExecutionPolicy Bypass -File \"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\sync-images.ps1\" -Silent";
            if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 15000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            
            ScanGames();
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBmp);
            
            Graphics g(memDC);
            g.SetSmoothingMode(SmoothingModeHighQuality);
            
            // Main background
            SolidBrush bgBrush(Theme::Bg::Surface);
            g.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
            
            // Header
            DrawHeader(g, rc.right, 80);
            
            // Search box background
            RECT searchRc;
            GetWindowRect(hSearchEdit, &searchRc);
            MapWindowPoints(NULL, hWnd, (LPPOINT)&searchRc, 2);
            DrawSearchBox(g, searchRc.left - 50, searchRc.top - 5, 
                searchRc.right - searchRc.left + 54, searchRc.bottom - searchRc.top + 10,
                GetFocus() == hSearchEdit, L"Search games...");
            
            // Status bar
            DrawStatusBar(g, 0, rc.bottom - 48, rc.right, 48);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBmp);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(248, 250, 252));
            SetBkColor(hdcEdit, RGB(22, 26, 35));
            static HBRUSH hEditBrush = CreateSolidBrush(RGB(22, 26, 35));
            return (LRESULT)hEditBrush;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1 && HIWORD(wParam) == EN_CHANGE) {
                wchar_t buffer[256];
                GetWindowTextW(hSearchEdit, buffer, 256);
                FilterGames(buffer);
                InvalidateRect(hListArea, NULL, FALSE);
                InvalidateRect(hPreviewArea, NULL, FALSE);
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (LOWORD(wParam) == 1 && (HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS)) {
                searchFocused = (HIWORD(wParam) == EN_SETFOCUS);
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (LOWORD(wParam) == 2) {
                if (selectedIndex >= 0) LaunchGame(selectedIndex);
            } else if (LOWORD(wParam) == 3) {
                // Refresh
                STARTUPINFOW si = {sizeof(si)};
                PROCESS_INFORMATION pi = {0};
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;
                wchar_t cmdLine[] = L"powershell.exe -ExecutionPolicy Bypass -File \"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\sync-images.ps1\" -Silent";
                if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                    WaitForSingleObject(pi.hProcess, 10000);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
                SetWindowTextW(hSearchEdit, L"");
                selectedIndex = -1;
                listScrollY = 0;
                ScanGames();
                InvalidateRect(hWnd, NULL, TRUE);
                InvalidateRect(hListArea, NULL, FALSE);
                InvalidateRect(hPreviewArea, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_SIZE: {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int w = rc.right;
            int h = rc.bottom;
            
            // Responsive layout
            int listW = min(480, (w - 60) * 45 / 100);
            int previewX = listW + 45;
            int previewW = w - previewX - 24;
            int listH = h - 205;
            int previewH = h - 180;
            int btnY = h - 100;
            int btnW = (previewW - 20) / 2;
            
            SetWindowPos(hSearchEdit, NULL, 90, 95, listW - 68, 30, SWP_NOZORDER);
            SetWindowPos(hListArea, NULL, 24, 150, listW, listH, SWP_NOZORDER);
            SetWindowPos(hPreviewArea, NULL, previewX, 95, previewW, previewH, SWP_NOZORDER);
            SetWindowPos(hPlayBtn, NULL, previewX, btnY, btnW, 56, SWP_NOZORDER);
            SetWindowPos(hRefreshBtn, NULL, previewX + btnW + 20, btnY, btnW, 56, SWP_NOZORDER);
            
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 850;
            mmi->ptMinTrackSize.y = 600;
            return 0;
        }
        
        case WM_DESTROY:
            GdiplusShutdown(gdiplusToken);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ENTRY POINT
// ═══════════════════════════════════════════════════════════════════════════════

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    // GDI+ init
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // Common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Window class
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PremiumGameLauncher";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(15, 18, 25));
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);
    
    // Center on screen
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 940;
    int winH = 700;
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;
    
    hMainWnd = CreateWindowExW(
        WS_EX_COMPOSITED,
        L"PremiumGameLauncher",
        L"Game Library",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        winX, winY, winW, winH,
        NULL, NULL, hInstance, NULL
    );
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    return (int)msg.wParam;
}

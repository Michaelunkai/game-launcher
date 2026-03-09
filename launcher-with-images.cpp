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

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace std;
using namespace Gdiplus;

// Premium color palette - inspired by modern gaming platforms
namespace Colors {
    // Backgrounds
    const Color BgDark(255, 13, 17, 23);           // Deepest dark
    const Color BgMain(255, 22, 27, 34);           // Main background
    const Color BgCard(255, 33, 38, 45);           // Card surface
    const Color BgCardHover(255, 48, 54, 61);      // Hover state
    const Color BgCardSelected(255, 38, 43, 50);   // Selected
    
    // Accents
    const Color AccentPrimary(255, 88, 166, 255);  // Blue accent
    const Color AccentSecondary(255, 139, 92, 246);// Purple
    const Color AccentGreen(255, 46, 204, 113);    // Success green
    const Color AccentOrange(255, 255, 159, 67);   // Warning orange
    const Color AccentPink(255, 236, 72, 153);     // Pink accent
    
    // Text
    const Color TextPrimary(255, 240, 246, 252);   // White
    const Color TextSecondary(255, 139, 148, 158); // Muted
    const Color TextTertiary(255, 88, 96, 105);    // Subtle
    
    // Borders & dividers
    const Color Border(255, 48, 54, 61);
    const Color BorderLight(255, 68, 76, 86);
    const Color Divider(255, 33, 38, 45);
    
    // Gradients
    const Color GradStart(255, 88, 166, 255);      // Blue
    const Color GradEnd(255, 139, 92, 246);        // Purple
}

struct Game {
    wstring name;
    wstring displayName;
    wstring exePath;
    wstring imagePath;
    bool hasImage;
};

vector<Game> games;
vector<Game> filteredGames;
HWND hMainWnd, hListView, hSearchBox, hImageWnd;
HWND hPlayBtn, hRefreshBtn;
HFONT hTitleFont, hMainFont, hSmallFont, hButtonFont, hAccentFont;
ULONG_PTR gdiplusToken;
wstring gamesFolder = L"E:\\games";
wstring cacheFolder;
wstring localImagesFolder = L"C:\\Users\\micha\\.openclaw\\workspace-moltbot\\game-library-manager-web\\public\\images";
int selectedIndex = -1;
int hoveredItem = -1;
int listScrollPos = 0;
int itemHeight = 56;
HBITMAP hBackBuffer = NULL;

// Forward declarations
void ScanGames();
void UpdateListView();
void LaunchGame(int index);
bool CopyLocalImage(const wstring& gameName, const wstring& savePath);

// Helper to draw rounded rect
void DrawRoundedRect(Graphics& g, int x, int y, int w, int h, int r, const Brush* fill, const Pen* stroke = NULL) {
    GraphicsPath path;
    path.AddArc(x, y, r * 2, r * 2, 180, 90);
    path.AddArc(x + w - r * 2, y, r * 2, r * 2, 270, 90);
    path.AddArc(x + w - r * 2, y + h - r * 2, r * 2, r * 2, 0, 90);
    path.AddArc(x, y + h - r * 2, r * 2, r * 2, 90, 90);
    path.CloseFigure();
    
    if (fill) g.FillPath(fill, &path);
    if (stroke) g.DrawPath(stroke, &path);
}

// Draw shadow behind element
void DrawShadow(Graphics& g, int x, int y, int w, int h, int r, int blur = 8) {
    for (int i = blur; i > 0; i--) {
        int alpha = 20 - (i * 2);
        if (alpha < 0) alpha = 0;
        SolidBrush shadowBrush(Color(alpha, 0, 0, 0));
        DrawRoundedRect(g, x - i/2, y + i, w + i, h + i/2, r + i/2, &shadowBrush);
    }
}

// Draw gradient button
void DrawGradientButton(Graphics& g, int x, int y, int w, int h, const wchar_t* text, bool isPrimary, bool isHovered) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int radius = 12;
    
    GraphicsPath path;
    path.AddArc(x, y, radius * 2, radius * 2, 180, 90);
    path.AddArc(x + w - radius * 2, y, radius * 2, radius * 2, 270, 90);
    path.AddArc(x + w - radius * 2, y + h - radius * 2, radius * 2, radius * 2, 0, 90);
    path.AddArc(x, y + h - radius * 2, radius * 2, radius * 2, 90, 90);
    path.CloseFigure();
    
    if (isPrimary) {
        // Glow effect for primary button
        if (isHovered) {
            for (int i = 8; i > 0; i--) {
                int alpha = 40 - i * 5;
                if (alpha < 0) alpha = 0;
                SolidBrush glowBrush(Color(alpha, 88, 166, 255));
                DrawRoundedRect(g, x - i, y - i/2, w + i * 2, h + i, radius + i/2, &glowBrush);
            }
        } else {
            DrawShadow(g, x, y, w, h, radius, 8);
        }
        
        // Gradient fill with shine
        LinearGradientBrush gradBrush(
            Point(x, y), Point(x, y + h),
            isHovered ? Color(255, 118, 190, 255) : Color(255, 98, 176, 255),
            isHovered ? Color(255, 149, 102, 255) : Colors::AccentSecondary
        );
        g.FillPath(&gradBrush, &path);
        
        // Top shine highlight
        GraphicsPath shinePath;
        shinePath.AddArc(x + 2, y + 2, radius * 2 - 4, radius * 2 - 4, 180, 90);
        shinePath.AddArc(x + w - radius * 2 + 2, y + 2, radius * 2 - 4, radius * 2 - 4, 270, 90);
        shinePath.AddLine(x + w - 2, y + h/3, x + 2, y + h/3);
        shinePath.CloseFigure();
        
        LinearGradientBrush shineBrush(
            Point(x, y), Point(x, y + h/3),
            Color(50, 255, 255, 255),
            Color(0, 255, 255, 255)
        );
        g.FillPath(&shineBrush, &shinePath);
        
    } else {
        // Secondary button with subtle hover
        if (isHovered) {
            SolidBrush hoverBrush(Colors::BgCardHover);
            g.FillPath(&hoverBrush, &path);
            Pen borderPen(Colors::AccentPrimary, 1.5f);
            g.DrawPath(&borderPen, &path);
        } else {
            SolidBrush fillBrush(Colors::BgCard);
            g.FillPath(&fillBrush, &path);
            Pen borderPen(Colors::Border, 1.5f);
            g.DrawPath(&borderPen, &path);
        }
    }
    
    // Text with subtle shadow for primary
    if (isPrimary) {
        Font font(L"Segoe UI", 12, FontStyleBold);
        SolidBrush shadowBrush(Color(60, 0, 0, 0));
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        format.SetLineAlignment(StringAlignmentCenter);
        RectF shadowRect((REAL)(x + 1), (REAL)(y + 1), (REAL)w, (REAL)h);
        g.DrawString(text, -1, &font, shadowRect, &format, &shadowBrush);
    }
    
    Font font(L"Segoe UI", 12, FontStyleBold);
    SolidBrush textBrush(Colors::TextPrimary);
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    RectF rect((REAL)x, (REAL)y, (REAL)w, (REAL)h);
    g.DrawString(text, -1, &font, rect, &format, &textBrush);
}

// Draw search box
void DrawSearchBox(Graphics& g, int x, int y, int w, int h, bool hasFocus) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    
    int radius = 14;
    
    // Shadow when focused
    if (hasFocus) {
        for (int i = 6; i > 0; i--) {
            int alpha = 30 - i * 5;
            if (alpha < 0) alpha = 0;
            SolidBrush glowBrush(Color(alpha, 88, 166, 255));
            DrawRoundedRect(g, x - i, y - i/2, w + i * 2, h + i, radius + i/2, &glowBrush);
        }
    }
    
    // Background with inner gradient
    LinearGradientBrush bgBrush(
        Point(x, y), Point(x, y + h),
        Color(255, 38, 43, 51),
        Color(255, 32, 37, 45)
    );
    DrawRoundedRect(g, x, y, w, h, radius, &bgBrush);
    
    // Border
    Pen borderPen(hasFocus ? Colors::AccentPrimary : Colors::Border, hasFocus ? 2.0f : 1.0f);
    GraphicsPath path;
    path.AddArc(x, y, radius * 2, radius * 2, 180, 90);
    path.AddArc(x + w - radius * 2, y, radius * 2, radius * 2, 270, 90);
    path.AddArc(x + w - radius * 2, y + h - radius * 2, radius * 2, radius * 2, 0, 90);
    path.AddArc(x, y + h - radius * 2, radius * 2, radius * 2, 90, 90);
    path.CloseFigure();
    g.DrawPath(&borderPen, &path);
    
    // Top inner highlight
    LinearGradientBrush innerHighlight(
        Point(x, y), Point(x, y + 8),
        Color(20, 255, 255, 255),
        Color(0, 255, 255, 255)
    );
    g.FillRectangle(&innerHighlight, x + radius, y + 1, w - radius * 2, 6);
    
    // Search icon with glow when focused
    if (hasFocus) {
        Font iconFont(L"Segoe UI Symbol", 12);
        SolidBrush iconBrush(Colors::AccentPrimary);
        g.DrawString(L"🔍", -1, &iconFont, PointF((REAL)x + 14, (REAL)y + 8), &iconBrush);
    } else {
        Font iconFont(L"Segoe UI Symbol", 12);
        SolidBrush iconBrush(Colors::TextSecondary);
        g.DrawString(L"🔍", -1, &iconFont, PointF((REAL)x + 14, (REAL)y + 8), &iconBrush);
    }
}

// Draw header section
void DrawHeader(Graphics& g, int width) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    // Beautiful gradient header background
    LinearGradientBrush headerBg(
        Point(0, 0), Point(width, 70),
        Color(255, 25, 30, 38),
        Colors::BgMain
    );
    g.FillRectangle(&headerBg, 0, 0, width, 70);
    
    // Accent gradient line at top (3px)
    LinearGradientBrush lineBrush(
        Point(0, 0), Point(width, 0),
        Colors::AccentPrimary, Colors::AccentSecondary
    );
    g.FillRectangle(&lineBrush, 0, 0, width, 3);
    
    // Glow effect under accent line
    for (int i = 0; i < 8; i++) {
        int alpha = 30 - i * 4;
        if (alpha < 0) alpha = 0;
        LinearGradientBrush glowBrush(
            Point(0, 3 + i), Point(width, 3 + i),
            Color(alpha, 88, 166, 255),
            Color(alpha, 139, 92, 246)
        );
        g.FillRectangle(&glowBrush, 0, 3 + i, width, 1);
    }
    
    // Game controller icon
    Font iconFont(L"Segoe UI Emoji", 20);
    SolidBrush iconBrush(Colors::AccentPrimary);
    g.DrawString(L"🎮", -1, &iconFont, PointF(24, 22), &iconBrush);
    
    // App title
    Font titleFont(L"Segoe UI", 20, FontStyleBold);
    SolidBrush titleBrush(Colors::TextPrimary);
    g.DrawString(L"Game Library", -1, &titleFont, PointF(60, 22), &titleBrush);
    
    // Subtitle with game count
    Font subFont(L"Segoe UI", 9);
    SolidBrush subBrush(Colors::TextSecondary);
    wstring subText = L"Your personal collection • " + to_wstring(games.size()) + L" games";
    g.DrawString(subText.c_str(), -1, &subFont, PointF(62, 50), &subBrush);
}

// Draw game item in list
void DrawGameItem(Graphics& g, int x, int y, int w, int h, const Game& game, bool isSelected, bool isHovered, int index) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int radius = 12;
    int margin = 8;
    
    // Card background with subtle elevation
    if (isSelected) {
        // Glow effect behind selected card
        for (int i = 6; i > 0; i--) {
            int alpha = 15 - i * 2;
            SolidBrush glowBrush(Color(alpha, 88, 166, 255));
            DrawRoundedRect(g, x + margin - i, y + 3 - i/2, w - margin * 2 + i * 2, h - 6 + i, radius + i/2, &glowBrush);
        }
        
        // Selected card background
        LinearGradientBrush selectBrush(
            Point(x, y), Point(x + w, y + h),
            Color(80, 88, 166, 255),
            Color(40, 139, 92, 246)
        );
        DrawRoundedRect(g, x + margin, y + 3, w - margin * 2, h - 6, radius, &selectBrush);
        
        // Left accent bar with glow
        LinearGradientBrush accentBrush(
            Point(x + margin, y), Point(x + margin, y + h),
            Colors::AccentPrimary, Colors::AccentSecondary
        );
        g.FillRectangle(&accentBrush, x + margin, y + 10, 4, h - 20);
    } else if (isHovered) {
        // Hover card with subtle border
        SolidBrush hoverBrush(Colors::BgCardHover);
        DrawRoundedRect(g, x + margin, y + 3, w - margin * 2, h - 6, radius, &hoverBrush);
        
        Pen hoverBorder(Color(60, 88, 166, 255), 1);
        GraphicsPath hoverPath;
        hoverPath.AddArc(x + margin, y + 3, radius * 2, radius * 2, 180, 90);
        hoverPath.AddArc(x + w - margin - radius * 2, y + 3, radius * 2, radius * 2, 270, 90);
        hoverPath.AddArc(x + w - margin - radius * 2, y + h - 9 - radius * 2, radius * 2, radius * 2, 0, 90);
        hoverPath.AddArc(x + margin, y + h - 9 - radius * 2, radius * 2, radius * 2, 90, 90);
        hoverPath.CloseFigure();
        g.DrawPath(&hoverBorder, &hoverPath);
    }
    
    // Game thumbnail with shadow
    int thumbSize = 42;
    int thumbX = x + margin + 18;
    int thumbY = y + (h - thumbSize) / 2;
    
    // Shadow under thumbnail
    if (isSelected || isHovered) {
        for (int i = 4; i > 0; i--) {
            SolidBrush thumbShadow(Color(20 - i * 4, 0, 0, 0));
            DrawRoundedRect(g, thumbX - i/2, thumbY + i, thumbSize + i, thumbSize + i/2, 8 + i/2, &thumbShadow);
        }
    }
    
    if (game.hasImage) {
        Image* img = Image::FromFile(game.imagePath.c_str());
        if (img && img->GetLastStatus() == Ok) {
            // Rounded thumbnail with border
            int thumbRadius = 8;
            GraphicsPath clipPath;
            clipPath.AddArc(thumbX, thumbY, thumbRadius * 2, thumbRadius * 2, 180, 90);
            clipPath.AddArc(thumbX + thumbSize - thumbRadius * 2, thumbY, thumbRadius * 2, thumbRadius * 2, 270, 90);
            clipPath.AddArc(thumbX + thumbSize - thumbRadius * 2, thumbY + thumbSize - thumbRadius * 2, thumbRadius * 2, thumbRadius * 2, 0, 90);
            clipPath.AddArc(thumbX, thumbY + thumbSize - thumbRadius * 2, thumbRadius * 2, thumbRadius * 2, 90, 90);
            clipPath.CloseFigure();
            
            Region oldClip;
            g.GetClip(&oldClip);
            g.SetClip(&clipPath);
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            g.DrawImage(img, thumbX, thumbY, thumbSize, thumbSize);
            g.SetClip(&oldClip);
            
            // Border around thumb
            Pen thumbBorder(Color(40, 255, 255, 255), 1);
            g.DrawPath(&thumbBorder, &clipPath);
            
            delete img;
        }
    } else {
        // Gradient placeholder
        LinearGradientBrush placeBrush(
            Point(thumbX, thumbY), Point(thumbX, thumbY + thumbSize),
            Color(255, 30, 34, 42),
            Color(255, 24, 28, 36)
        );
        DrawRoundedRect(g, thumbX, thumbY, thumbSize, thumbSize, 8, &placeBrush);
        
        // Border
        Pen placeBorder(Colors::Border, 1);
        GraphicsPath placePath;
        placePath.AddArc(thumbX, thumbY, 16, 16, 180, 90);
        placePath.AddArc(thumbX + thumbSize - 16, thumbY, 16, 16, 270, 90);
        placePath.AddArc(thumbX + thumbSize - 16, thumbY + thumbSize - 16, 16, 16, 0, 90);
        placePath.AddArc(thumbX, thumbY + thumbSize - 16, 16, 16, 90, 90);
        placePath.CloseFigure();
        g.DrawPath(&placeBorder, &placePath);
        
        Font iconFont(L"Segoe UI Emoji", 18);
        SolidBrush iconBrush(Colors::TextTertiary);
        StringFormat iconFmt;
        iconFmt.SetAlignment(StringAlignmentCenter);
        iconFmt.SetLineAlignment(StringAlignmentCenter);
        RectF iconRect((REAL)thumbX, (REAL)thumbY, (REAL)thumbSize, (REAL)thumbSize);
        g.DrawString(L"🎮", -1, &iconFont, iconRect, &iconFmt, &iconBrush);
    }
    
    // Game name
    int textX = thumbX + thumbSize + 16;
    Font nameFont(L"Segoe UI", 11, isSelected ? FontStyleBold : FontStyleRegular);
    SolidBrush nameBrush(isSelected || isHovered ? Colors::TextPrimary : Colors::TextSecondary);
    
    StringFormat nameFmt;
    nameFmt.SetTrimming(StringTrimmingEllipsisCharacter);
    nameFmt.SetFormatFlags(StringFormatFlagsNoWrap);
    nameFmt.SetLineAlignment(StringAlignmentCenter);
    
    RectF nameRect((REAL)textX, (REAL)(y + 6), (REAL)(w - textX - margin - 50), (REAL)(h - 12));
    g.DrawString(game.displayName.c_str(), -1, &nameFont, nameRect, &nameFmt, &nameBrush);
    
    // Play indicator for selected with glow
    if (isSelected) {
        int playX = x + w - 36;
        int playY = y + h/2 - 10;
        
        // Glow
        for (int i = 3; i > 0; i--) {
            SolidBrush playGlow(Color(40 - i * 12, 46, 204, 113));
            g.FillEllipse(&playGlow, playX - i, playY - i, 20 + i * 2, 20 + i * 2);
        }
        
        // Circle background
        SolidBrush circleBg(Color(60, 46, 204, 113));
        g.FillEllipse(&circleBg, playX, playY, 20, 20);
        
        // Play icon
        SolidBrush playBrush(Colors::AccentGreen);
        Font playFont(L"Segoe UI Symbol", 9);
        StringFormat playFmt;
        playFmt.SetAlignment(StringAlignmentCenter);
        playFmt.SetLineAlignment(StringAlignmentCenter);
        RectF playRect((REAL)playX, (REAL)playY, 20, 20);
        g.DrawString(L"▶", -1, &playFont, playRect, &playFmt, &playBrush);
    }
}

// Draw game preview panel
void DrawPreviewPanel(Graphics& g, int x, int y, int w, int h) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int radius = 16;
    int padding = 24;
    
    // Outer glow/shadow
    DrawShadow(g, x, y, w, h, radius, 10);
    
    // Panel background with gradient
    LinearGradientBrush panelBg(
        Point(x, y), Point(x, y + h),
        Color(255, 36, 41, 48),
        Color(255, 28, 32, 40)
    );
    DrawRoundedRect(g, x, y, w, h, radius, &panelBg);
    
    // Subtle top highlight
    LinearGradientBrush topHighlight(
        Point(x, y), Point(x, y + 60),
        Color(15, 255, 255, 255),
        Color(0, 255, 255, 255)
    );
    GraphicsPath highlightPath;
    highlightPath.AddArc(x, y, radius * 2, radius * 2, 180, 90);
    highlightPath.AddArc(x + w - radius * 2, y, radius * 2, radius * 2, 270, 90);
    highlightPath.AddLine(x + w, y + radius, x + w, y + 50);
    highlightPath.AddLine(x + w, y + 50, x, y + 50);
    highlightPath.AddLine(x, y + 50, x, y + radius);
    highlightPath.CloseFigure();
    g.FillPath(&topHighlight, &highlightPath);
    
    // Border with gradient
    Pen borderPen(Color(80, 88, 96, 105), 1);
    GraphicsPath borderPath;
    borderPath.AddArc(x, y, radius * 2, radius * 2, 180, 90);
    borderPath.AddArc(x + w - radius * 2, y, radius * 2, radius * 2, 270, 90);
    borderPath.AddArc(x + w - radius * 2, y + h - radius * 2, radius * 2, radius * 2, 0, 90);
    borderPath.AddArc(x, y + h - radius * 2, radius * 2, radius * 2, 90, 90);
    borderPath.CloseFigure();
    g.DrawPath(&borderPen, &borderPath);
    
    int imgAreaX = x + padding;
    int imgAreaY = y + padding;
    int imgAreaW = w - padding * 2;
    int imgAreaH = h - padding * 2 - 90;
    
    // Decorative corner accents on preview panel
    Pen accentPen(Color(40, 88, 166, 255), 2);
    // Top left
    g.DrawLine(&accentPen, x + 8, y + 20, x + 8, y + 8);
    g.DrawLine(&accentPen, x + 8, y + 8, x + 20, y + 8);
    // Top right
    g.DrawLine(&accentPen, x + w - 8, y + 20, x + w - 8, y + 8);
    g.DrawLine(&accentPen, x + w - 8, y + 8, x + w - 20, y + 8);
    // Bottom left
    g.DrawLine(&accentPen, x + 8, y + h - 20, x + 8, y + h - 8);
    g.DrawLine(&accentPen, x + 8, y + h - 8, x + 20, y + h - 8);
    // Bottom right  
    g.DrawLine(&accentPen, x + w - 8, y + h - 20, x + w - 8, y + h - 8);
    g.DrawLine(&accentPen, x + w - 8, y + h - 8, x + w - 20, y + h - 8);
    
    if (selectedIndex >= 0 && selectedIndex < (int)filteredGames.size()) {
        const Game& game = filteredGames[selectedIndex];
        
        // Image area
        if (game.hasImage) {
            Image* img = Image::FromFile(game.imagePath.c_str());
            if (img && img->GetLastStatus() == Ok) {
                g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                
                // Calculate aspect fit
                float imgRatio = (float)img->GetWidth() / img->GetHeight();
                float areaRatio = (float)imgAreaW / imgAreaH;
                
                int drawW, drawH, drawX, drawY;
                if (imgRatio > areaRatio) {
                    drawW = imgAreaW;
                    drawH = (int)(imgAreaW / imgRatio);
                    drawX = imgAreaX;
                    drawY = imgAreaY + (imgAreaH - drawH) / 2;
                } else {
                    drawH = imgAreaH;
                    drawW = (int)(imgAreaH * imgRatio);
                    drawX = imgAreaX + (imgAreaW - drawW) / 2;
                    drawY = imgAreaY;
                }
                
                // Rounded clip
                GraphicsPath clipPath;
                int imgRadius = 12;
                clipPath.AddArc(drawX, drawY, imgRadius * 2, imgRadius * 2, 180, 90);
                clipPath.AddArc(drawX + drawW - imgRadius * 2, drawY, imgRadius * 2, imgRadius * 2, 270, 90);
                clipPath.AddArc(drawX + drawW - imgRadius * 2, drawY + drawH - imgRadius * 2, imgRadius * 2, imgRadius * 2, 0, 90);
                clipPath.AddArc(drawX, drawY + drawH - imgRadius * 2, imgRadius * 2, imgRadius * 2, 90, 90);
                clipPath.CloseFigure();
                
                // Shadow under image
                DrawShadow(g, drawX, drawY, drawW, drawH, imgRadius, 12);
                
                Region oldClip;
                g.GetClip(&oldClip);
                g.SetClip(&clipPath);
                g.DrawImage(img, drawX, drawY, drawW, drawH);
                g.SetClip(&oldClip);
                
                delete img;
            }
        } else {
            // Placeholder
            SolidBrush placeBg(Colors::BgDark);
            DrawRoundedRect(g, imgAreaX, imgAreaY, imgAreaW, imgAreaH, 12, &placeBg);
            
            Font iconFont(L"Segoe UI Emoji", 48);
            SolidBrush iconBrush(Colors::TextTertiary);
            StringFormat iconFmt;
            iconFmt.SetAlignment(StringAlignmentCenter);
            iconFmt.SetLineAlignment(StringAlignmentCenter);
            RectF iconRect((REAL)imgAreaX, (REAL)imgAreaY, (REAL)imgAreaW, (REAL)imgAreaH);
            g.DrawString(L"🎮", -1, &iconFont, iconRect, &iconFmt, &iconBrush);
        }
        
        // Title area background
        LinearGradientBrush titleAreaBg(
            Point(x, y + h - 85), Point(x, y + h),
            Color(0, 28, 32, 40),
            Color(200, 20, 24, 30)
        );
        g.FillRectangle(&titleAreaBg, x + 8, y + h - 85, w - 16, 75);
        
        // Game title with shadow
        Font titleFont(L"Segoe UI", 15, FontStyleBold);
        SolidBrush shadowBrush(Color(100, 0, 0, 0));
        StringFormat titleFmt;
        titleFmt.SetAlignment(StringAlignmentCenter);
        titleFmt.SetTrimming(StringTrimmingEllipsisCharacter);
        
        RectF shadowRect((REAL)(x + padding + 1), (REAL)(y + h - 68), (REAL)(w - padding * 2), 28);
        g.DrawString(game.displayName.c_str(), -1, &titleFont, shadowRect, &titleFmt, &shadowBrush);
        
        SolidBrush titleBrush(Colors::TextPrimary);
        RectF titleRect((REAL)(x + padding), (REAL)(y + h - 69), (REAL)(w - padding * 2), 28);
        g.DrawString(game.displayName.c_str(), -1, &titleFont, titleRect, &titleFmt, &titleBrush);
        
        // Status with pulsing dot effect (static for now)
        Font statusFont(L"Segoe UI", 10);
        
        // Draw status badge
        int badgeX = x + w/2 - 60;
        int badgeY = y + h - 38;
        
        // Glow behind dot
        for (int i = 4; i > 0; i--) {
            SolidBrush dotGlow(Color(40 - i * 10, 46, 204, 113));
            g.FillEllipse(&dotGlow, badgeX - i, badgeY + 4 - i/2, 10 + i * 2, 10 + i);
        }
        
        // Green dot
        SolidBrush dotBrush(Colors::AccentGreen);
        g.FillEllipse(&dotBrush, badgeX, badgeY + 4, 10, 10);
        
        // Dot shine
        SolidBrush dotShine(Color(80, 255, 255, 255));
        g.FillEllipse(&dotShine, badgeX + 2, badgeY + 5, 4, 4);
        
        // Status text
        SolidBrush statusBrush(Colors::TextSecondary);
        g.DrawString(L"Ready to play", -1, &statusFont, PointF((REAL)(badgeX + 16), (REAL)(badgeY + 2)), &statusBrush);
        
    } else {
        // No selection
        Font promptFont(L"Segoe UI", 12);
        SolidBrush promptBrush(Colors::TextTertiary);
        StringFormat promptFmt;
        promptFmt.SetAlignment(StringAlignmentCenter);
        promptFmt.SetLineAlignment(StringAlignmentCenter);
        RectF promptRect((REAL)x, (REAL)y, (REAL)w, (REAL)h);
        g.DrawString(L"Select a game to preview", -1, &promptFont, promptRect, &promptFmt, &promptBrush);
    }
}

// Draw stats bar at bottom
void DrawStatsBar(Graphics& g, int x, int y, int w, int h, int total, int filtered) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    // Background with subtle gradient
    LinearGradientBrush bgBrush(
        Point(x, y), Point(x, y + h),
        Color(255, 20, 24, 30),
        Color(255, 16, 20, 26)
    );
    g.FillRectangle(&bgBrush, x, y, w, h);
    
    // Top divider with gradient
    LinearGradientBrush divBrush(
        Point(x, y), Point(x + w, y),
        Color(60, 88, 166, 255),
        Color(60, 139, 92, 246)
    );
    g.FillRectangle(&divBrush, x, y, w, 1);
    
    // Left - Games counter with icon
    Font statsFont(L"Segoe UI", 10);
    SolidBrush accentBrush(Colors::AccentPrimary);
    g.DrawString(L"📁", -1, &statsFont, PointF((REAL)(x + 20), (REAL)(y + 10)), &accentBrush);
    
    SolidBrush statsBrush(Colors::TextSecondary);
    wstring statsText = to_wstring(filtered) + L" of " + to_wstring(total) + L" games";
    g.DrawString(statsText.c_str(), -1, &statsFont, PointF((REAL)(x + 44), (REAL)(y + 11)), &statsBrush);
    
    // Right side - keyboard hint
    Font hintFont(L"Segoe UI", 9);
    SolidBrush hintBrush(Colors::TextTertiary);
    StringFormat rightFmt;
    rightFmt.SetAlignment(StringAlignmentFar);
    RectF hintRect((REAL)x, (REAL)(y + 11), (REAL)(w - 24), 20);
    g.DrawString(L"⏎ Double-click to play", -1, &hintFont, hintRect, &rightFmt, &hintBrush);
}

// Copy local image from repo
bool CopyLocalImage(const wstring& gameName, const wstring& savePath) {
    wstring searchName;
    for (size_t i = 0; i < gameName.length(); i++) {
        wchar_t c = gameName[i];
        if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9')) {
            if (c >= L'A' && c <= L'Z') searchName += (c + 32);
            else searchName += c;
        }
    }
    
    wstring localPath = localImagesFolder + L"\\" + searchName + L".png";
    DWORD attrs = GetFileAttributesW(localPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        if (CopyFileW(localPath.c_str(), savePath.c_str(), FALSE)) return true;
    }
    
    WIN32_FIND_DATAW findData;
    wstring searchPattern = localImagesFolder + L"\\" + searchName + L"*.png";
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        wstring foundPath = localImagesFolder + L"\\" + findData.cFileName;
        FindClose(hFind);
        if (CopyFileW(foundPath.c_str(), savePath.c_str(), FALSE)) return true;
    }
    
    searchPattern = localImagesFolder + L"\\*.png";
    hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            wstring fileName = findData.cFileName;
            wstring lowerFileName;
            for (size_t i = 0; i < fileName.length(); i++) {
                wchar_t c = fileName[i];
                if (c >= L'A' && c <= L'Z') lowerFileName += (c + 32);
                else lowerFileName += c;
            }
            if (lowerFileName.find(searchName) != wstring::npos) {
                wstring foundPath = localImagesFolder + L"\\" + findData.cFileName;
                FindClose(hFind);
                if (CopyFileW(foundPath.c_str(), savePath.c_str(), FALSE)) return true;
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
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

void UpdateListView() {
    InvalidateRect(hListView, NULL, FALSE);
    InvalidateRect(hImageWnd, NULL, FALSE);
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
    listScrollPos = 0;
    UpdateListView();
}

void LaunchGame(int index) {
    if (index >= 0 && index < (int)filteredGames.size()) {
        ShellExecuteW(NULL, L"open", filteredGames[index].exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}

// List view proc - with debounced hover for no flicker
static int pendingHover = -1;
static UINT_PTR hoverTimerId = 0;

LRESULT CALLBACK ListViewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static bool isDragging = false;
    static int dragStartY = 0, dragStartScroll = 0;
    
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;  // We handle all painting, prevent flicker
        
        case WM_TIMER:
            if (wParam == 1) {
                KillTimer(hWnd, 1);
                hoverTimerId = 0;
                if (pendingHover != hoveredItem) {
                    hoveredItem = pendingHover;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            return 0;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBitmap);
            
            Graphics g(memDC);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            
            // Background
            SolidBrush bgBrush(Colors::BgMain);
            g.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
            
            // Draw items
            int y = -listScrollPos;
            for (size_t i = 0; i < filteredGames.size(); i++) {
                if (y + itemHeight > 0 && y < rc.bottom) {
                    DrawGameItem(g, 0, y, rc.right - 10, itemHeight, filteredGames[i],
                        (int)i == selectedIndex, (int)i == hoveredItem, (int)i);
                }
                y += itemHeight;
            }
            
            // Premium scrollbar
            int contentH = (int)filteredGames.size() * itemHeight;
            if (contentH > rc.bottom) {
                int trackX = rc.right - 8;
                int trackW = 6;
                int trackH = rc.bottom - 16;
                int trackY = 8;
                
                // Track with gradient
                LinearGradientBrush trackBrush(
                    Point(trackX, trackY), Point(trackX + trackW, trackY),
                    Color(30, 255, 255, 255),
                    Color(15, 255, 255, 255)
                );
                DrawRoundedRect(g, trackX, trackY, trackW, trackH, 3, &trackBrush);
                
                // Thumb
                int thumbH = (rc.bottom * trackH) / contentH;
                if (thumbH < 40) thumbH = 40;
                int maxScroll = contentH - rc.bottom;
                if (maxScroll < 1) maxScroll = 1;
                int thumbY = trackY + (listScrollPos * (trackH - thumbH)) / maxScroll;
                
                // Thumb with accent gradient
                LinearGradientBrush thumbBrush(
                    Point(trackX, thumbY), Point(trackX, thumbY + thumbH),
                    Color(180, 88, 166, 255),
                    Color(180, 139, 92, 246)
                );
                DrawRoundedRect(g, trackX, thumbY, trackW, thumbH, 3, &thumbBrush);
                
                // Thumb highlight
                SolidBrush highlightBrush(Color(40, 255, 255, 255));
                g.FillRectangle(&highlightBrush, trackX + 1, thumbY + 2, trackW - 2, 3);
            }
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            int newHovered = (pt.y + listScrollPos) / itemHeight;
            if (newHovered < 0 || newHovered >= (int)filteredGames.size()) newHovered = -1;
            
            // Debounce hover updates - only redraw after mouse stops moving
            if (newHovered != pendingHover) {
                pendingHover = newHovered;
                if (hoverTimerId) KillTimer(hWnd, 1);
                hoverTimerId = SetTimer(hWnd, 1, 50, NULL);  // 50ms delay
            }
            
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hWnd, 0};
            TrackMouseEvent(&tme);
            
            if (isDragging) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                int delta = dragStartY - pt.y;
                int maxScroll = (int)filteredGames.size() * itemHeight - rc.bottom;
                if (maxScroll < 0) maxScroll = 0;
                listScrollPos = dragStartScroll + delta;
                if (listScrollPos < 0) listScrollPos = 0;
                if (listScrollPos > maxScroll) listScrollPos = maxScroll;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_MOUSELEAVE: {
            if (hoverTimerId) {
                KillTimer(hWnd, 1);
                hoverTimerId = 0;
            }
            pendingHover = -1;
            if (hoveredItem >= 0) {
                hoveredItem = -1;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            int clicked = (pt.y + listScrollPos) / itemHeight;
            if (clicked >= 0 && clicked < (int)filteredGames.size()) {
                selectedIndex = clicked;
                InvalidateRect(hWnd, NULL, FALSE);
                InvalidateRect(hImageWnd, NULL, FALSE);
            }
            dragStartY = pt.y;
            dragStartScroll = listScrollPos;
            isDragging = true;
            SetCapture(hWnd);
            return 0;
        }
        
        case WM_LBUTTONUP:
            isDragging = false;
            ReleaseCapture();
            return 0;
        
        case WM_LBUTTONDBLCLK: {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            int clicked = (pt.y + listScrollPos) / itemHeight;
            if (clicked >= 0 && clicked < (int)filteredGames.size()) {
                LaunchGame(clicked);
            }
            return 0;
        }
        
        case WM_MOUSEWHEEL: {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int maxScroll = (int)filteredGames.size() * itemHeight - rc.bottom;
            if (maxScroll < 0) maxScroll = 0;
            listScrollPos = listScrollPos - delta / 2;
            if (listScrollPos < 0) listScrollPos = 0;
            if (listScrollPos > maxScroll) listScrollPos = maxScroll;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Preview window proc
LRESULT CALLBACK PreviewWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;  // We handle all painting, prevent flicker
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBitmap);
            
            Graphics g(memDC);
            SolidBrush bgBrush(Colors::BgMain);
            g.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
            
            DrawPreviewPanel(g, 0, 0, rc.right, rc.bottom);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Button drawing
LRESULT CALLBACK ButtonProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static bool isHovered = false;
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBitmap);
            
            Graphics g(memDC);
            SolidBrush bgBrush(Colors::BgMain);
            g.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
            
            wchar_t text[64];
            GetWindowTextW(hWnd, text, 64);
            
            bool isPrimary = (dwRefData == 1);
            DrawGradientButton(g, 0, 0, rc.right, rc.bottom, text, isPrimary, isHovered);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBitmap);
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
            InvalidateRect(hWnd, NULL, FALSE);
            break;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// Main window proc
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hStatsLabel;
    
    switch (msg) {
        case WM_CREATE: {
            // Dark mode
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(hWnd, 20, &darkMode, sizeof(darkMode));
            
            // Fonts
            hTitleFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            hMainFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            hSmallFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            
            // Register list class with double-buffer support
            WNDCLASSW listClass = {0};
            listClass.lpfnWndProc = ListViewProc;
            listClass.hInstance = GetModuleHandle(NULL);
            listClass.lpszClassName = L"GameListView";
            listClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            listClass.style = CS_DBLCLKS | CS_OWNDC;
            listClass.hbrBackground = NULL;  // We handle all painting
            RegisterClassW(&listClass);
            
            // Register preview class with double-buffer support
            WNDCLASSW prevClass = {0};
            prevClass.lpfnWndProc = PreviewWndProc;
            prevClass.hInstance = GetModuleHandle(NULL);
            prevClass.lpszClassName = L"GamePreviewView";
            prevClass.hCursor = LoadCursor(NULL, IDC_ARROW);
            prevClass.style = CS_OWNDC;
            prevClass.hbrBackground = NULL;
            RegisterClassW(&prevClass);
            
            // Search box
            hSearchBox = CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                72, 80, 340, 34, hWnd, (HMENU)1, NULL, NULL);
            SendMessageW(hSearchBox, WM_SETFONT, (WPARAM)hMainFont, TRUE);
            
            // List view
            hListView = CreateWindowW(L"GameListView", L"",
                WS_CHILD | WS_VISIBLE,
                24, 135, 410, 445, hWnd, NULL, NULL, NULL);
            
            // Preview
            hImageWnd = CreateWindowW(L"GamePreviewView", L"",
                WS_CHILD | WS_VISIBLE,
                450, 80, 420, 430, hWnd, NULL, NULL, NULL);
            
            // Buttons
            hPlayBtn = CreateWindowW(L"BUTTON", L"▶  Play Game",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                450, 525, 200, 54, hWnd, (HMENU)2, NULL, NULL);
            SetWindowSubclass(hPlayBtn, ButtonProc, 0, 1);
            
            hRefreshBtn = CreateWindowW(L"BUTTON", L"↻  Refresh",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                665, 525, 205, 54, hWnd, (HMENU)3, NULL, NULL);
            SetWindowSubclass(hRefreshBtn, ButtonProc, 0, 0);
            
            // Setup
            cacheFolder = L"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\cache";
            CreateDirectoryW(cacheFolder.c_str(), NULL);
            
            // Sync images
            STARTUPINFOW si = {0};
            PROCESS_INFORMATION pi = {0};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            wchar_t cmdLine[] = L"powershell.exe -ExecutionPolicy Bypass -File \"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\sync-images.ps1\" -Silent";
            if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, 15000);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            
            ScanGames();
            UpdateListView();
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(memDC, memBitmap);
            
            Graphics g(memDC);
            
            // Main background
            SolidBrush bgBrush(Colors::BgMain);
            g.FillRectangle(&bgBrush, 0, 0, rc.right, rc.bottom);
            
            // Header
            DrawHeader(g, rc.right);
            
            // Search box background - get actual position
            RECT searchRc;
            GetWindowRect(hSearchBox, &searchRc);
            MapWindowPoints(NULL, hWnd, (LPPOINT)&searchRc, 2);
            DrawSearchBox(g, searchRc.left - 48, searchRc.top - 3, searchRc.right - searchRc.left + 50, searchRc.bottom - searchRc.top + 6, GetFocus() == hSearchBox);
            
            // Stats bar
            DrawStatsBar(g, 0, rc.bottom - 44, rc.right, 44, (int)games.size(), (int)filteredGames.size());
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(240, 246, 252));
            SetBkColor(hdcEdit, RGB(33, 38, 45));
            static HBRUSH hEditBrush = CreateSolidBrush(RGB(33, 38, 45));
            return (LRESULT)hEditBrush;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1 && HIWORD(wParam) == EN_CHANGE) {
                wchar_t buffer[256];
                GetWindowTextW(hSearchBox, buffer, 256);
                FilterGames(buffer);
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (LOWORD(wParam) == 2) {
                if (selectedIndex >= 0) LaunchGame(selectedIndex);
            } else if (LOWORD(wParam) == 3) {
                // Refresh
                STARTUPINFOW si = {0};
                PROCESS_INFORMATION pi = {0};
                si.cb = sizeof(si);
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;
                wchar_t cmdLine[] = L"powershell.exe -ExecutionPolicy Bypass -File \"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\sync-images.ps1\" -Silent";
                if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                    WaitForSingleObject(pi.hProcess, 10000);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
                SetWindowTextW(hSearchBox, L"");
                selectedIndex = -1;
                listScrollPos = 0;
                ScanGames();
                UpdateListView();
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
        }
        
        case WM_SIZE: {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int w = rc.right;
            int h = rc.bottom;
            
            // Calculate responsive layout
            int listW = (w < 1000) ? (w * 45 / 100) : (w - 480);
            int previewX = listW + 40;
            int previewW = w - previewX - 20;
            int listH = h - 200;
            int previewH = h - 175;
            int btnY = h - 90;
            int btnW = (previewW - 20) / 2;
            
            // Reposition controls
            SetWindowPos(hSearchBox, NULL, 72, 80, listW - 50, 34, SWP_NOZORDER);
            SetWindowPos(hListView, NULL, 24, 135, listW, listH, SWP_NOZORDER);
            SetWindowPos(hImageWnd, NULL, previewX, 80, previewW, previewH, SWP_NOZORDER);
            SetWindowPos(hPlayBtn, NULL, previewX, btnY, btnW, 54, SWP_NOZORDER);
            SetWindowPos(hRefreshBtn, NULL, previewX + btnW + 20, btnY, btnW, 54, SWP_NOZORDER);
            
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 800;
            mmi->ptMinTrackSize.y = 550;
            return 0;
        }
        
        case WM_DESTROY:
            if (hTitleFont) DeleteObject(hTitleFont);
            if (hMainFont) DeleteObject(hMainFont);
            if (hSmallFont) DeleteObject(hSmallFont);
            GdiplusShutdown(gdiplusToken);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"GameLauncherPro";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(22, 27, 34));
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassW(&wc);
    
    // Center window on screen
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 900;
    int winH = 660;
    int winX = (screenW - winW) / 2;
    int winY = (screenH - winH) / 2;
    
    hMainWnd = CreateWindowExW(
        WS_EX_COMPOSITED,
        L"GameLauncherPro",
        L"Game Library",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,  // Prevent child flicker
        winX, winY,
        winW, winH,
        NULL, NULL, hInstance, NULL
    );
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    
    return (int)message.wParam;
}

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
// ULTRA PREMIUM DESIGN SYSTEM - Steam/Epic Inspired
// ═══════════════════════════════════════════════════════════════════════════════

namespace DS {
    // Core palette - Deep midnight theme
    namespace C {
        // Backgrounds - layered depth
        const Color Bg0(255, 6, 8, 12);             // Deepest void
        const Color Bg1(255, 12, 15, 21);           // Base layer
        const Color Bg2(255, 18, 22, 30);           // Surface
        const Color Bg3(255, 26, 31, 42);           // Elevated
        const Color Bg4(255, 35, 41, 54);           // Raised
        const Color BgHover(255, 42, 49, 65);       // Hover state
        
        // Accent system - vibrant neon
        const Color Accent1(255, 94, 106, 210);     // Primary indigo
        const Color Accent2(255, 147, 87, 229);     // Secondary purple
        const Color Accent3(255, 239, 68, 166);     // Tertiary pink
        const Color AccentCyan(255, 34, 211, 238);  // Highlight cyan
        const Color AccentGreen(255, 52, 211, 153); // Success
        const Color AccentAmber(255, 251, 191, 36); // Warning
        
        // Text - clarity hierarchy
        const Color T1(255, 250, 251, 253);         // Brightest
        const Color T2(255, 186, 195, 209);         // Secondary
        const Color T3(255, 125, 137, 156);         // Muted
        const Color T4(255, 78, 91, 113);           // Disabled
        
        // Borders
        const Color B1(255, 45, 52, 68);            // Subtle
        const Color B2(255, 62, 71, 90);            // Visible
        const Color BAccent(100, 94, 106, 210);     // Accent border
    }
    
    // Spacing scale (4px base)
    namespace S {
        const int U1 = 4, U2 = 8, U3 = 12, U4 = 16, U5 = 20, U6 = 24, U7 = 32, U8 = 40, U9 = 48, U10 = 64;
    }
    
    // Radius scale
    namespace R {
        const int XS = 4, S = 6, M = 10, L = 14, XL = 18, XXL = 24, Full = 9999;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// APPLICATION STATE
// ═══════════════════════════════════════════════════════════════════════════════

struct Game {
    wstring name;
    wstring displayName;
    wstring exePath;
    wstring imagePath;
    bool hasImage;
};

vector<Game> games;
vector<Game> filteredGames;
HWND hMainWnd, hListPanel, hPreviewPanel, hSearchEdit;
HWND hPlayBtn, hRefreshBtn;
HFONT hFontUI;
ULONG_PTR gdiplusToken;
wstring gamesFolder = L"E:\\games";
wstring cacheFolder;
wstring localImagesFolder = L"C:\\Users\\micha\\.openclaw\\workspace-moltbot\\game-library-manager-web\\public\\images";
int selectedIdx = -1;
int hoverIdx = -1;
int scrollY = 0;
int cardHeight = 80;  // Larger cards for better visuals
bool isSearchFocused = false;

// ═══════════════════════════════════════════════════════════════════════════════
// PREMIUM DRAWING UTILITIES
// ═══════════════════════════════════════════════════════════════════════════════

void RoundRect(Graphics& g, int x, int y, int w, int h, int r, const Brush* fill, const Pen* stroke = nullptr) {
    GraphicsPath p;
    int d = r * 2;
    if (d > h) d = h;
    if (d > w) d = w;
    p.AddArc(x, y, d, d, 180, 90);
    p.AddArc(x + w - d, y, d, d, 270, 90);
    p.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    p.AddArc(x, y + h - d, d, d, 90, 90);
    p.CloseFigure();
    if (fill) g.FillPath(fill, &p);
    if (stroke) g.DrawPath(stroke, &p);
}

// Premium soft shadow with multiple layers
void SoftShadow(Graphics& g, int x, int y, int w, int h, int r, int intensity = 12, int offsetY = 4) {
    for (int i = intensity; i > 0; i -= 2) {
        int alpha = min(40, 50 - (i * 4));
        if (alpha > 0) {
            SolidBrush sb(Color(alpha, 0, 0, 0));
            RoundRect(g, x - i/2, y + offsetY + i/2, w + i, h + i/3, r + i/4, &sb);
        }
    }
}

// Glow effect (for accents and highlights)
void GlowEffect(Graphics& g, int x, int y, int w, int h, int r, Color glowColor, int intensity = 8) {
    for (int i = intensity; i > 0; i -= 2) {
        int alpha = min(50, 60 - (i * 6));
        if (alpha > 0) {
            SolidBrush gb(Color(alpha, glowColor.GetR(), glowColor.GetG(), glowColor.GetB()));
            RoundRect(g, x - i, y - i/2, w + i * 2, h + i, r + i/2, &gb);
        }
    }
}

// Glass panel with blur effect simulation
void GlassPanel(Graphics& g, int x, int y, int w, int h, int r, bool elevated = false) {
    // Shadow
    SoftShadow(g, x, y, w, h, r, elevated ? 16 : 10, elevated ? 6 : 3);
    
    // Main background with subtle gradient
    LinearGradientBrush bg(
        Point(x, y), Point(x, y + h),
        Color(elevated ? 240 : 220, 20, 24, 33),
        Color(elevated ? 250 : 235, 14, 17, 24)
    );
    RoundRect(g, x, y, w, h, r, &bg);
    
    // Top edge highlight
    LinearGradientBrush shine(Point(x, y), Point(x, y + 60),
        Color(18, 255, 255, 255), Color(0, 255, 255, 255));
    
    GraphicsPath shp;
    int d = r * 2;
    shp.AddArc(x, y, d, d, 180, 90);
    shp.AddArc(x + w - d, y, d, d, 270, 90);
    shp.AddLine(x + w - r, y + r, x + w - r, y + 50);
    shp.AddLine(x + w - r, y + 50, x + r, y + 50);
    shp.AddLine(x + r, y + 50, x + r, y + r);
    shp.CloseFigure();
    g.FillPath(&shine, &shp);
    
    // Border
    Pen bp(Color(50, 255, 255, 255), 1);
    RoundRect(g, x, y, w, h, r, nullptr, &bp);
}

// ═══════════════════════════════════════════════════════════════════════════════
// UI COMPONENT RENDERERS
// ═══════════════════════════════════════════════════════════════════════════════

// Header with premium branding
void RenderHeader(Graphics& g, int w, int h) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    // Header gradient background
    LinearGradientBrush hbg(Point(0, 0), Point(0, h),
        Color(255, 10, 12, 18), DS::C::Bg1);
    g.FillRectangle(&hbg, 0, 0, w, h);
    
    // Premium accent bar - animated gradient feel
    LinearGradientBrush accent(Point(0, 0), Point(w, 0),
        DS::C::Accent1, DS::C::Accent3);
    g.FillRectangle(&accent, 0, 0, w, 3);
    
    // Accent glow under bar
    for (int i = 0; i < 15; i++) {
        int a = 30 - i * 2;
        if (a > 0) {
            LinearGradientBrush gl(Point(0, 3 + i), Point(w, 3 + i),
                Color(a, 94, 106, 210), Color(a, 239, 68, 166));
            g.FillRectangle(&gl, 0, 3 + i, w, 1);
        }
    }
    
    // Logo icon container with glow
    int iconX = 28, iconY = 26;
    GlowEffect(g, iconX - 4, iconY - 4, 48, 48, 24, DS::C::Accent1, 8);
    
    LinearGradientBrush iconBg(Point(iconX, iconY), Point(iconX, iconY + 40),
        Color(255, 30, 35, 48), DS::C::Bg3);
    g.FillEllipse(&iconBg, iconX, iconY, 40, 40);
    
    // Icon border glow
    Pen iconBorder(Color(80, 94, 106, 210), 2);
    g.DrawEllipse(&iconBorder, iconX, iconY, 40, 40);
    
    // Game controller emoji
    Font iconFont(L"Segoe UI Emoji", 16);
    SolidBrush iconBrush(DS::C::AccentCyan);
    StringFormat cf;
    cf.SetAlignment(StringAlignmentCenter);
    cf.SetLineAlignment(StringAlignmentCenter);
    RectF ir((REAL)iconX, (REAL)iconY, 40, 40);
    g.DrawString(L"🎮", -1, &iconFont, ir, &cf, &iconBrush);
    
    // Title with subtle shadow
    Font titleFont(L"Segoe UI", 20, FontStyleBold);
    SolidBrush titleShadow(Color(80, 0, 0, 0));
    g.DrawString(L"GAME LIBRARY", -1, &titleFont, PointF(83, 29), &titleShadow);
    SolidBrush titleBrush(DS::C::T1);
    g.DrawString(L"GAME LIBRARY", -1, &titleFont, PointF(82, 28), &titleBrush);
    
    // Subtitle with accent
    Font subFont(L"Segoe UI", 9);
    SolidBrush subBrush(DS::C::T3);
    wstring sub = to_wstring(games.size()) + L" titles in your collection";
    g.DrawString(sub.c_str(), -1, &subFont, PointF(84, 54), &subBrush);
    
    // Online indicator
    SolidBrush dotBrush(DS::C::AccentGreen);
    g.FillEllipse(&dotBrush, 84, 60, 6, 6);
}

// Premium search field
void RenderSearchBox(Graphics& g, int x, int y, int w, int h, bool focused) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    
    int r = DS::R::L;
    
    // Focus glow
    if (focused) {
        GlowEffect(g, x, y, w, h, r, DS::C::Accent1, 10);
    } else {
        SoftShadow(g, x, y, w, h, r, 6, 2);
    }
    
    // Background
    SolidBrush bg(DS::C::Bg3);
    RoundRect(g, x, y, w, h, r, &bg);
    
    // Inner shadow for depth
    LinearGradientBrush inner(Point(x, y), Point(x, y + 12),
        Color(20, 0, 0, 0), Color(0, 0, 0, 0));
    RoundRect(g, x + 2, y + 2, w - 4, 10, r - 2, &inner);
    
    // Border
    Pen borderPen(focused ? DS::C::Accent1 : DS::C::B1, focused ? 2.0f : 1.0f);
    RoundRect(g, x, y, w, h, r, nullptr, &borderPen);
    
    // Search icon
    Font iconFont(L"Segoe UI Symbol", 12);
    SolidBrush iconBrush(focused ? DS::C::Accent1 : DS::C::T3);
    g.DrawString(L"🔍", -1, &iconFont, PointF((REAL)(x + 14), (REAL)(y + h/2 - 10)), &iconBrush);
}

// Premium button
void RenderButton(Graphics& g, int x, int y, int w, int h, const wchar_t* text, 
                  bool primary, bool hovered, bool pressed) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int r = DS::R::L;
    
    if (primary) {
        // Primary button with gradient and glow
        if (hovered && !pressed) {
            GlowEffect(g, x, y, w, h, r, DS::C::Accent1, 12);
        } else {
            SoftShadow(g, x, y, w, h, r, 8, 3);
        }
        
        // Gradient fill
        Color c1 = pressed ? Color(255, 74, 86, 190) : (hovered ? Color(255, 114, 126, 230) : DS::C::Accent1);
        Color c2 = pressed ? Color(255, 127, 67, 209) : (hovered ? Color(255, 167, 107, 249) : DS::C::Accent2);
        LinearGradientBrush grad(Point(x, y), Point(x + w, y + h), c1, c2);
        RoundRect(g, x, y, w, h, r, &grad);
        
        // Shine overlay
        LinearGradientBrush shine(Point(x, y), Point(x, y + h/2),
            Color(pressed ? 15 : 35, 255, 255, 255), Color(0, 255, 255, 255));
        GraphicsPath sp;
        sp.AddArc(x + 2, y + 2, r * 2 - 2, r * 2 - 2, 180, 90);
        sp.AddArc(x + w - r * 2 + 2, y + 2, r * 2 - 2, r * 2 - 2, 270, 90);
        sp.AddLine(x + w - r, y + r, x + w - r, y + h/3);
        sp.AddLine(x + w - r, y + h/3, x + r, y + h/3);
        sp.AddLine(x + r, y + h/3, x + r, y + r);
        sp.CloseFigure();
        g.FillPath(&shine, &sp);
        
    } else {
        // Secondary button
        SoftShadow(g, x, y, w, h, r, 6, 2);
        
        SolidBrush bg(pressed ? DS::C::Bg2 : (hovered ? DS::C::BgHover : DS::C::Bg3));
        RoundRect(g, x, y, w, h, r, &bg);
        
        Pen border(hovered ? DS::C::BAccent : DS::C::B1, hovered ? 1.5f : 1.0f);
        RoundRect(g, x, y, w, h, r, nullptr, &border);
    }
    
    // Text
    Font font(L"Segoe UI Semibold", 11);
    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentCenter);
    fmt.SetLineAlignment(StringAlignmentCenter);
    
    if (primary) {
        SolidBrush shadow(Color(50, 0, 0, 0));
        RectF sr((REAL)(x + 1), (REAL)(y + 1), (REAL)w, (REAL)h);
        g.DrawString(text, -1, &font, sr, &fmt, &shadow);
    }
    
    SolidBrush tb(DS::C::T1);
    RectF tr((REAL)x, (REAL)y, (REAL)w, (REAL)h);
    g.DrawString(text, -1, &font, tr, &fmt, &tb);
}

// Premium game card
void RenderGameCard(Graphics& g, int x, int y, int w, int h, const Game& game,
                    bool selected, bool hovered, int idx) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int r = DS::R::M;
    int pad = 10;
    int thumbW = 55, thumbH = 55;
    int thumbX = x + pad + 8;
    int thumbY = y + (h - thumbH) / 2;
    
    // Card effects based on state
    if (selected) {
        // Selected glow
        GlowEffect(g, x + 6, y + 2, w - 12, h - 4, r, DS::C::Accent1, 10);
        
        // Selected background
        LinearGradientBrush selBg(Point(x, y), Point(x + w, y + h),
            Color(60, 94, 106, 210), Color(35, 147, 87, 229));
        RoundRect(g, x + 6, y + 2, w - 12, h - 4, r, &selBg);
        
        // Accent bar
        LinearGradientBrush bar(Point(x + 6, y + 12), Point(x + 6, y + h - 12),
            DS::C::Accent1, DS::C::Accent2);
        g.FillRectangle(&bar, x + 6, y + 15, 4, h - 30);
        
    } else if (hovered) {
        // Hover background
        SolidBrush hoverBg(DS::C::Bg4);
        RoundRect(g, x + 6, y + 2, w - 12, h - 4, r, &hoverBg);
        
        // Subtle border
        Pen hoverBorder(Color(40, 94, 106, 210), 1);
        RoundRect(g, x + 6, y + 2, w - 12, h - 4, r, nullptr, &hoverBorder);
    }
    
    // Thumbnail container
    thumbX += (selected || hovered) ? 4 : 0;
    
    if (selected || hovered) {
        SoftShadow(g, thumbX, thumbY, thumbW, thumbH, 8, 8, 3);
    }
    
    // Thumbnail clip path
    GraphicsPath tp;
    int tr = 8;
    tp.AddArc(thumbX, thumbY, tr * 2, tr * 2, 180, 90);
    tp.AddArc(thumbX + thumbW - tr * 2, thumbY, tr * 2, tr * 2, 270, 90);
    tp.AddArc(thumbX + thumbW - tr * 2, thumbY + thumbH - tr * 2, tr * 2, tr * 2, 0, 90);
    tp.AddArc(thumbX, thumbY + thumbH - tr * 2, tr * 2, tr * 2, 90, 90);
    tp.CloseFigure();
    
    if (game.hasImage) {
        Image* img = Image::FromFile(game.imagePath.c_str());
        if (img && img->GetLastStatus() == Ok) {
            Region oldClip;
            g.GetClip(&oldClip);
            g.SetClip(&tp);
            g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            g.DrawImage(img, thumbX, thumbY, thumbW, thumbH);
            g.SetClip(&oldClip);
            delete img;
        }
    } else {
        // Placeholder
        LinearGradientBrush pb(Point(thumbX, thumbY), Point(thumbX, thumbY + thumbH),
            DS::C::Bg4, DS::C::Bg2);
        g.FillPath(&pb, &tp);
        
        // Icon
        Font pf(L"Segoe UI Emoji", 22);
        SolidBrush pi(DS::C::T4);
        StringFormat pcf;
        pcf.SetAlignment(StringAlignmentCenter);
        pcf.SetLineAlignment(StringAlignmentCenter);
        RectF pr((REAL)thumbX, (REAL)thumbY, (REAL)thumbW, (REAL)thumbH);
        g.DrawString(L"🎮", -1, &pf, pr, &pcf, &pi);
    }
    
    // Thumbnail border
    Pen thumbBorder(Color(35, 255, 255, 255), 1);
    g.DrawPath(&thumbBorder, &tp);
    
    // Game title
    int textX = thumbX + thumbW + 16;
    int textW = w - textX - pad - 55;
    
    Font nameFont(L"Segoe UI", 12, selected ? FontStyleBold : FontStyleRegular);
    SolidBrush nameBrush(selected ? DS::C::T1 : (hovered ? DS::C::T1 : DS::C::T2));
    
    StringFormat nfmt;
    nfmt.SetTrimming(StringTrimmingEllipsisCharacter);
    nfmt.SetFormatFlags(StringFormatFlagsNoWrap);
    nfmt.SetLineAlignment(StringAlignmentCenter);
    
    RectF nr((REAL)textX, (REAL)(y + 8), (REAL)textW, (REAL)(h - 16));
    g.DrawString(game.displayName.c_str(), -1, &nameFont, nr, &nfmt, &nameBrush);
    
    // Play indicator for selected
    if (selected) {
        int indX = x + w - 50;
        int indY = y + h/2 - 13;
        
        // Glow
        GlowEffect(g, indX, indY, 26, 26, 13, DS::C::AccentGreen, 6);
        
        // Circle
        SolidBrush indBg(Color(80, 52, 211, 153));
        g.FillEllipse(&indBg, indX, indY, 26, 26);
        
        // Play triangle
        SolidBrush playBrush(DS::C::AccentGreen);
        PointF tri[3] = {
            PointF((REAL)(indX + 10), (REAL)(indY + 7)),
            PointF((REAL)(indX + 10), (REAL)(indY + 19)),
            PointF((REAL)(indX + 19), (REAL)(indY + 13))
        };
        g.FillPolygon(&playBrush, tri, 3);
    }
}

// Premium preview panel with hero image
void RenderPreview(Graphics& g, int x, int y, int w, int h) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    int r = DS::R::XL;
    int pad = DS::S::U6;
    
    // Glass panel
    GlassPanel(g, x, y, w, h, r, true);
    
    // Corner accents
    Pen cp(Color(50, 94, 106, 210), 2);
    int cl = 25;
    // TL
    g.DrawLine(&cp, x + 14, y + 14, x + 14, y + 14 + cl);
    g.DrawLine(&cp, x + 14, y + 14, x + 14 + cl, y + 14);
    // TR
    g.DrawLine(&cp, x + w - 14, y + 14, x + w - 14, y + 14 + cl);
    g.DrawLine(&cp, x + w - 14, y + 14, x + w - 14 - cl, y + 14);
    // BL
    g.DrawLine(&cp, x + 14, y + h - 14, x + 14, y + h - 14 - cl);
    g.DrawLine(&cp, x + 14, y + h - 14, x + 14 + cl, y + h - 14);
    // BR
    g.DrawLine(&cp, x + w - 14, y + h - 14, x + w - 14, y + h - 14 - cl);
    g.DrawLine(&cp, x + w - 14, y + h - 14, x + w - 14 - cl, y + h - 14);
    
    int imgX = x + pad;
    int imgY = y + pad;
    int imgW = w - pad * 2;
    int imgH = h - pad * 2 - 110;
    
    if (selectedIdx >= 0 && selectedIdx < (int)filteredGames.size()) {
        const Game& game = filteredGames[selectedIdx];
        
        if (game.hasImage) {
            Image* img = Image::FromFile(game.imagePath.c_str());
            if (img && img->GetLastStatus() == Ok) {
                // Calculate aspect-fit
                float ir = (float)img->GetWidth() / (float)img->GetHeight();
                float ar = (float)imgW / (float)imgH;
                
                int dw, dh, dx, dy;
                if (ir > ar) {
                    dw = imgW; dh = (int)(imgW / ir);
                    dx = imgX; dy = imgY + (imgH - dh) / 2;
                } else {
                    dh = imgH; dw = (int)(imgH * ir);
                    dx = imgX + (imgW - dw) / 2; dy = imgY;
                }
                
                // Shadow
                SoftShadow(g, dx, dy, dw, dh, DS::R::L, 18, 8);
                
                // Rounded clip
                GraphicsPath ip;
                int ipr = DS::R::L;
                ip.AddArc(dx, dy, ipr * 2, ipr * 2, 180, 90);
                ip.AddArc(dx + dw - ipr * 2, dy, ipr * 2, ipr * 2, 270, 90);
                ip.AddArc(dx + dw - ipr * 2, dy + dh - ipr * 2, ipr * 2, ipr * 2, 0, 90);
                ip.AddArc(dx, dy + dh - ipr * 2, ipr * 2, ipr * 2, 90, 90);
                ip.CloseFigure();
                
                Region oc;
                g.GetClip(&oc);
                g.SetClip(&ip);
                g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                g.DrawImage(img, dx, dy, dw, dh);
                g.SetClip(&oc);
                
                // Border
                Pen ib(Color(30, 255, 255, 255), 1);
                g.DrawPath(&ib, &ip);
                
                delete img;
            }
        } else {
            // Placeholder
            SolidBrush pb(DS::C::Bg0);
            RoundRect(g, imgX, imgY, imgW, imgH, DS::R::L, &pb);
            
            Font pf(L"Segoe UI Emoji", 52);
            SolidBrush pi(DS::C::T4);
            StringFormat pcf;
            pcf.SetAlignment(StringAlignmentCenter);
            pcf.SetLineAlignment(StringAlignmentCenter);
            RectF pr((REAL)imgX, (REAL)imgY, (REAL)imgW, (REAL)imgH);
            g.DrawString(L"🎮", -1, &pf, pr, &pcf, &pi);
        }
        
        // Info area with fade
        int infoY = y + h - 100;
        LinearGradientBrush fade(Point(x, infoY - 40), Point(x, y + h - 16),
            Color(0, 14, 17, 24), Color(230, 14, 17, 24));
        g.FillRectangle(&fade, x + 14, infoY - 40, w - 28, h - infoY + 24);
        
        // Title with shadow
        Font tf(L"Segoe UI", 17, FontStyleBold);
        StringFormat tfmt;
        tfmt.SetAlignment(StringAlignmentCenter);
        tfmt.SetTrimming(StringTrimmingEllipsisCharacter);
        
        SolidBrush tsh(Color(100, 0, 0, 0));
        RectF tsr((REAL)(x + pad + 1), (REAL)(infoY + 1), (REAL)(w - pad * 2), 35);
        g.DrawString(game.displayName.c_str(), -1, &tf, tsr, &tfmt, &tsh);
        
        SolidBrush tb(DS::C::T1);
        RectF ttr((REAL)(x + pad), (REAL)infoY, (REAL)(w - pad * 2), 35);
        g.DrawString(game.displayName.c_str(), -1, &tf, ttr, &tfmt, &tb);
        
        // Status badge
        int bx = x + w/2 - 60;
        int by = infoY + 44;
        
        // Badge bg
        SolidBrush bbg(Color(50, 52, 211, 153));
        RoundRect(g, bx, by, 120, 28, DS::R::Full, &bbg);
        
        // Pulsing dot
        GlowEffect(g, bx + 12, by + 9, 10, 10, 5, DS::C::AccentGreen, 4);
        SolidBrush dot(DS::C::AccentGreen);
        g.FillEllipse(&dot, bx + 12, by + 9, 10, 10);
        SolidBrush dotShine(Color(90, 255, 255, 255));
        g.FillEllipse(&dotShine, bx + 14, by + 10, 4, 4);
        
        // Status text
        Font sf(L"Segoe UI", 9);
        SolidBrush st(DS::C::T2);
        g.DrawString(L"Ready to launch", -1, &sf, PointF((REAL)(bx + 28), (REAL)(by + 6)), &st);
        
    } else {
        // Empty state
        Font ef(L"Segoe UI", 13);
        SolidBrush eb(DS::C::T4);
        StringFormat efmt;
        efmt.SetAlignment(StringAlignmentCenter);
        efmt.SetLineAlignment(StringAlignmentCenter);
        RectF er((REAL)x, (REAL)y, (REAL)w, (REAL)h);
        g.DrawString(L"Select a game to preview", -1, &ef, er, &efmt, &eb);
    }
}

// Status bar
void RenderStatusBar(Graphics& g, int x, int y, int w, int h) {
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    
    // Background
    SolidBrush bg(Color(255, 8, 10, 14));
    g.FillRectangle(&bg, x, y, w, h);
    
    // Top gradient line
    LinearGradientBrush tl(Point(x, y), Point(x + w, y),
        Color(70, 94, 106, 210), Color(70, 147, 87, 229));
    g.FillRectangle(&tl, x, y, w, 1);
    
    // Stats
    Font sf(L"Segoe UI", 10);
    SolidBrush ab(DS::C::Accent1);
    g.DrawString(L"📁", -1, &sf, PointF((REAL)(x + 22), (REAL)(y + 14)), &ab);
    
    SolidBrush stb(DS::C::T3);
    wstring stats = to_wstring(filteredGames.size()) + L" of " + to_wstring(games.size()) + L" games";
    g.DrawString(stats.c_str(), -1, &sf, PointF((REAL)(x + 46), (REAL)(y + 15)), &stb);
    
    // Hint
    Font hf(L"Segoe UI", 9);
    SolidBrush hb(DS::C::T4);
    StringFormat rfmt;
    rfmt.SetAlignment(StringAlignmentFar);
    RectF hr((REAL)x, (REAL)(y + 15), (REAL)(w - 22), 20);
    g.DrawString(L"Double-click or press Enter to play", -1, &hf, hr, &rfmt, &hb);
}

// Scrollbar
void RenderScrollbar(Graphics& g, int x, int y, int w, int h, int contentH, int viewH, int scroll) {
    if (contentH <= viewH) return;
    
    int tX = x + w - 10;
    int tY = y + 10;
    int tH = h - 20;
    int tW = 6;
    
    // Track
    SolidBrush tb(Color(25, 255, 255, 255));
    RoundRect(g, tX, tY, tW, tH, 3, &tb);
    
    // Thumb
    int thumbH = max(45, (viewH * tH) / contentH);
    int maxS = contentH - viewH;
    int thumbY = tY + (scroll * (tH - thumbH)) / max(1, maxS);
    
    LinearGradientBrush thb(Point(tX, thumbY), Point(tX, thumbY + thumbH),
        Color(200, 94, 106, 210), Color(200, 147, 87, 229));
    RoundRect(g, tX, thumbY, tW, thumbH, 3, &thb);
    
    // Shine
    SolidBrush sh(Color(50, 255, 255, 255));
    g.FillRectangle(&sh, tX + 1, thumbY + 4, tW - 2, 5);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GAME LOGIC
// ═══════════════════════════════════════════════════════════════════════════════

bool CopyLocalImage(const wstring& gameName, const wstring& savePath) {
    wstring searchName;
    for (size_t i = 0; i < gameName.length(); i++) {
        wchar_t c = gameName[i];
        if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9')) {
            searchName += (c >= L'A' && c <= L'Z') ? (c + 32) : c;
        }
    }
    
    // Exact match
    wstring lp = localImagesFolder + L"\\" + searchName + L".png";
    if (GetFileAttributesW(lp.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (CopyFileW(lp.c_str(), savePath.c_str(), FALSE)) return true;
    }
    
    // Partial match
    WIN32_FIND_DATAW fd;
    wstring sp = localImagesFolder + L"\\*" + searchName + L"*.png";
    HANDLE hf = FindFirstFileW(sp.c_str(), &fd);
    if (hf != INVALID_HANDLE_VALUE) {
        wstring fp = localImagesFolder + L"\\" + fd.cFileName;
        FindClose(hf);
        if (CopyFileW(fp.c_str(), savePath.c_str(), FALSE)) return true;
    }
    
    return false;
}

void LoadGameImage(Game& game) {
    wstring safeName = game.name;
    for (wchar_t& c : safeName) {
        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || 
            c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') c = L'_';
    }
    game.imagePath = cacheFolder + L"\\" + safeName + L".jpg";
    
    if (GetFileAttributesW(game.imagePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        game.hasImage = true;
        return;
    }
    game.hasImage = CopyLocalImage(game.name, game.imagePath);
}

void ScanGames() {
    games.clear();
    
    WIN32_FIND_DATAW fd;
    wstring sp = gamesFolder + L"\\*";
    HANDLE hf = FindFirstFileW(sp.c_str(), &fd);
    if (hf == INVALID_HANDLE_VALUE) return;
    
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            wstring name = fd.cFileName;
            if (name != L"." && name != L"..") {
                wstring gp = gamesFolder + L"\\" + name;
                
                WIN32_FIND_DATAW ed;
                wstring es = gp + L"\\*.exe";
                HANDLE he = FindFirstFileW(es.c_str(), &ed);
                
                if (he != INVALID_HANDLE_VALUE) {
                    Game g;
                    g.name = name;
                    g.displayName = name;
                    g.exePath = gp + L"\\" + ed.cFileName;
                    g.hasImage = false;
                    LoadGameImage(g);
                    games.push_back(g);
                    FindClose(he);
                }
            }
        }
    } while (FindNextFileW(hf, &fd));
    FindClose(hf);
    
    sort(games.begin(), games.end(), [](const Game& a, const Game& b) {
        return _wcsicmp(a.displayName.c_str(), b.displayName.c_str()) < 0;
    });
    
    filteredGames = games;
}

void FilterGames(const wstring& query) {
    filteredGames.clear();
    wstring lq = query;
    transform(lq.begin(), lq.end(), lq.begin(), ::tolower);
    
    for (const auto& g : games) {
        wstring ln = g.displayName;
        transform(ln.begin(), ln.end(), ln.begin(), ::tolower);
        if (lq.empty() || ln.find(lq) != wstring::npos) {
            filteredGames.push_back(g);
        }
    }
    selectedIdx = -1;
    scrollY = 0;
}

void LaunchGame(int idx) {
    if (idx >= 0 && idx < (int)filteredGames.size()) {
        ShellExecuteW(NULL, L"open", filteredGames[idx].exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// WINDOW PROCEDURES
// ═══════════════════════════════════════════════════════════════════════════════

LRESULT CALLBACK ListWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int dragY = 0, dragScroll = 0;
    static bool dragging = false;
    
    switch (msg) {
        case WM_ERASEBKGND: return 1;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc; GetClientRect(hWnd, &rc);
            
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(mem, bmp);
            
            Graphics g(mem);
            g.SetSmoothingMode(SmoothingModeHighQuality);
            
            SolidBrush bg(DS::C::Bg1);
            g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
            
            int y = -scrollY;
            for (size_t i = 0; i < filteredGames.size(); i++) {
                if (y + cardHeight > 0 && y < rc.bottom) {
                    RenderGameCard(g, 0, y, rc.right - 14, cardHeight, filteredGames[i],
                        (int)i == selectedIdx, (int)i == hoverIdx, (int)i);
                }
                y += cardHeight;
            }
            
            int contentH = (int)filteredGames.size() * cardHeight;
            RenderScrollbar(g, 0, 0, rc.right, rc.bottom, contentH, rc.bottom, scrollY);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int nh = (pt.y + scrollY) / cardHeight;
            if (nh < 0 || nh >= (int)filteredGames.size()) nh = -1;
            
            if (nh != hoverIdx) {
                hoverIdx = nh;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            
            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hWnd, 0};
            TrackMouseEvent(&tme);
            
            if (dragging) {
                RECT rc; GetClientRect(hWnd, &rc);
                int delta = dragY - pt.y;
                int maxS = max(0, (int)((int)filteredGames.size() * cardHeight - rc.bottom));
                scrollY = max(0, min(maxS, dragScroll + delta));
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_MOUSELEAVE:
            if (hoverIdx >= 0) { hoverIdx = -1; InvalidateRect(hWnd, NULL, FALSE); }
            return 0;
        
        case WM_LBUTTONDOWN: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int c = (pt.y + scrollY) / cardHeight;
            if (c >= 0 && c < (int)filteredGames.size()) {
                selectedIdx = c;
                InvalidateRect(hWnd, NULL, FALSE);
                InvalidateRect(hPreviewPanel, NULL, FALSE);
            }
            dragY = pt.y; dragScroll = scrollY; dragging = true;
            SetCapture(hWnd);
            return 0;
        }
        
        case WM_LBUTTONUP: dragging = false; ReleaseCapture(); return 0;
        
        case WM_LBUTTONDBLCLK: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            int c = (pt.y + scrollY) / cardHeight;
            if (c >= 0 && c < (int)filteredGames.size()) LaunchGame(c);
            return 0;
        }
        
        case WM_MOUSEWHEEL: {
            RECT rc; GetClientRect(hWnd, &rc);
            int d = GET_WHEEL_DELTA_WPARAM(wParam);
            int maxS = max(0, (int)((int)filteredGames.size() * cardHeight - rc.bottom));
            scrollY = max(0, min(maxS, scrollY - d / 2));
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        
        case WM_KEYDOWN:
            if (wParam == VK_UP && selectedIdx > 0) {
                selectedIdx--;
                if (selectedIdx * cardHeight < scrollY) scrollY = selectedIdx * cardHeight;
                InvalidateRect(hWnd, NULL, FALSE);
                InvalidateRect(hPreviewPanel, NULL, FALSE);
            } else if (wParam == VK_DOWN && selectedIdx < (int)filteredGames.size() - 1) {
                selectedIdx++;
                RECT rc; GetClientRect(hWnd, &rc);
                if ((selectedIdx + 1) * cardHeight > scrollY + rc.bottom)
                    scrollY = (selectedIdx + 1) * cardHeight - rc.bottom;
                InvalidateRect(hWnd, NULL, FALSE);
                InvalidateRect(hPreviewPanel, NULL, FALSE);
            } else if (wParam == VK_RETURN && selectedIdx >= 0) {
                LaunchGame(selectedIdx);
            }
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK PreviewWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND: return 1;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc; GetClientRect(hWnd, &rc);
            
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(mem, bmp);
            
            Graphics g(mem);
            SolidBrush bg(DS::C::Bg1);
            g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
            RenderPreview(g, 0, 0, rc.right, rc.bottom);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK BtnSubProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, 
                            UINT_PTR uId, DWORD_PTR dwRef) {
    static bool hover = false, press = false;
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc; GetClientRect(hWnd, &rc);
            
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(mem, bmp);
            
            Graphics g(mem);
            SolidBrush bg(DS::C::Bg1);
            g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
            
            wchar_t txt[64]; GetWindowTextW(hWnd, txt, 64);
            RenderButton(g, 0, 0, rc.right, rc.bottom, txt, dwRef == 1, hover, press);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_MOUSEMOVE:
            if (!hover) { hover = true; InvalidateRect(hWnd, NULL, FALSE);
                TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hWnd, 0}; TrackMouseEvent(&tme); }
            break;
        
        case WM_MOUSELEAVE: hover = false; press = false; InvalidateRect(hWnd, NULL, FALSE); break;
        case WM_LBUTTONDOWN: press = true; InvalidateRect(hWnd, NULL, FALSE); break;
        case WM_LBUTTONUP: press = false; InvalidateRect(hWnd, NULL, FALSE); break;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            BOOL dm = TRUE;
            DwmSetWindowAttribute(hWnd, 20, &dm, sizeof(dm));
            int mica = 2;
            DwmSetWindowAttribute(hWnd, 38, &mica, sizeof(mica));
            
            // Register list class
            WNDCLASSW lc = {0};
            lc.lpfnWndProc = ListWndProc;
            lc.hInstance = GetModuleHandle(NULL);
            lc.lpszClassName = L"UltraList";
            lc.hCursor = LoadCursor(NULL, IDC_ARROW);
            lc.style = CS_DBLCLKS | CS_OWNDC;
            RegisterClassW(&lc);
            
            // Register preview class
            WNDCLASSW pc = {0};
            pc.lpfnWndProc = PreviewWndProc;
            pc.hInstance = GetModuleHandle(NULL);
            pc.lpszClassName = L"UltraPreview";
            pc.hCursor = LoadCursor(NULL, IDC_ARROW);
            RegisterClassW(&pc);
            
            // Search
            hSearchEdit = CreateWindowExW(0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                92, 100, 330, 32, hWnd, (HMENU)1, NULL, NULL);
            hFontUI = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            SendMessageW(hSearchEdit, WM_SETFONT, (WPARAM)hFontUI, TRUE);
            
            // List
            hListPanel = CreateWindowW(L"UltraList", L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                24, 155, 440, 420, hWnd, NULL, NULL, NULL);
            
            // Preview
            hPreviewPanel = CreateWindowW(L"UltraPreview", L"",
                WS_CHILD | WS_VISIBLE,
                485, 100, 405, 405, hWnd, NULL, NULL, NULL);
            
            // Buttons
            hPlayBtn = CreateWindowW(L"BUTTON", L"▶  PLAY NOW",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                485, 520, 190, 58, hWnd, (HMENU)2, NULL, NULL);
            SetWindowSubclass(hPlayBtn, BtnSubProc, 0, 1);
            
            hRefreshBtn = CreateWindowW(L"BUTTON", L"↻  REFRESH",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                695, 520, 195, 58, hWnd, (HMENU)3, NULL, NULL);
            SetWindowSubclass(hRefreshBtn, BtnSubProc, 0, 0);
            
            // Setup
            cacheFolder = L"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\cache";
            CreateDirectoryW(cacheFolder.c_str(), NULL);
            
            // Sync images
            STARTUPINFOW si = {sizeof(si)};
            PROCESS_INFORMATION pi = {0};
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            wchar_t cmd[] = L"powershell.exe -ExecutionPolicy Bypass -File \"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\sync-images.ps1\" -Silent";
            if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
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
            RECT rc; GetClientRect(hWnd, &rc);
            
            HDC mem = CreateCompatibleDC(hdc);
            HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            SelectObject(mem, bmp);
            
            Graphics g(mem);
            g.SetSmoothingMode(SmoothingModeHighQuality);
            
            SolidBrush bg(DS::C::Bg1);
            g.FillRectangle(&bg, 0, 0, rc.right, rc.bottom);
            
            RenderHeader(g, rc.right, 85);
            
            RECT sr; GetWindowRect(hSearchEdit, &sr);
            MapWindowPoints(NULL, hWnd, (LPPOINT)&sr, 2);
            RenderSearchBox(g, sr.left - 52, sr.top - 6, sr.right - sr.left + 56, sr.bottom - sr.top + 12, isSearchFocused);
            
            RenderStatusBar(g, 0, rc.bottom - 50, rc.right, 50);
            
            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
            DeleteObject(bmp);
            DeleteDC(mem);
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(250, 251, 253));
            SetBkColor(hdc, RGB(26, 31, 42));
            static HBRUSH hBr = CreateSolidBrush(RGB(26, 31, 42));
            return (LRESULT)hBr;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 1 && HIWORD(wParam) == EN_CHANGE) {
                wchar_t buf[256]; GetWindowTextW(hSearchEdit, buf, 256);
                FilterGames(buf);
                InvalidateRect(hListPanel, NULL, FALSE);
                InvalidateRect(hPreviewPanel, NULL, FALSE);
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (LOWORD(wParam) == 1 && (HIWORD(wParam) == EN_SETFOCUS || HIWORD(wParam) == EN_KILLFOCUS)) {
                isSearchFocused = (HIWORD(wParam) == EN_SETFOCUS);
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (LOWORD(wParam) == 2) {
                if (selectedIdx >= 0) LaunchGame(selectedIdx);
            } else if (LOWORD(wParam) == 3) {
                STARTUPINFOW si = {sizeof(si)};
                PROCESS_INFORMATION pi = {0};
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;
                wchar_t cmd[] = L"powershell.exe -ExecutionPolicy Bypass -File \"F:\\study\\Dev_Toolchain\\programming\\C++\\projects\\game-launcher\\sync-images.ps1\" -Silent";
                if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                    WaitForSingleObject(pi.hProcess, 10000);
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
                SetWindowTextW(hSearchEdit, L"");
                selectedIdx = -1; scrollY = 0;
                ScanGames();
                InvalidateRect(hWnd, NULL, TRUE);
                InvalidateRect(hListPanel, NULL, FALSE);
                InvalidateRect(hPreviewPanel, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_SIZE: {
            RECT rc; GetClientRect(hWnd, &rc);
            int w = rc.right, h = rc.bottom;
            
            int listW = min(500, (w - 70) * 48 / 100);
            int prevX = listW + 50;
            int prevW = w - prevX - 24;
            int listH = h - 210;
            int prevH = h - 180;
            int btnY = h - 105;
            int btnW = (prevW - 20) / 2;
            
            SetWindowPos(hSearchEdit, NULL, 92, 100, listW - 70, 32, SWP_NOZORDER);
            SetWindowPos(hListPanel, NULL, 24, 155, listW, listH, SWP_NOZORDER);
            SetWindowPos(hPreviewPanel, NULL, prevX, 100, prevW, prevH, SWP_NOZORDER);
            SetWindowPos(hPlayBtn, NULL, prevX, btnY, btnW, 58, SWP_NOZORDER);
            SetWindowPos(hRefreshBtn, NULL, prevX + btnW + 20, btnY, btnW, 58, SWP_NOZORDER);
            
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 880;
            mmi->ptMinTrackSize.y = 620;
            return 0;
        }
        
        case WM_DESTROY:
            if (hFontUI) DeleteObject(hFontUI);
            GdiplusShutdown(gdiplusToken);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ENTRY POINT
// ═══════════════════════════════════════════════════════════════════════════════

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    GdiplusStartupInput gsi;
    GdiplusStartup(&gdiplusToken, &gsi, NULL);
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"UltraGameLauncher";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(12, 15, 21));
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);
    
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int ww = 980, wh = 720;
    int wx = (sw - ww) / 2, wy = (sh - wh) / 2;
    
    hMainWnd = CreateWindowExW(
        WS_EX_COMPOSITED,
        L"UltraGameLauncher",
        L"GAME LIBRARY",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        wx, wy, ww, wh,
        NULL, NULL, hInst, NULL
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

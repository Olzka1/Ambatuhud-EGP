#pragma once

#include <windows.h>

// Defined in main.cpp
extern HFONT g_hUIFont;
extern HFONT g_hBoldFont;
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include "model.hpp"
#include "ui_helpers.hpp"

extern std::vector<DrawShape> g_shapes;
extern int g_selectedShapeIndex;
extern bool g_isDrawing;
extern ToolType g_currentTool;
extern COLORREF g_currentColor;
extern int g_currentThickness;
extern int g_currentTextSize;
extern std::string g_currentText;
extern bool g_currentIsInput;
extern POINT g_startPt, g_endPt, g_mousePos;
extern std::string g_logMessage;
extern ViewMode g_currentView;
extern int g_bgMode;
extern HBITMAP g_hBgBitmap;
extern bool g_darkTheme;
extern bool g_showGrid;

inline void DrawTopWhiteHeaderShadow(HDC hdc, int winWidth) {
    RECT topHeaderRect = { 0, 0, winWidth, 42 };
    HBRUSH headerBrush = CreateSolidBrush(g_darkTheme ? RGB(41, 41, 41) : RGB(255, 255, 255));
    FillRect(hdc, &topHeaderRect, headerBrush);
    DeleteObject(headerBrush);

    HPEN linePen = CreatePen(PS_SOLID, 1, g_darkTheme ? RGB(76, 79, 85) : RGB(218, 220, 224));
    HPEN oldPen = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, 0, 42, NULL); LineTo(hdc, winWidth, 42);

    COLORREF shadowColors[] = { g_darkTheme ? RGB(52, 55, 61) : RGB(225, 225, 225), g_darkTheme ? RGB(50, 53, 58) : RGB(238, 238, 238), g_darkTheme ? RGB(47, 50, 54) : RGB(245, 245, 245) };
    for (int i = 0; i < 3; i++) {
        HPEN shadowPen = CreatePen(PS_SOLID, 1, shadowColors[i]);
        SelectObject(hdc, shadowPen);
        MoveToEx(hdc, 0, 43 + i, NULL); LineTo(hdc, winWidth, 43 + i);
        DeleteObject(shadowPen);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
}

inline void DrawTopToolbarSeparators(HDC hdc, RECT cRect) {
    // Small neutral dividers keep the left management buttons and right background/interval controls visually grouped.
    HPEN pen = CreatePen(PS_SOLID, 1, g_darkTheme ? RGB(80, 83, 89) : RGB(205, 205, 210));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);

    int leftDividerX = cRect.left + 117;
    MoveToEx(hdc, leftDividerX, cRect.top - 29, NULL);
    LineTo(hdc, leftDividerX, cRect.top - 2);

    int rightDividerX = cRect.right - 103;
    MoveToEx(hdc, rightDividerX, cRect.top - 29, NULL);
    LineTo(hdc, rightDividerX, cRect.top - 2);

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

inline void DrawCanvasGrid(HDC hdc, RECT cRect) {
    if (g_bgMode != 0 && g_hBgBitmap) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, g_hBgBitmap);
        BITMAP bm;
        GetObject(g_hBgBitmap, sizeof(bm), &bm);
        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc, cRect.left, cRect.top, cRect.right - cRect.left, cRect.bottom - cRect.top,
                   hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        SelectObject(hdcMem, hOldBm);
        DeleteDC(hdcMem);
    } else {
        COLORREF bg = (g_bgMode == 3) ? RGB(30, 30, 30) : (g_darkTheme ? RGB(48, 50, 54) : RGB(248, 248, 250));
        HBRUSH bgCanvas = CreateSolidBrush(bg);
        FillRect(hdc, &cRect, bgCanvas);
        DeleteObject(bgCanvas);
    }

    COLORREF gridColor = (g_bgMode == 3) ? RGB(57, 57, 57) : (g_darkTheme ? RGB(68, 68, 68) : RGB(230, 230, 235));
    HPEN gridPen = CreatePen(PS_SOLID, 1, gridColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);

    if (g_showGrid) {
        for (int x = 0; x <= 1920; x += 40) {
            POINT pTop = CanvasToScreen({ (double)x, 0 }, cRect);
            POINT pBot = CanvasToScreen({ (double)x, 1080 }, cRect);
            MoveToEx(hdc, pTop.x, pTop.y, NULL); LineTo(hdc, pBot.x, pBot.y);
        }
        for (int y = 0; y <= 1080; y += 40) {
            POINT pL = CanvasToScreen({ 0, (double)y }, cRect);
            POINT pR = CanvasToScreen({ 1920, (double)y }, cRect);
            MoveToEx(hdc, pL.x, pL.y, NULL); LineTo(hdc, pR.x, pR.y);
        }
    }
    // Center X/Y axes: short, closely-spaced dashed marks only.
    // There is no solid line underneath. The origin itself is a clean '+'.
    COLORREF centerColor = (g_bgMode == 3) ? RGB(105, 105, 105)
                       : (g_darkTheme ? RGB(118, 118, 118) : RGB(150, 150, 150));
    LOGBRUSH dashBrush = { BS_SOLID, centerColor, 0 };
    DWORD dashPattern[2] = { 2, 2 };
    HPEN centerPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 1, &dashBrush, 2, dashPattern);
    HPEN oldCenterPen = (HPEN)SelectObject(hdc, centerPen);
    POINT origin = CanvasToScreen({ 960, 540 }, cRect);
    POINT cTop = CanvasToScreen({ 960, 0 }, cRect);
    POINT cBottom = CanvasToScreen({ 960, 1080 }, cRect);
    POINT cLeft = CanvasToScreen({ 0, 540 }, cRect);
    POINT cRight = CanvasToScreen({ 1920, 540 }, cRect);

    // PS_DOT is intentionally used without any continuous segment.
    MoveToEx(hdc, cTop.x, cTop.y, NULL); LineTo(hdc, origin.x, origin.y);
    MoveToEx(hdc, origin.x, origin.y, NULL); LineTo(hdc, cBottom.x, cBottom.y);
    MoveToEx(hdc, cLeft.x, cLeft.y, NULL); LineTo(hdc, origin.x, origin.y);
    MoveToEx(hdc, origin.x, origin.y, NULL); LineTo(hdc, cRight.x, cRight.y);
    SelectObject(hdc, oldCenterPen);

    // Exact '+' at the canvas origin, with no gap.
    HPEN originPen = CreatePen(PS_SOLID, 1, centerColor);
    HPEN oldOriginPen = (HPEN)SelectObject(hdc, originPen);
    MoveToEx(hdc, origin.x - 4, origin.y, NULL); LineTo(hdc, origin.x + 5, origin.y);
    MoveToEx(hdc, origin.x, origin.y - 4, NULL); LineTo(hdc, origin.x, origin.y + 5);
    SelectObject(hdc, oldOriginPen);
    DeleteObject(originPen);

    HPEN borderPen = CreatePen(PS_SOLID, 1, (g_bgMode == 3) ? RGB(85, 85, 85) : (g_darkTheme ? RGB(100, 100, 100) : RGB(100, 100, 100)));
    SelectObject(hdc, borderPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, cRect.left, cRect.top, cRect.right, cRect.bottom);

    SelectObject(hdc, oldPen);
    DeleteObject(gridPen); DeleteObject(centerPen); DeleteObject(borderPen);
}

// ===== การหมุน (Angle) แบบ EGP: หมุนตามเข็มนาฬิกาเมื่อค่าเป็นบวก =====
// หมุน shape รอบจุดหมุนของตัวเอง (box/rect = จุดกึ่งกลาง, อื่นๆ = จุดแรก) แล้วให้ parent หมุนซ้ำทั้งชุดตามลำดับ
inline Point RotateAbout(Point p, Point pivot, double deg) {
    const double rad = deg * 3.14159265358979323846 / 180.0;
    const double cs = cos(rad), sn = sin(rad);
    const double dx = p.first - pivot.first, dy = p.second - pivot.second;
    return { pivot.first + dx * cs - dy * sn, pivot.second + dx * sn + dy * cs };
}

inline Point ShapePivotCanvas(int idx) {
    const DrawShape& s = g_shapes[idx];
    Point a1 = GetAbsolutePosition(idx);
    if (s.type == TOOL_BOX || s.type == TOOL_RECT) {
        return { a1.first + (s.p2.first - s.p1.first) / 2.0, a1.second + (s.p2.second - s.p1.second) / 2.0 };
    }
    return a1;
}

// จุด P (พิกัด canvas ก่อนหมุน) -> ตำแหน่งจริงหลังผ่านการหมุนของ shape นี้และ parent ทั้งสาย
inline Point TransformCanvasPoint(int idx, Point P, int depth = 0) {
    if (idx < 0 || idx >= (int)g_shapes.size() || depth > 50) return P;
    const DrawShape& s = g_shapes[idx];
    if (std::fabs(s.angle) > 0.0001) P = RotateAbout(P, ShapePivotCanvas(idx), s.angle);
    return TransformCanvasPoint(s.parentIndex, P, depth + 1);
}

// มุมรวมของ shape (รวมมุมของ parent ทั้งสาย)
inline double TotalShapeAngle(int idx) {
    double total = 0.0;
    int depth = 0;
    while (idx >= 0 && idx < (int)g_shapes.size() && depth < 50) {
        total += g_shapes[idx].angle;
        idx = g_shapes[idx].parentIndex;
        depth++;
    }
    return total;
}

inline POINT RotateScreenOffset(POINT origin, double dx, double dy, double deg) {
    const double rad = deg * 3.14159265358979323846 / 180.0;
    const double cs = cos(rad), sn = sin(rad);
    return POINT{ origin.x + (LONG)lround(dx * cs - dy * sn), origin.y + (LONG)lround(dx * sn + dy * cs) };
}

inline void DrawShapes(HDC hdc, RECT cRect) {
    for (size_t i = 0; i < g_shapes.size(); i++) {
        const auto& s = g_shapes[i];

        Point absP1 = GetAbsolutePosition((int)i);
        Point absP2 = { absP1.first + (s.p2.first - s.p1.first), absP1.second + (s.p2.second - s.p1.second) };

        POINT s1 = CanvasToScreen(absP1, cRect);
        POINT s2 = CanvasToScreen(absP2, cRect);

        const double totalAngle = TotalShapeAngle((int)i);
        if (std::fabs(totalAngle) > 0.0001) {
            // ---------- เส้นทางวาดแบบหมุน ----------
            auto T = [&](Point P) { return CanvasToScreen(TransformCanvasPoint((int)i, P), cRect); };
            double minCx = std::min(absP1.first, absP2.first), maxCx = std::max(absP1.first, absP2.first);
            double minCy = std::min(absP1.second, absP2.second), maxCy = std::max(absP1.second, absP2.second);

            std::vector<POINT> hull;   // ใช้วางชื่อและกรอบเลือก
            HPEN rPen = CreatePen(PS_SOLID, s.thickness, s.color);
            HPEN rOldPen = (HPEN)SelectObject(hdc, rPen);

            if (s.type == TOOL_LINE) {
                POINT a = T(absP1), b = T(absP2);
                MoveToEx(hdc, a.x, a.y, NULL); LineTo(hdc, b.x, b.y);
                hull.push_back(a); hull.push_back(b);
            } else if (s.type == TOOL_BOX || s.type == TOOL_RECT) {
                POINT q[4] = { T({minCx, minCy}), T({maxCx, minCy}), T({maxCx, maxCy}), T({minCx, maxCy}) };
                HBRUSH rb = (s.type == TOOL_BOX) ? CreateSolidBrush(s.color) : (HBRUSH)GetStockObject(NULL_BRUSH);
                HBRUSH rOldB = (HBRUSH)SelectObject(hdc, rb);
                Polygon(hdc, q, 4);
                SelectObject(hdc, rOldB);
                if (s.type == TOOL_BOX) DeleteObject(rb);
                for (int k = 0; k < 4; k++) hull.push_back(q[k]);
            } else if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) {
                POINT ctr = T(absP1);
                int r = (int)sqrt(pow(s2.x - s1.x, 2) + pow(s2.y - s1.y, 2));
                HBRUSH rb = (s.type == TOOL_CIRCLE_SOLID) ? CreateSolidBrush(s.color) : (HBRUSH)GetStockObject(NULL_BRUSH);
                HBRUSH rOldB = (HBRUSH)SelectObject(hdc, rb);
                Ellipse(hdc, ctr.x - r, ctr.y - r, ctr.x + r, ctr.y + r);
                SelectObject(hdc, rOldB);
                if (s.type == TOOL_CIRCLE_SOLID) DeleteObject(rb);
                hull.push_back(POINT{ctr.x - r, ctr.y - r}); hull.push_back(POINT{ctr.x + r, ctr.y - r});
                hull.push_back(POINT{ctr.x + r, ctr.y + r}); hull.push_back(POINT{ctr.x - r, ctr.y + r});
            } else if (s.type == TOOL_TEXT) {
                POINT origin = T(absP1);
                LOGFONTA lf = {};
                lf.lfHeight = s.textSize;
                lf.lfWeight = FW_NORMAL;
                lf.lfCharSet = ANSI_CHARSET;
                lf.lfEscapement = -(LONG)lround(totalAngle * 10.0);   // GDI: บวก = ทวนเข็ม, EGP: บวก = ตามเข็ม
                lf.lfOrientation = lf.lfEscapement;
                strcpy(lf.lfFaceName, "Tahoma");
                HFONT rf = CreateFontIndirectA(&lf);
                HFONT rOldF = (HFONT)SelectObject(hdc, rf);
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, s.color);
                SIZE ts;
                GetTextExtentPoint32A(hdc, s.textContent.c_str(), (int)s.textContent.size(), &ts);
                TextOutA(hdc, origin.x, origin.y, s.textContent.c_str(), (int)s.textContent.size());
                if (s.isInput) {
                    POINT sp = RotateScreenOffset(origin, ts.cx + 2, -2, totalAngle);
                    SetTextColor(hdc, RGB(220, 38, 38));
                    TextOutA(hdc, sp.x, sp.y, "*", 1);
                }
                SelectObject(hdc, rOldF);
                DeleteObject(rf);
                hull.push_back(RotateScreenOffset(origin, -3, -3, totalAngle));
                hull.push_back(RotateScreenOffset(origin, ts.cx + 3, -3, totalAngle));
                hull.push_back(RotateScreenOffset(origin, ts.cx + 3, s.textSize + 3, totalAngle));
                hull.push_back(RotateScreenOffset(origin, -3, s.textSize + 3, totalAngle));
            }
            SelectObject(hdc, rOldPen);
            DeleteObject(rPen);

            // ชื่อ shape (มุมซ้ายบนของกรอบที่หมุนแล้ว)
            if (!hull.empty()) {
                LONG nx = hull[0].x, ny = hull[0].y;
                for (size_t k = 1; k < hull.size(); k++) { nx = std::min(nx, hull[k].x); ny = std::min(ny, hull[k].y); }
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(160, 160, 160));
                HFONT nf = CreateFontA(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
                HFONT nof = (HFONT)SelectObject(hdc, nf);
                TextOutA(hdc, nx, ny - 13, s.name.c_str(), (int)s.name.size());
                SelectObject(hdc, nof);
                DeleteObject(nf);
            }

            // กรอบเลือก (หมุนตาม shape)
            if ((int)i == g_selectedShapeIndex && hull.size() >= 2) {
                HPEN selPen = CreatePen(PS_DOT, 1, RGB(0, 120, 215));
                HPEN selOld = (HPEN)SelectObject(hdc, selPen);
                HBRUSH selOldB = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                if (hull.size() >= 4) {
                    Polygon(hdc, hull.data(), (int)hull.size());
                } else {
                    MoveToEx(hdc, hull[0].x, hull[0].y, NULL); LineTo(hdc, hull[1].x, hull[1].y);
                }
                SelectObject(hdc, selOldB);
                SelectObject(hdc, selOld);
                DeleteObject(selPen);
            }
            continue;
        }

        HPEN hPen = CreatePen(PS_SOLID, s.thickness, s.color);
        SelectObject(hdc, hPen);

        if (s.type == TOOL_LINE) {
            MoveToEx(hdc, s1.x, s1.y, NULL); LineTo(hdc, s2.x, s2.y);
        } else if (s.type == TOOL_BOX) {
            HBRUSH hBrush = CreateSolidBrush(s.color);
            HBRUSH oB = (HBRUSH)SelectObject(hdc, hBrush);
            Rectangle(hdc, min(s1.x, s2.x), min(s1.y, s2.y), max(s1.x, s2.x), max(s1.y, s2.y));
            SelectObject(hdc, oB); DeleteObject(hBrush);
        } else if (s.type == TOOL_RECT) {
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, min(s1.x, s2.x), min(s1.y, s2.y), max(s1.x, s2.x), max(s1.y, s2.y));
        } else if (s.type == TOOL_CIRCLE_OUTLINE) {
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            int r = (int)sqrt(pow(s2.x - s1.x, 2) + pow(s2.y - s1.y, 2));
            Ellipse(hdc, s1.x - r, s1.y - r, s1.x + r, s1.y + r);
        } else if (s.type == TOOL_CIRCLE_SOLID) {
            HBRUSH hBrush = CreateSolidBrush(s.color);
            HBRUSH oB = (HBRUSH)SelectObject(hdc, hBrush);
            int r = (int)sqrt(pow(s2.x - s1.x, 2) + pow(s2.y - s1.y, 2));
            Ellipse(hdc, s1.x - r, s1.y - r, s1.x + r, s1.y + r);
            SelectObject(hdc, oB); DeleteObject(hBrush);
        } else if (s.type == TOOL_TEXT) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, s.color);
            HFONT hFont = CreateFontA(s.textSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
            HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

            SIZE textSize;
            GetTextExtentPoint32A(hdc, s.textContent.c_str(), s.textContent.size(), &textSize);
            TextOutA(hdc, s1.x, s1.y, s.textContent.c_str(), s.textContent.size());

            if (s.isInput) {
                HFONT hStarFont = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
                SelectObject(hdc, hStarFont);
                SetTextColor(hdc, RGB(220, 38, 38));
                TextOutA(hdc, s1.x + textSize.cx + 2, s1.y - 2, "*", 1);
                DeleteObject(hStarFont);
            }

            SelectObject(hdc, oldFont);
            DeleteObject(hFont);
        }
        DeleteObject(hPen);

        int minX, minY;
        if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) {
            int rad = (int)sqrt(pow(s2.x - s1.x, 2) + pow(s2.y - s1.y, 2));
            minX = s1.x - rad;
            minY = s1.y - rad;
        } else {
            minX = min(s1.x, s2.x);
            minY = min(s1.y, s2.y);
        }

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(160, 160, 160));
        HFONT hNameFont = CreateFontA(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
        HFONT oldNameFont = (HFONT)SelectObject(hdc, hNameFont);
        TextOutA(hdc, minX, minY - 13, s.name.c_str(), (int)s.name.size());
        if (s.parentIndex >= 0 && s.parentIndex < (int)g_shapes.size()) {
            int trackerCount = 0;
            for (const DrawShape& shape : g_shapes) if (shape.isEgp3D) ++trackerCount;
            int childId = trackerCount + (int)i + 1;
            int parentId = trackerCount + s.parentIndex + 1;
            std::string rel = "(" + std::to_string(childId) + "," + std::to_string(parentId) + ")";
            SIZE nameSize = {};
            GetTextExtentPoint32A(hdc, s.name.c_str(), (int)s.name.size(), &nameSize);
            SelectObject(hdc, hNameFont);
            SetTextColor(hdc, RGB(220, 70, 70));
            TextOutA(hdc, minX + nameSize.cx + 3, minY - 13, rel.c_str(), (int)rel.size());
        }
        SelectObject(hdc, oldNameFont);
        DeleteObject(hNameFont);

        if ((int)i == g_selectedShapeIndex) {
            HPEN selPen = CreatePen(PS_DOT, 1, RGB(145, 145, 145));
            SelectObject(hdc, selPen);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            
            int l, t, r, b;
            if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) {
                int rad = (int)sqrt(pow(s2.x - s1.x, 2) + pow(s2.y - s1.y, 2));
                l = s1.x - rad - 3;
                t = s1.y - rad - 3;
                r = s1.x + rad + 3;
                b = s1.y + rad + 3;
            } else if (s.type == TOOL_TEXT) {
                HFONT hFont = CreateFontA(s.textSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
                HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
                SIZE textSize;
                GetTextExtentPoint32A(hdc, s.textContent.c_str(), s.textContent.size(), &textSize);
                SelectObject(hdc, oldFont);
                DeleteObject(hFont);

                l = s1.x - 3; 
                t = s1.y - 3; 
                r = s1.x + textSize.cx + 3;
                b = s1.y + s.textSize + 3;
            } else {
                l = min(s1.x, s2.x) - 3; t = min(s1.y, s2.y) - 3;
                r = max(s1.x, s2.x) + 3; b = max(s1.y, s2.y) + 3;
            }

            Rectangle(hdc, l, t, r, b);
            DeleteObject(selPen);
        }
    }
}

inline void DrawPreviewShape(HDC hdc) {
    if (!g_isDrawing) return;

    HPEN previewPen = CreatePen(PS_DOT, g_currentThickness, g_currentColor);
    SelectObject(hdc, previewPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));

    if (g_currentTool == TOOL_LINE) {
        MoveToEx(hdc, g_startPt.x, g_startPt.y, NULL); LineTo(hdc, g_endPt.x, g_endPt.y);
    } else if (g_currentTool == TOOL_BOX || g_currentTool == TOOL_RECT) {
        Rectangle(hdc, min(g_startPt.x, g_endPt.x), min(g_startPt.y, g_endPt.y), max(g_startPt.x, g_endPt.x), max(g_startPt.y, g_endPt.y));
    } else if (g_currentTool == TOOL_CIRCLE_OUTLINE || g_currentTool == TOOL_CIRCLE_SOLID) {
        int r = (int)sqrt(pow(g_endPt.x - g_startPt.x, 2) + pow(g_endPt.y - g_startPt.y, 2));
        Ellipse(hdc, g_startPt.x - r, g_startPt.y - r, g_startPt.x + r, g_startPt.y + r);
    } else if (g_currentTool == TOOL_TEXT) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, g_currentColor);
        HFONT hFont = CreateFontA(g_currentTextSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

        SIZE textSize;
        GetTextExtentPoint32A(hdc, g_currentText.c_str(), g_currentText.size(), &textSize);
        TextOutA(hdc, g_startPt.x, g_startPt.y, g_currentText.c_str(), g_currentText.size());

        if (g_currentIsInput) {
            HFONT hStarFont = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
            SelectObject(hdc, hStarFont);
            SetTextColor(hdc, RGB(220, 38, 38));
            TextOutA(hdc, g_startPt.x + textSize.cx + 2, g_startPt.y - 2, "*", 1);
            DeleteObject(hStarFont);
        }

        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    DeleteObject(previewPen);
}

inline void DrawCanvasBottomBar(HDC hdc, RECT cRect, int winW, int winH) {
    SetBkMode(hdc, TRANSPARENT);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hUIFont);
    HPEN borderPen = CreatePen(PS_SOLID, 1, g_darkTheme ? RGB(76, 79, 85) : RGB(215, 215, 220));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);

    if (g_currentView == VIEW_VISUAL) {
        RECT barRect = { cRect.left, cRect.bottom + 2, cRect.right, cRect.bottom + 22 };
        HBRUSH bgBrush = CreateSolidBrush(g_darkTheme ? RGB(48, 50, 54) : RGB(242, 242, 245));
        FillRect(hdc, &barRect, bgBrush);
        DeleteObject(bgBrush);

        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, barRect.left, barRect.top, barRect.right, barRect.bottom);

        char szInfo[160];
        snprintf(szInfo, sizeof(szInfo), " Cursor: %ld, %ld   |   Canvas: 1920x1080   |   Window: %dx%d", 
            g_mousePos.x, g_mousePos.y, winW, winH);
        
        SetTextColor(hdc, g_darkTheme ? RGB(202, 205, 210) : RGB(60, 60, 60));
        DrawTextA(hdc, szInfo, -1, &barRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
    }

    int logTop = (g_currentView == VIEW_VISUAL) ? (cRect.bottom + 24) : (winH - 30);
    int logBottom = logTop + 22;
    int logLeft = (g_currentView == VIEW_VISUAL) ? cRect.left : 10;
    int logRight = (g_currentView == VIEW_VISUAL) ? cRect.right : (winW - 10);

    RECT logRect = { logLeft, logTop, logRight, logBottom };
    HBRUSH logBg = CreateSolidBrush(g_darkTheme ? RGB(45, 47, 51) : RGB(240, 240, 243));
    FillRect(hdc, &logRect, logBg);
    DeleteObject(logBg);

    SelectObject(hdc, borderPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, logRect.left, logRect.top, logRect.right, logRect.bottom);

    SetTextColor(hdc, g_darkTheme ? RGB(158, 162, 169) : RGB(80, 80, 85));
    std::string fullLog = " Log: " + g_logMessage;
    DrawTextA(hdc, fullLog.c_str(), -1, &logRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    SelectObject(hdc, oldFont);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}

inline void DrawVSPropertyGridBackground(HDC hdc, RECT rect) {
    HPEN borderPen = CreatePen(PS_SOLID, 1, g_darkTheme ? RGB(82, 85, 91) : RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

    int colSplitX = rect.left + 105;
    MoveToEx(hdc, colSplitX, rect.top, NULL);
    LineTo(hdc, colSplitX, rect.bottom);

    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}

inline std::string AttributeShapeTypeName(const DrawShape& s) {
    switch (s.type) {
        case TOOL_LINE: return "Line";
        case TOOL_RECT: return "Rect";
        case TOOL_BOX: return "Box";
        case TOOL_CIRCLE_OUTLINE: return "Circle (Outline)";
        case TOOL_CIRCLE_SOLID: return "Circle (Solid)";
        case TOOL_TEXT: return "Text";
        default: return "Shape";
    }
}

inline void DrawShapeAttributePanel(HDC hdc, RECT rect) {
    if (rect.bottom <= rect.top) return;
    COLORREF panelBg = g_darkTheme ? RGB(47, 49, 53) : RGB(247, 247, 249);
    COLORREF border = g_darkTheme ? RGB(76, 79, 85) : RGB(205, 205, 210);
    COLORREF headerBg = g_darkTheme ? RGB(57, 60, 65) : RGB(238, 238, 241);
    COLORREF text = g_darkTheme ? RGB(205, 208, 213) : RGB(55, 55, 60);
    COLORREF value = g_darkTheme ? RGB(190, 194, 200) : RGB(75, 75, 80);
    HBRUSH bg = CreateSolidBrush(panelBg);
    FillRect(hdc, &rect, bg);
    DeleteObject(bg);

    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    RECT header = { rect.left + 1, rect.top + 1, rect.right - 1, rect.top + 24 };
    HBRUSH hb = CreateSolidBrush(headerBg);
    FillRect(hdc, &header, hb);
    DeleteObject(hb);

    // Match the compact UI text size; the previous font looked oversized here.
    HFONT attrFont = CreateFontA(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
    HFONT oldFont = (HFONT)SelectObject(hdc, attrFont ? attrFont : (g_hUIFont ? g_hUIFont : GetStockObject(DEFAULT_GUI_FONT)));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, text);
    std::string title = "Attribute";
    if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size())
        title += " (" + g_shapes[g_selectedShapeIndex].name + ")";
    else
        title += " (None)";
    RECT tr = { rect.left + 8, rect.top + 3, rect.right - 6, rect.top + 23 };
    DrawTextA(hdc, title.c_str(), -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
    if (g_selectedShapeIndex < 0 || g_selectedShapeIndex >= (int)g_shapes.size()) {
        SelectObject(hdc, oldFont);
        if (attrFont) DeleteObject(attrFont);
        return;
    }
    // (เดิมวาดแถว Type/Vec2 Pos/Vec2 Size/Angle ซ้ำตรงนี้ ซึ่งซ้ำกับช่อง Attribute แล้ว จึงลบออก)
    SelectObject(hdc, oldFont);
    if (attrFont) DeleteObject(attrFont);
}

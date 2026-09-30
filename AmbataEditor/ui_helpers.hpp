#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <vector>
#include "model.hpp"

// Global Variables Reference (อ้างอิงจาก main.cpp)
extern std::vector<DrawShape> g_shapes;
extern int g_selectedShapeIndex;
extern ToolType g_currentTool;
extern WNDPROC g_OldToolBtnProc;
extern WNDPROC g_OldCopyBtnProc;
extern WNDPROC g_OldShapesPopupBtnProc;
extern HWND g_hMainWnd;
extern bool g_darkTheme;

// ==========================================
// Helper Functions
// ==========================================
inline HICON LoadEditorIconFromFile(const char* filePath) {
    return (HICON)LoadImageA(
        NULL, filePath, IMAGE_ICON, 0, 0,
        LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED
    );
}

inline RECT GetCanvasRect(int winWidth, int winHeight) {
    int topOffset = 70;
    int leftPanelWidth = 135, rightPanelWidth = 310;
    int workWidth = winWidth - leftPanelWidth - rightPanelWidth;
    int workHeight = winHeight - topOffset - 60; 

    if (workWidth < 100) workWidth = 100;
    if (workHeight < 100) workHeight = 100;

    double targetAspect = 1920.0 / 1080.0;
    double currentAspect = (double)workWidth / workHeight;

    int canvasW, canvasH;
    if (currentAspect > targetAspect) {
        canvasH = workHeight - 10;
        canvasW = (int)(canvasH * targetAspect);
    } else {
        canvasW = workWidth - 10;
        canvasH = (int)(canvasW / targetAspect);
    }

    int left = leftPanelWidth + (workWidth - canvasW) / 2;
    int top = topOffset + (workHeight - canvasH) / 2;
    return RECT{ left, top, left + canvasW, top + canvasH };
}

inline Point ScreenToCanvas(POINT pt, RECT canvasRect) {
    double scaleX = 1920.0 / std::max(1L, (long)(canvasRect.right - canvasRect.left));
    double scaleY = 1080.0 / std::max(1L, (long)(canvasRect.bottom - canvasRect.top));
    return { (pt.x - canvasRect.left) * scaleX, (pt.y - canvasRect.top) * scaleY };
}

inline POINT CanvasToScreen(Point pt, RECT canvasRect) {
    double scaleX = (canvasRect.right - canvasRect.left) / 1920.0;
    double scaleY = (canvasRect.bottom - canvasRect.top) / 1080.0;
    return POINT{ canvasRect.left + (int)(pt.first * scaleX), canvasRect.top + (int)(pt.second * scaleY) };
}

inline void CopyTextToClipboard(HWND hwnd, const std::string& text) {
    if (!OpenClipboard(hwnd)) return;
    EmptyClipboard();

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hGlob) {
        memcpy(GlobalLock(hGlob), text.c_str(), text.size() + 1);
        GlobalUnlock(hGlob);
        SetClipboardData(CF_TEXT, hGlob);
    }
    CloseClipboard();
}

inline Point GetAbsolutePosition(int index) {
    if (index < 0 || index >= (int)g_shapes.size()) return { 0.0, 0.0 };
    
    Point p = g_shapes[index].p1;
    int pIdx = g_shapes[index].parentIndex;
    int depth = 0;
    
    while (pIdx >= 0 && pIdx < (int)g_shapes.size() && depth < 50) {
        p.first += g_shapes[pIdx].p1.first;
        p.second += g_shapes[pIdx].p1.second;
        pIdx = g_shapes[pIdx].parentIndex;
        depth++;
    }
    return p;
}


// ==========================================
// Owner-draw ปุ่มที่มีไอคอน (แทนการ subclass + วาดทับใน WM_PAINT)
// วาดทุกอย่าง (พื้นปุ่ม ขอบ ไอคอน ข้อความ) ในรอบเดียว ไอคอนจึงไม่หายตอนกด/โฟกัส/เปลี่ยนสถานะ
// ==========================================
inline bool IsIconButtonId(int id) {
    // Main-window buttons, tool buttons, theme button, and action buttons are all
    // owner-drawn so the dark theme is consistent instead of falling back to the
    // Windows light button renderer.
    return (id >= 200 && id <= 205) || id == 208 || id == 403 ||
           (id >= 305 && id <= 308) || id == 101 || id == 102 || id == 103 ||
           id == 206 || id == 207 || id == 210 || id == 211 ||
           id == 304 || id == 314 || id == 315 || id == 318 || id == 321 ||
           id == 503 || id == 504 || id == 508 || id == 509 || id == 510 || id == IDCANCEL;
}

inline void DrawThemedButtonFrame(HDC hdc, RECT rc, bool pressed, bool enabled) {
    COLORREF bg = g_darkTheme ? (pressed ? RGB(55, 57, 61) : RGB(43, 45, 49))
                              : (pressed ? RGB(218, 218, 222) : RGB(245, 245, 247));
    COLORREF border = g_darkTheme ? RGB(72, 75, 81) : RGB(185, 185, 190);
    if (!enabled) bg = g_darkTheme ? RGB(38, 40, 44) : RGB(232, 232, 234);
    HBRUSH b = CreateSolidBrush(bg);
    FillRect(hdc, &rc, b);
    DeleteObject(b);
    HPEN p = CreatePen(PS_SOLID, 1, border);
    HPEN old = (HPEN)SelectObject(hdc, p);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right - 1, rc.bottom - 1);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, old);
    DeleteObject(p);
}

inline void DrawIconButtonItem(const DRAWITEMSTRUCT* d) {
    HDC hdc = d->hDC;
    RECT rc = d->rcItem;
    bool pressed = (d->itemState & ODS_SELECTED) != 0;
    bool checked = (d->itemState & ODS_CHECKED) != 0;
    bool focused = (d->itemState & ODS_FOCUS) != 0;
    bool enabled = IsWindowEnabled(d->hwndItem);
    int id = (int)d->CtlID;

    // Checkbox-style: ไม่มีแผ่นปุ่มรองพื้น (เรียบ กลืนกับพื้นหน้าต่าง) ส่วนปุ่มอื่นวาดกรอบปุ่มตามปกติ
    bool isCheckStyle = (id == 304 || id == 314 || id == 315 || id == 318 || id == 321);
    if (isCheckStyle) {
        HBRUSH flat = CreateSolidBrush(g_darkTheme ? RGB(41, 41, 41) : RGB(245, 245, 247));
        FillRect(hdc, &rc, flat);
        DeleteObject(flat);
    } else {
        DrawThemedButtonFrame(hdc, rc, pressed, enabled);
    }

    COLORREF fg = enabled ? (g_darkTheme ? RGB(232,234,237) : RGB(35,35,38))
                          : (g_darkTheme ? RGB(105,108,114) : RGB(155,155,160));
    HPEN pen = CreatePen(PS_SOLID, 1, fg);
    HBRUSH brush = CreateSolidBrush(fg);
    // Dark theme: ไอคอนสีขาว + จุดเน้นสีส้ม (สไตล์ Maya)  |  light theme และปุ่ม disabled ใช้สี fg เดิม
    COLORREF acc = (g_darkTheme && enabled) ? RGB(255, 145, 30) : fg;
    HPEN penA = CreatePen(PS_SOLID, 1, acc);
    HBRUSH brA = CreateSolidBrush(acc);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    // Theme switch: moon in light mode, sun in dark mode.
    if (id == 103) {
        int cx = (rc.left + rc.right) / 2;
        int cy = (rc.top + rc.bottom) / 2;
        if (!g_darkTheme) {
            // Moon / crescent
            SelectObject(hdc, brush);
            Ellipse(hdc, cx - 6, cy - 6, cx + 6, cy + 6);
            HBRUSH cut = CreateSolidBrush(g_darkTheme ? RGB(43,45,49) : RGB(245,245,247));
            SelectObject(hdc, cut);
            Ellipse(hdc, cx - 2, cy - 8, cx + 8, cy + 2);
            DeleteObject(cut);
        } else {
            // Sun: วงกลมสีขาว + รังสีสีส้ม
            HPEN rayPen = CreatePen(PS_SOLID, 2, acc);
            HPEN prevPen = (HPEN)SelectObject(hdc, rayPen);
            for (int i = 0; i < 8; ++i) {
                double a = i * 3.141592653589793 / 4.0;
                int x1 = cx + (int)lround(7.5 * cos(a));
                int y1 = cy + (int)lround(7.5 * sin(a));
                int x2 = cx + (int)lround(11.0 * cos(a));
                int y2 = cy + (int)lround(11.0 * sin(a));
                MoveToEx(hdc, x1, y1, NULL); LineTo(hdc, x2, y2);
            }
            SelectObject(hdc, pen);
            SelectObject(hdc, brush);
            Ellipse(hdc, cx - 4, cy - 4, cx + 5, cy + 5);
            SelectObject(hdc, prevPen);
            DeleteObject(rayPen);
        }
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
        DeleteObject(brush);
        DeleteObject(penA);
        DeleteObject(brA);
        return;
    }

    // Checkbox-style buttons.
    if (id == 304 || id == 314 || id == 315 || id == 318 || id == 321) {
        int bx = rc.left + 5;
        int by = rc.top + ((rc.bottom - rc.top) - 13) / 2;
        RECT cb = { bx, by, bx + 13, by + 13 };
        HBRUSH cbBg = CreateSolidBrush(g_darkTheme ? RGB(52, 52, 52) : RGB(255, 255, 255));   // เท่าสีช่องกรอก
        FillRect(hdc, &cb, cbBg);
        DeleteObject(cbBg);
        HPEN boxPen = CreatePen(PS_SOLID, 1, g_darkTheme ? (enabled ? RGB(150, 153, 160) : RGB(85, 88, 94)) : fg);
        HPEN prevBox = (HPEN)SelectObject(hdc, boxPen);
        Rectangle(hdc, cb.left, cb.top, cb.right, cb.bottom);
        SelectObject(hdc, prevBox);
        DeleteObject(boxPen);
        if (checked) {
            HPEN chkPen = CreatePen(PS_SOLID, 2, acc);   // เครื่องหมายถูกสีส้มใน dark theme
            HPEN prevChk = (HPEN)SelectObject(hdc, chkPen);
            MoveToEx(hdc, bx + 3, by + 7, NULL);
            LineTo(hdc, bx + 6, by + 10);
            LineTo(hdc, bx + 11, by + 3);
            SelectObject(hdc, prevChk);
            DeleteObject(chkPen);
        }
        char buf[128] = {0}; GetWindowTextA(d->hwndItem, buf, sizeof(buf));
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, fg);
        HFONT font = (HFONT)SendMessage(d->hwndItem, WM_GETFONT, 0, 0);
        HFONT oldFont = font ? (HFONT)SelectObject(hdc, font) : NULL;
        RECT tr = rc; tr.left = bx + 18;
        DrawTextA(hdc, buf, -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
        if (oldFont) SelectObject(hdc, oldFont);
        SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen);
        DeleteObject(pen); DeleteObject(brush);
        DeleteObject(penA); DeleteObject(brA);
        return;
    }

    int off = pressed ? 1 : 0;
    int ix = rc.left + 8 + off;
    int iy = rc.top + ((rc.bottom - rc.top) - 11) / 2 + off;

    switch (id) {
    case 200: {   // Select: ลูกศรเติมสีส้ม ขอบขาว
        POINT pts[3] = {{ix,iy},{ix+10,iy+5},{ix+4,iy+9}};
        SelectObject(hdc, brA);
        Polygon(hdc, pts, 3);
        break;
    }
    case 201:     // Line: เส้นขาว จุดปลายส้ม
        MoveToEx(hdc, ix, iy+10, NULL); LineTo(hdc, ix+10, iy);
        SelectObject(hdc, penA); SelectObject(hdc, brA);
        Rectangle(hdc, ix-1, iy+8, ix+3, iy+12);
        Rectangle(hdc, ix+8, iy-1, ix+12, iy+3);
        break;
    case 202:     // Rect outline: กรอบขาว มุมส้ม
        Rectangle(hdc, ix, iy, ix+11, iy+10);
        SelectObject(hdc, penA); SelectObject(hdc, brA);
        Rectangle(hdc, ix-1, iy-1, ix+2, iy+2);
        Rectangle(hdc, ix+9, iy+8, ix+12, iy+11);
        break;
    case 203:     // Box solid: เติมส้ม ขอบขาว
        SelectObject(hdc, brA);
        Rectangle(hdc, ix, iy, ix+11, iy+10);
        break;
    case 204:     // Circle outline: วงขาว จุดกลางส้ม
        Ellipse(hdc, ix, iy, ix+11, iy+11);
        SelectObject(hdc, penA); SelectObject(hdc, brA);
        Ellipse(hdc, ix+4, iy+4, ix+7, iy+7);
        break;
    case 208:     // Circle solid: เติมส้ม ขอบขาว
        SelectObject(hdc, brA);
        Ellipse(hdc, ix, iy, ix+11, iy+11);
        break;
    case 205: {   // Text: "A" ขาว + "a" ส้ม
        SetBkMode(hdc, TRANSPARENT);
        HFONT f = CreateFontA(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, 0, 0, "Tahoma");
        HFONT of = (HFONT)SelectObject(hdc, f);
        SIZE sz = {0, 0};
        GetTextExtentPoint32A(hdc, "A", 1, &sz);
        SetTextColor(hdc, fg);  TextOutA(hdc, ix, iy - 1, "A", 1);
        SetTextColor(hdc, acc); TextOutA(hdc, ix + sz.cx, iy - 1, "a", 1);
        SelectObject(hdc, of);
        DeleteObject(f);
        break;
    }
    case 305:     // Flip X: ลูกศรซ้าย/ขวาสีขาว แกนกลางสีส้ม
        MoveToEx(hdc,ix+1,iy+5,NULL); LineTo(hdc,ix+5,iy+5); MoveToEx(hdc,ix+1,iy+5,NULL); LineTo(hdc,ix+3,iy+3); MoveToEx(hdc,ix+1,iy+5,NULL); LineTo(hdc,ix+3,iy+7);
        MoveToEx(hdc,ix+9,iy+5,NULL); LineTo(hdc,ix+5,iy+5); MoveToEx(hdc,ix+9,iy+5,NULL); LineTo(hdc,ix+7,iy+3); MoveToEx(hdc,ix+9,iy+5,NULL); LineTo(hdc,ix+7,iy+7);
        SelectObject(hdc, penA);
        MoveToEx(hdc,ix+5,iy-1,NULL); LineTo(hdc,ix+5,iy+11);
        MoveToEx(hdc,ix+6,iy-1,NULL); LineTo(hdc,ix+6,iy+11);
        break;
    case 306:     // Flip Y: ลูกศรขึ้น/ลงสีขาว แกนกลางสีส้ม
        MoveToEx(hdc,ix+5,iy+1,NULL); LineTo(hdc,ix+5,iy+5); MoveToEx(hdc,ix+5,iy+1,NULL); LineTo(hdc,ix+3,iy+3); MoveToEx(hdc,ix+5,iy+1,NULL); LineTo(hdc,ix+7,iy+3);
        MoveToEx(hdc,ix+5,iy+9,NULL); LineTo(hdc,ix+5,iy+5); MoveToEx(hdc,ix+5,iy+9,NULL); LineTo(hdc,ix+3,iy+7); MoveToEx(hdc,ix+5,iy+9,NULL); LineTo(hdc,ix+7,iy+7);
        SelectObject(hdc, penA);
        MoveToEx(hdc,ix-1,iy+5,NULL); LineTo(hdc,ix+12,iy+5);
        MoveToEx(hdc,ix-1,iy+6,NULL); LineTo(hdc,ix+12,iy+6);
        break;
    case 307:     // Center: วงขาว กากบาท+จุดกลางส้ม
        Ellipse(hdc, ix+1, iy+1, ix+9, iy+9);
        SelectObject(hdc, penA); SelectObject(hdc, brA);
        MoveToEx(hdc,ix+5,iy-1,NULL); LineTo(hdc,ix+5,iy+11);
        MoveToEx(hdc,ix-1,iy+5,NULL); LineTo(hdc,ix+11,iy+5);
        Ellipse(hdc, ix+3, iy+3, ix+7, iy+7);
        break;
    case 308:     // Clone: ใบหลังส้ม ใบหน้าขาว
        SelectObject(hdc, penA); SelectObject(hdc, brA);
        Rectangle(hdc, ix+3, iy-1, ix+11, iy+8);
        SelectObject(hdc, pen); SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, ix, iy+2, ix+8, iy+11);
        break;
    case 403:     // Copy: ใบหลังขาว ใบหน้าส้ม
        Rectangle(hdc, ix+3, iy-1, ix+11, iy+8);
        SelectObject(hdc, penA);
        Rectangle(hdc, ix, iy+2, ix+8, iy+11);
        break;
    }

    // Text buttons / tabs / actions.
    char buf[128] = {0}; GetWindowTextA(d->hwndItem, buf, sizeof(buf));
    const char* txt = buf; while (*txt == ' ') ++txt;
    SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, fg);
    HFONT font = (HFONT)SendMessage(d->hwndItem, WM_GETFONT, 0, 0);
    HFONT oldFont = font ? (HFONT)SelectObject(hdc, font) : NULL;
    RECT tr = rc;
    bool left = (id == 210 || id == 211 || id == 403 || id == 200 || id == 201 || id == 202 || id == 203 || id == 204 || id == 205 || id == 208 || (id >= 305 && id <= 308));
    if (id >= 200 && id <= 205 || id == 208 || id == 403 || (id >= 305 && id <= 308)) tr.left += 26 + off;
    if (left && id != 403 && id != 200 && id != 201 && id != 202 && id != 203 && id != 204 && id != 205 && id != 208 && !(id >= 305 && id <= 308)) tr.left += 8;
    DrawTextA(hdc, txt, -1, &tr, DT_SINGLELINE | DT_VCENTER | (left ? DT_LEFT : DT_CENTER) | DT_NOPREFIX);
    if (oldFont) SelectObject(hdc, oldFont);

    if (focused) { RECT fr = rc; InflateRect(&fr,-3,-3); DrawFocusRect(hdc,&fr); }
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen);
    DeleteObject(pen); DeleteObject(brush);
    DeleteObject(penA); DeleteObject(brA);
}

// เรียก proc เดิมของ subclass; ถ้าเป็น NULL ให้ใช้ DefWindowProc (กัน WM_PAINT ไม่ถูก validate จนหน้าต่างอื่นไม่ได้ repaint)
inline LRESULT CallOldProc(WNDPROC oldProc, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return oldProc ? CallWindowProc(oldProc, hwnd, msg, wParam, lParam)
                   : DefWindowProc(hwnd, msg, wParam, lParam);
}

// ==========================================
// Custom Subclassing
// ==========================================
inline LRESULT CALLBACK ToolButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        LRESULT res = CallOldProc(g_OldToolBtnProc, hwnd, msg, wParam, lParam);
        HDC hdc = GetDC(hwnd);
        RECT rc; GetClientRect(hwnd, &rc);

        int id = GetDlgCtrlID(hwnd);
        HPEN darkPen = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
        HPEN oldPen = (HPEN)SelectObject(hdc, darkPen);
        HBRUSH darkBrush = CreateSolidBrush(RGB(80, 80, 80));

        int ix = 8, iy = (rc.bottom - 10) / 2;

        if (id == 200 /* ID_BTN_SELECT */) {
            POINT pts[3] = { {ix, iy}, {ix + 10, iy + 5}, {ix + 4, iy + 9} };
            SelectObject(hdc, darkBrush);
            Polygon(hdc, pts, 3);
        } else if (id == 201 /* ID_BTN_LINE */) {
            MoveToEx(hdc, ix, iy + 10, NULL); LineTo(hdc, ix + 10, iy);
        } else if (id == 202 /* ID_BTN_RECT */) {
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, ix, iy, ix + 11, iy + 10);
        } else if (id == 203 /* ID_BTN_BOX */) {
            SelectObject(hdc, darkBrush);
            Rectangle(hdc, ix, iy, ix + 11, iy + 10);
        } else if (id == 204 /* ID_BTN_CIRCLE_OUTLINE */) {
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Ellipse(hdc, ix, iy, ix + 11, iy + 11);
        } else if (id == 208 /* ID_BTN_CIRCLE_SOLID */) {
            SelectObject(hdc, darkBrush);
            Ellipse(hdc, ix, iy, ix + 11, iy + 11);
        } else if (id == 205 /* ID_BTN_TEXT */) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(40, 40, 40));
            HFONT font = CreateFontA(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, 0, 0, "Tahoma");
            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            TextOutA(hdc, ix, iy - 1, "Aa", 2);
            SelectObject(hdc, oldFont);
            DeleteObject(font);
        }

        SelectObject(hdc, oldPen);
        DeleteObject(darkPen);
        DeleteObject(darkBrush);
        ReleaseDC(hwnd, hdc);
        return res;
    }
    return CallOldProc(g_OldToolBtnProc, hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK CopyButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        LRESULT res = CallOldProc(g_OldCopyBtnProc, hwnd, msg, wParam, lParam);
        HDC hdc = GetDC(hwnd);
        RECT rc; GetClientRect(hwnd, &rc);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(70, 70, 70));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        SelectObject(hdc, GetStockObject(WHITE_BRUSH));

        int ix = 8, iy = (rc.bottom - 12) / 2;
        Rectangle(hdc, ix + 3, iy - 1, ix + 11, iy + 8);
        Rectangle(hdc, ix, iy + 2, ix + 8, iy + 11);

        SelectObject(hdc, oldPen);
        DeleteObject(pen);
        ReleaseDC(hwnd, hdc);
        return res;
    }
    return CallOldProc(g_OldCopyBtnProc, hwnd, msg, wParam, lParam);
}

inline LRESULT CALLBACK ShapesPopupBtnSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        LRESULT res = CallOldProc(g_OldShapesPopupBtnProc, hwnd, msg, wParam, lParam);
        HDC hdc = GetDC(hwnd);
        RECT rc; GetClientRect(hwnd, &rc);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        SelectObject(hdc, GetStockObject(WHITE_BRUSH));

        int ix = (rc.right - 14) / 2, iy = (rc.bottom - 14) / 2;
        Rectangle(hdc, ix + 4, iy, ix + 14, iy + 10);
        Rectangle(hdc, ix, iy + 4, ix + 10, iy + 14);

        SelectObject(hdc, oldPen);
        DeleteObject(pen);
        ReleaseDC(hwnd, hdc);
        return res;
    }
    return CallOldProc(g_OldShapesPopupBtnProc, hwnd, msg, wParam, lParam);
}
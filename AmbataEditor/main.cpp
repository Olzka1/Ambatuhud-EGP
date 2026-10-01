#ifndef WINVER
#define WINVER 0x0600
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <climits>
#include <gdiplus.h>

#include "model.hpp"
#include "exporter.hpp"
#include "ui_helpers.hpp"
#include "canvas_drawer.hpp"
#include "e2_syntax.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace std;

// Forward declaration used by the Shape Manage popup before its definition.
static void UpdateAttributeText();

// Control IDs
#define ID_TAB_VISUAL          101
#define ID_TAB_SCRIPT          102
#define ID_BTN_THEME            103

#define ID_BTN_SELECT          200
#define ID_BTN_LINE            201
#define ID_BTN_RECT            202
#define ID_BTN_BOX             203
#define ID_BTN_CIRCLE_OUTLINE  204
#define ID_BTN_CIRCLE_SOLID    208
#define ID_BTN_TEXT            205
#define ID_BTN_EXPORT          206
#define ID_BTN_CLEAR           207

#define ID_BTN_SHAPES_POPUP    210
#define ID_BTN_VARS_POPUP      211
#define ID_COMBO_BG            212

#define ID_BTN_COLOR_PICKER    301
#define ID_EDIT_THICKNESS      302
#define ID_EDIT_TEXT_CONTENT   303
#define ID_CHK_IS_INPUT        304
#define ID_EDIT_TEXT_VARNAME   309
#define ID_COMBO_INPUT_TYPE    310

// Pos X, Pos Y, Parent & Variable IDs
#define ID_EDIT_POS_X          311
#define ID_EDIT_POS_Y          312
#define ID_COMBO_PARENT        313
#define ID_CHK_USE_VAR_X       314
#define ID_CHK_USE_VAR_Y       315
#define ID_EDIT_VAR_X_NAME     316
#define ID_EDIT_VAR_Y_NAME     317
#define ID_CHK_USE_VAR_SIZE_X  330
#define ID_CHK_USE_VAR_SIZE_Y  331
#define ID_CHK_USE_VAR_ANGLE   332
#define ID_EDIT_VAR_SIZE_X_NAME 333
#define ID_EDIT_VAR_SIZE_Y_NAME 334
#define ID_EDIT_VAR_ANGLE_NAME  335
#define ID_CHK_IS_EGP3D        321
#define ID_EDIT_VAR_3D_NAME    322

// Text Size Control ID
#define ID_EDIT_TEXT_SIZE      320

// Interval Settings IDs
#define ID_CHK_USE_INTERVAL    318
#define ID_EDIT_INTERVAL_MS    319

#define ID_BTN_FLIP_X          305
#define ID_BTN_FLIP_Y          306
#define ID_BTN_CENTER          307
#define ID_BTN_CLONE           308

#define ID_EDIT_SCRIPT_VIEW    401
#define ID_EDIT_SCRIPT_LINES   402
#define ID_BTN_COPY_SCRIPT     403
#define ID_EDIT_E2NAME         404

#define ID_DLG_LISTBOX         502
#define ID_DLG_BTN_DELETE      503
#define ID_DLG_BTN_CLEARALL    504
#define ID_VAR_LIST             505
#define ID_VAR_EDIT_NAME        506
#define ID_VAR_COMBO_TYPE       507
#define ID_VAR_BTN_ADD          508
#define ID_VAR_BTN_RENAME       509
#define ID_VAR_BTN_DELETE       510
#define ID_VAR_COMBO_DIRECTION 511
#define ID_VAR_EDIT_EXPRESSION 512
#define ID_CHK_SHOW_GRID        324
#define ID_EDIT_OPACITY        323
#define ID_EDIT_SIZE_X         325
#define ID_EDIT_SIZE_Y         326
#define ID_EDIT_ANGLE          327

// Global Variables & State
Project g_project = Project::create_new(1920, 1080);
vector<DrawShape> g_shapes;
vector<E2Variable> g_variables;

ViewMode g_currentView = VIEW_VISUAL;
ToolType g_currentTool = TOOL_TEXT; 
COLORREF g_currentColor = RGB(0, 204, 255);
int g_currentThickness = 1;
int g_currentTextSize = 24;
string g_currentText = "Text";
bool g_currentIsInput = false;
string g_currentVarName = "Var1";
InputType g_currentInputType = INPUT_NUM;

// Background State
int g_bgMode = 0; // 0: White, 1: Sky1, 2: Sky2, 3: Dark BG
HBITMAP g_hBgBitmap = NULL;
ULONG_PTR g_gdiplusToken = 0;

// Counts for auto-naming shapes
int g_lineCount = 0;
int g_rectCount = 0;
int g_boxCount = 0;
int g_circleOutlineCount = 0;
int g_circleSolidCount = 0;
int g_textCount = 0;

// Position State & Variable Controls
double g_currentPosX = 0.0;
double g_currentPosY = 0.0;
double g_currentAngle = 0.0;
int g_currentParentIndex = -1;
bool g_currentUseVarX = false;
bool g_currentUseVarY = false;
bool g_currentUseVarSizeX = false;
bool g_currentUseVarSizeY = false;
bool g_currentUseVarAngle = false;
string g_currentVarXName = "X";
string g_currentVarYName = "Y";
string g_currentVarSizeXName = "SizeX";
string g_currentVarSizeYName = "SizeY";
string g_currentVarAngleName = "Angle";
bool g_currentIsEgp3D = false;
string g_currentVar3DName = "Pos";

// Interval Global State
bool g_useInterval = true;
bool g_darkTheme = false;
bool g_showGrid = true;      // show/hide canvas grid lines (center axes always shown)
int g_intervalMs = 100;
string g_e2Name = "myass hud";
int g_propGridBottom = 0;   // bottom of the property grid (computed at layout)

int g_selectedShapeIndex = -1; 
bool g_isDraggingShape = false;
bool g_isResizingShape = false;
Point g_dragOffset = { 0.0, 0.0 };
Point g_dragRawP1  = { 0.0, 0.0 };   // raw (pre-snap) p1 while dragging a shape

bool g_isDrawing = false;
POINT g_startPt, g_endPt;
POINT g_mousePos = { 0, 0 };
COLORREF g_custColors[16] = { 0 };

string g_logMessage = "System initialized.";

HFONT g_hUIFont = NULL;
HFONT g_hBoldFont = NULL;
HFONT g_hCodeFont = NULL;
HICON g_hAppIcon = NULL;

// Controls
HWND g_hMainWnd = NULL;
HWND g_hBtnTabVisual, g_hBtnTabScript, g_hBtnTheme;
HWND g_hLblTools, g_hBtnSelect, g_hBtnLine, g_hBtnRect, g_hBtnBox, g_hBtnCircleOutline, g_hBtnCircleSolid, g_hBtnText, g_hBtnExport, g_hBtnClear;
HWND g_hBtnShapesPopup, g_hBtnVarsPopup, g_hComboBg;

// Visual Studio Style Properties Panel
HWND g_hHeaderTransform, g_hHeaderAppearance, g_hHeaderProperties;

// Appearance Controls
HWND g_hLblColorName, g_hBtnColorPicker;
HWND g_hLblSizeX, g_hEditSizeX, g_hLblSizeY, g_hEditSizeY, g_hLblAngle, g_hEditAngle;
HWND g_hLblOpacityName, g_hEditOpacity;
HWND g_hLblThickName, g_hEditThick;
HWND g_hLblTextContentName, g_hEditTextContent;
HWND g_hLblTextSizeName, g_hEditTextSize;

// Transform Controls
HWND g_hLblPosX, g_hEditPosX, g_hChkUseVarX, g_hEditVarXName;
HWND g_hLblPosY, g_hEditPosY, g_hChkUseVarY, g_hEditVarYName;
HWND g_hChkUseVarSizeX, g_hChkUseVarSizeY, g_hChkUseVarAngle;
HWND g_hEditVarSizeXName, g_hEditVarSizeYName, g_hEditVarAngleName;
HWND g_hChkIsEgp3D, g_hEditVar3DName;

// Properties Controls
HWND g_hLblParent, g_hComboParent;
HWND g_hChkIsInput, g_hLblVarName, g_hEditTextVarName, g_hLblType, g_hComboInputType;
HWND g_hChkUseInterval, g_hLblInterval, g_hEditIntervalMs;
HWND g_hChkShowGrid;

HWND g_hBtnFlipX, g_hBtnFlipY, g_hBtnCenter, g_hBtnClone;
HWND g_hAttrText = NULL;

// Script View Controls
HWND g_hBtnCopyScript;
HWND g_hLblE2Name, g_hEditE2Name;
HWND g_hEditScriptView;
HWND g_hEditScriptLines;

// Dynamic Subclass Window Procedures
WNDPROC g_OldToolBtnProc = NULL;
WNDPROC g_OldCopyBtnProc = NULL;
WNDPROC g_OldShapesPopupBtnProc = NULL;
WNDPROC g_OldScriptViewProc = NULL;
bool g_scriptIsRich = false;      // true = Script view is a RichEdit (colored) | false = plain EDIT (fallback)
int  g_gutterLastScroll = -1;

HWND g_hPopupDlg = NULL;
HWND g_hPopupList = NULL;
HWND g_hVarPopupDlg = NULL;
HWND g_hVarPopupList = NULL;
HFONT g_hVarListFont = NULL;
HWND g_hVarNameLabel = NULL;
HWND g_hVarTypeLabel = NULL;
HWND g_hVarNameEdit = NULL;
HWND g_hVarTypeCombo = NULL;
HWND g_hVarDirectionCombo = NULL;
HWND g_hVarExpressionLabel = NULL;
HWND g_hVarExpressionEdit = NULL;

void UpdateToolButtonHighlights();
void UpdateInputControlsState();
LRESULT CALLBACK ScriptViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditCtrlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void SubclassEditControl(HWND h);
void UpdateScriptViewText();

static string GetExeDirectory() {
    char path[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return ".";
    string full(path, len);
    size_t slash = full.find_last_of("\\/");
    return slash == string::npos ? "." : full.substr(0, slash);
}

static string FindBgImageFile(const string& stem) {
    const char* exts[] = { ".png", ".jpg", ".jpeg", ".bmp" };
    string exeDir = GetExeDirectory();
    for (const char* ext : exts) {
        string p = exeDir + "\\images\\" + stem + ext;
        if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES) return p;
    }
    // Fallback for running directly from the project working directory.
    for (const char* ext : exts) {
        string p = string("images/") + stem + ext;
        if (GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES) return p;
    }
    return "";
}

void LoadBgImage() {
    if (g_hBgBitmap) {
        DeleteObject(g_hBgBitmap);
        g_hBgBitmap = NULL;
    }

    string stem;
    if (g_bgMode == 1) stem = "sky1";
    else if (g_bgMode == 2) stem = "sky2";
    else if (g_bgMode == 3) stem.clear();
    if (stem.empty()) return;

    string filePath = FindBgImageFile(stem);
    if (filePath.empty()) return;

    // GDI+ allows the background enum to use PNG/JPG/JPEG as well as BMP.
    // We convert the loaded image to an HBITMAP so the existing canvas renderer stays unchanged.
    int wideLen = MultiByteToWideChar(CP_ACP, 0, filePath.c_str(), -1, NULL, 0);
    if (wideLen <= 0) return;
    std::vector<wchar_t> widePath((size_t)wideLen);
    MultiByteToWideChar(CP_ACP, 0, filePath.c_str(), -1, widePath.data(), wideLen);

    Gdiplus::Bitmap bitmap(widePath.data());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) return;

    HBITMAP hbm = NULL;
    Gdiplus::Color transparent(0, 0, 0, 0);
    if (bitmap.GetHBITMAP(transparent, &hbm) == Gdiplus::Ok) {
        g_hBgBitmap = hbm;
    }
}

void AddLog(HWND hwnd, const string& msg) {
    g_logMessage = msg;
    RECT rc; GetClientRect(hwnd, &rc);
    RECT cRect = GetCanvasRect(rc.right, rc.bottom);
    RECT logArea = { cRect.left, cRect.bottom + 24, cRect.right, cRect.bottom + 46 };
    InvalidateRect(hwnd, &logArea, FALSE);
}

// ==========================================
// Anti-flicker helpers: act only when the value really changes
// ==========================================
static bool g_layoutChanged = false;

static void SetTextIfChanged(HWND h, const char* text) {
    if (!h) return;
    char cur[512] = {0};
    GetWindowTextA(h, cur, sizeof(cur));
    if (strcmp(cur, text) != 0) SetWindowTextA(h, text);
}

static void SetCtrlVisible(HWND h, bool visible) {
    if (!h) return;
    bool cur = (GetWindowLongPtr(h, GWL_STYLE) & WS_VISIBLE) != 0;
    if (cur != visible) {
        ShowWindow(h, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
        g_layoutChanged = true;
    }
}

static void ShowCtrl(HWND h, int cmd) { SetCtrlVisible(h, cmd != SW_HIDE); }

static void HideScriptControlsInVisual() {
    if (g_currentView != VIEW_VISUAL) return;
    SetCtrlVisible(g_hBtnCopyScript, false);
    SetCtrlVisible(g_hLblE2Name, false);
    SetCtrlVisible(g_hEditE2Name, false);
    SetCtrlVisible(g_hEditScriptLines, false);
    SetCtrlVisible(g_hEditScriptView, false);
}

static void SetCtrlEnabled(HWND h, bool enabled) {
    if (!h) return;
    if ((IsWindowEnabled(h) != FALSE) != enabled) EnableWindow(h, enabled);
}

// A label/checkbox disabled via EnableWindow gets Windows' white-shadow text (hard to read in dark theme)
// so do not truly disable: use "dim" (label) / "lock" (checkbox) and draw gray text ourselves in WM_CTLCOLOR*
static void SetCtrlProp(HWND h, const char* prop, bool on) {
    if (!h) return;
    bool cur = GetPropA(h, prop) != NULL;
    if (cur == on) return;
    if (on) SetPropA(h, prop, (HANDLE)1); else RemovePropA(h, prop);
    InvalidateRect(h, NULL, TRUE);
}
static void SetCtrlDim(HWND h, bool dim) { SetCtrlProp(h, "AmbataDim", dim); }
static void SetCtrlLocked(HWND h, bool locked) { SetCtrlProp(h, "AmbataLocked", locked); }
static bool IsCtrlLocked(HWND h) { return h && GetPropA(h, "AmbataLocked") != NULL; }

static void PlaceCtrl(HWND h, int x, int y, int w, int hgt) {
    if (!h) return;
    RECT r; GetWindowRect(h, &r);
    MapWindowPoints(NULL, GetParent(h), (POINT*)&r, 2);
    char cls[32] = {0};
    GetClassNameA(h, cls, sizeof(cls));
    bool isCombo = (_stricmp(cls, "ComboBox") == 0);   // ComboBox: configured height is the dropdown height
    bool same = (r.left == x && r.top == y && (r.right - r.left) == w &&
                 (isCombo || (r.bottom - r.top) == hgt));
    if (same) return;
    SetWindowPos(h, NULL, x, y, w, hgt, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    g_layoutChanged = true;
}

// ===== Central theme palette =====
// Rule: every WM_CTLCOLOR* must SetBkColor(X) and return a brush of the same color X
// otherwise text gets a differently colored band over the control background (cause of the dark-theme double background)
static COLORREF ThemeWindowColor() { return g_darkTheme ? RGB(41, 41, 41) : RGB(245, 245, 247); }
static COLORREF ThemePanelColor()  { return ThemeWindowColor(); }   // no separate panel color: flat like the light theme
static COLORREF ThemeInputColor()  { return g_darkTheme ? RGB(52, 52, 52) : RGB(255, 255, 255); }
static COLORREF ThemeTextColor()   { return g_darkTheme ? RGB(214, 217, 221) : RGB(0, 0, 0); }
static COLORREF ThemeMutedTextColor() { return g_darkTheme ? RGB(105, 108, 114) : RGB(160, 160, 160); }
static COLORREF ThemeHeaderColor() { return g_darkTheme ? RGB(58, 58, 58) : RGB(215, 215, 215); }
static COLORREF ThemeAttrColor()   { return g_darkTheme ? RGB(47, 47, 47) : RGB(247, 247, 249); }

// brush cache by color (created once per color, never DeleteObject)
static HBRUSH ThemeBrush(COLORREF c) {
    static COLORREF keys[24];
    static HBRUSH vals[24];
    static int n = 0;
    for (int i = 0; i < n; i++) if (keys[i] == c) return vals[i];
    HBRUSH b = CreateSolidBrush(c);
    if (n < 24) { keys[n] = c; vals[n] = b; n++; }
    return b;
}

// Standard WM_CTLCOLORSTATIC/EDIT/LISTBOX/BTN handler (shared by all windows)
static LRESULT ThemeCtlColor(UINT msg, HDC hdc, HWND h) {
    if (msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX) {
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, ThemeInputColor());
        SetTextColor(hdc, (h && !IsWindowEnabled(h)) ? ThemeMutedTextColor() : ThemeTextColor());
        return (LRESULT)ThemeBrush(ThemeInputColor());
    }
    if (msg == WM_CTLCOLORSTATIC && h) {
        char cls[16] = {0};
        GetClassNameA(h, cls, sizeof(cls));
        if (_stricmp(cls, "Edit") == 0) {   // disabled/read-only EDITs arrive here; must match a normal EDIT background
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, ThemeInputColor());
            SetTextColor(hdc, !IsWindowEnabled(h) ? ThemeMutedTextColor() : ThemeTextColor());
            return (LRESULT)ThemeBrush(ThemeInputColor());
        }
    }
    // STATIC label / BUTTON: same background as the window
    SetBkMode(hdc, TRANSPARENT);
    SetBkColor(hdc, ThemeWindowColor());
    SetTextColor(hdc, (h && !IsWindowEnabled(h)) ? ThemeMutedTextColor() : ThemeTextColor());
    return (LRESULT)ThemeBrush(ThemeWindowColor());
}

static BOOL CALLBACK InvalidateThemeChild(HWND child, LPARAM) {
    // RDW_FRAME: also repaint the non-client frame of input fields with the theme color
    RedrawWindow(child, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
    return TRUE;
}

static void ApplyThemeToControls(HWND hwnd) {
    // Theme button is icon-only; its owner-draw code changes the moon/sun icon.
    SetWindowTextA(g_hBtnTheme, "");
    InvalidateRect(hwnd, NULL, TRUE);
    EnumChildWindows(hwnd, InvalidateThemeChild, 0);
    if (g_hVarPopupDlg) { InvalidateRect(g_hVarPopupDlg, NULL, TRUE); EnumChildWindows(g_hVarPopupDlg, InvalidateThemeChild, 0); }
    if (g_hPopupDlg) { InvalidateRect(g_hPopupDlg, NULL, TRUE); EnumChildWindows(g_hPopupDlg, InvalidateThemeChild, 0); }
    UpdateScriptViewText();   // update code palette + background for the theme

    // Theme change sends WM_SIZE/WM_PAINT through the window hierarchy.
    // Re-apply the current view visibility explicitly so the Script RichEdit
    // can never become visible while the Visual Editor is active.
    bool showScript = (g_currentView == VIEW_SCRIPT);
    SetCtrlVisible(g_hEditScriptLines, showScript);
    SetCtrlVisible(g_hEditScriptView, showScript);
    SetCtrlVisible(g_hBtnCopyScript, showScript);
    SetCtrlVisible(g_hLblE2Name, showScript);
    SetCtrlVisible(g_hEditE2Name, showScript);

    UpdateWindow(hwnd);
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// Y position (pixels, in the code box's client coords) of the given line
static int GetScriptLineY(int line) {
    int idx = (int)SendMessage(g_hEditScriptView, EM_LINEINDEX, (WPARAM)line, 0);
    if (idx < 0) return INT_MIN;
    if (g_scriptIsRich) {
        POINTL pl = { 0, 0 };
        SendMessage(g_hEditScriptView, EM_POSFROMCHAR, (WPARAM)&pl, (LPARAM)idx);
        return (int)pl.y;
    }
    LRESULT r = SendMessage(g_hEditScriptView, EM_POSFROMCHAR, (WPARAM)idx, 0);
    return (int)(short)HIWORD(r);
}

// Draw line-number gutter: dim numbers + very dim right divider, aligned with the code box's real lines
static void DrawScriptGutter(const DRAWITEMSTRUCT* d) {
    E2Palette P = E2GetPalette(g_darkTheme);
    HDC hdc = d->hDC;
    RECT rc = d->rcItem;

    HBRUSH bg = CreateSolidBrush(P.gutterBg);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    HPEN pen = CreatePen(PS_SOLID, 1, P.gutterLine);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, rc.right - 1, rc.top, NULL);
    LineTo(hdc, rc.right - 1, rc.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    if (!g_hEditScriptView) return;

    HFONT oldFont = (HFONT)SelectObject(hdc, g_hCodeFont ? g_hCodeFont : g_hUIFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, P.gutterNum);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    RECT vr;
    GetClientRect(g_hEditScriptView, &vr);

    int total = (int)SendMessage(g_hEditScriptView, EM_GETLINECOUNT, 0, 0);
    int first = (int)SendMessage(g_hEditScriptView, EM_GETFIRSTVISIBLELINE, 0, 0);
    for (int ln = first; ln < total; ln++) {
        int y = GetScriptLineY(ln);
        if (y == INT_MIN || y > vr.bottom) break;
        char num[16];
        snprintf(num, sizeof(num), "%d", ln + 1);
        RECT tr = { rc.left, y, rc.right - 8, y + tm.tmHeight };
        DrawTextA(hdc, num, -1, &tr, DT_RIGHT | DT_SINGLELINE | DT_NOPREFIX);
    }
    SelectObject(hdc, oldFont);
}

// Redraw the gutter when the code scrolls (force = always)
static void SyncScriptGutter(bool force = false) {
    if (!g_hEditScriptView || !g_hEditScriptLines) return;
    int key;
    if (g_scriptIsRich) {
        POINT pt = { 0, 0 };
        SendMessage(g_hEditScriptView, EM_GETSCROLLPOS, 0, (LPARAM)&pt);
        key = pt.y;
    } else {
        key = (int)SendMessage(g_hEditScriptView, EM_GETFIRSTVISIBLELINE, 0, 0);
    }
    if (force || key != g_gutterLastScroll) {
        g_gutterLastScroll = key;
        InvalidateRect(g_hEditScriptLines, NULL, FALSE);
    }
}

void UpdateScriptViewText() {
    // On the Visual page: don't touch the Script box (it used to pop up on top); it updates itself when switching to Script
    if (g_currentView != VIEW_SCRIPT) return;
    string code = HudExporter::generate_e2_code(g_shapes, g_variables, g_useInterval, g_intervalMs, g_e2Name);
    if (g_scriptIsRich) {
        E2ApplyToRichEdit(g_hEditScriptView, code, E2GetPalette(g_darkTheme));   // apply E2-editor-style token colors
    } else {
        SetWindowTextA(g_hEditScriptView, code.c_str());
    }
    SyncScriptGutter(true);
}

void UpdateParentComboBox() {
    if (!g_hComboParent) return;
    SendMessage(g_hComboParent, WM_SETREDRAW, FALSE, 0);
    SendMessage(g_hComboParent, CB_RESETCONTENT, 0, 0);
    SendMessageA(g_hComboParent, CB_ADDSTRING, 0, (LPARAM)"[ None ]");
    for (size_t i = 0; i < g_shapes.size(); i++) {
        if ((int)i == g_selectedShapeIndex) continue;
        string itemStr = g_shapes[i].name;
        SendMessageA(g_hComboParent, CB_ADDSTRING, 0, (LPARAM)itemStr.c_str());
    }
    
    if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
        int parentIdx = g_shapes[g_selectedShapeIndex].parentIndex;
        if (parentIdx == -1) {
            SendMessage(g_hComboParent, CB_SETCURSEL, 0, 0);
        } else {
            int sel = 1;
            for (int i = 0; i < parentIdx; i++) {
                if (i != g_selectedShapeIndex) sel++;
            }
            SendMessage(g_hComboParent, CB_SETCURSEL, sel, 0);
        }
    } else {
        SendMessage(g_hComboParent, CB_SETCURSEL, 0, 0);
    }
    SendMessage(g_hComboParent, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hComboParent, NULL, FALSE);
}

void PopulateShapesList(HWND hList) {
    if (!hList) return;
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < g_shapes.size(); i++) {
        string name = g_shapes[i].name + ": ";
        switch (g_shapes[i].type) {
        case TOOL_LINE: name += "Line"; break;
        case TOOL_RECT: name += "Rect Outline"; break;
        case TOOL_BOX: name += "Box (Solid)"; break;
        case TOOL_CIRCLE_OUTLINE: name += "Circle (Outline)"; break;
        case TOOL_CIRCLE_SOLID: name += "Circle (Solid)"; break;
        case TOOL_TEXT: name += "Text (" + g_shapes[i].textContent + ")"; break;
        default: name += "Unknown"; break;
        }
        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)name.c_str());
    }
    if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size())
        SendMessage(hList, LB_SETCURSEL, g_selectedShapeIndex, 0);
}

LRESULT CALLBACK ManageShapesPopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hPopupList = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            10, 10, 260, 180, hwnd, (HMENU)ID_DLG_LISTBOX, GetModuleHandle(NULL), NULL);
        
        HWND hBtnDel = CreateWindowA("BUTTON", "Delete", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            10, 200, 80, 25, hwnd, (HMENU)ID_DLG_BTN_DELETE, GetModuleHandle(NULL), NULL);
        
        HWND hBtnClear = CreateWindowA("BUTTON", "Clear All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            100, 200, 80, 25, hwnd, (HMENU)ID_DLG_BTN_CLEARALL, GetModuleHandle(NULL), NULL);

        HWND hBtnClose = CreateWindowA("BUTTON", "Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
            190, 200, 80, 25, hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);

        SendMessage(g_hPopupList, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(hBtnDel, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(hBtnClear, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(hBtnClose, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

        PopulateShapesList(g_hPopupList);
        break;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam; RECT rc; GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, ThemeBrush(ThemeWindowColor())); return 1;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        return ThemeCtlColor(msg, (HDC)wParam, (HWND)lParam);
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT d = (LPDRAWITEMSTRUCT)lParam;
        if (d && d->CtlType == ODT_BUTTON) { DrawIconButtonItem(d); return TRUE; }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        // Editing visual properties must never expose the Script view.
        // Keep all Script controls hidden while the Visual Editor is active.
        HideScriptControlsInVisual();
        if (id == ID_DLG_LISTBOX && code == LBN_SELCHANGE) {
            int sel = (int)SendMessage(g_hPopupList, LB_GETCURSEL, 0, 0);
            if (sel >= 0 && sel < (int)g_shapes.size()) {
                g_selectedShapeIndex = sel;
                const DrawShape& s = g_shapes[sel];
                g_currentColor = s.color;
                g_currentThickness = s.thickness;
                g_currentText = s.textContent;
                g_currentTextSize = s.textSize;
                g_currentIsInput = s.isInput;
                g_currentVarName = s.varName;
                g_currentInputType = s.inputType;
                g_currentPosX = s.p1.first;
                g_currentPosY = s.p1.second;
                g_currentAngle = s.angle;
                g_currentParentIndex = s.parentIndex;
                g_currentUseVarX = s.useVarX;
                g_currentUseVarY = s.useVarY;
                g_currentUseVarSizeX = s.useVarSizeX;
                g_currentUseVarSizeY = s.useVarSizeY;
                g_currentUseVarAngle = s.useVarAngle;
                g_currentVarSizeXName = s.varSizeXName.empty() ? "SizeX" : s.varSizeXName;
                g_currentVarSizeYName = s.varSizeYName.empty() ? "SizeY" : s.varSizeYName;
                g_currentVarAngleName = s.varAngleName.empty() ? "Angle" : s.varAngleName;
                g_currentVarXName = s.varXName;
                g_currentVarYName = s.varYName;
                g_currentIsEgp3D = s.isEgp3D;
                g_currentVar3DName = s.var3DName.empty() ? "Pos" : s.var3DName;
                SetTextIfChanged(g_hEditOpacity, to_string(s.opacity).c_str());
                SetTextIfChanged(g_hEditThick, to_string(s.thickness).c_str());
                SetTextIfChanged(g_hEditTextContent, s.textContent.c_str());
                SetTextIfChanged(g_hEditTextSize, to_string(s.textSize).c_str());
                SetTextIfChanged(g_hEditTextVarName, s.varName.c_str());
                SetTextIfChanged(g_hEditPosX, to_string((int)s.p1.first).c_str());
                SetTextIfChanged(g_hEditPosY, to_string((int)s.p1.second).c_str());
                SetTextIfChanged(g_hEditAngle, to_string((int)std::round(s.angle)).c_str());
                double uiSizeW = std::abs(s.p2.first - s.p1.first);
                double uiSizeH = std::abs(s.p2.second - s.p1.second);
                if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) { double rr = std::sqrt((s.p2.first-s.p1.first)*(s.p2.first-s.p1.first) + (s.p2.second-s.p1.second)*(s.p2.second-s.p1.second)); uiSizeW = uiSizeH = rr; }
                SetTextIfChanged(g_hEditSizeX, to_string((int)std::round(uiSizeW)).c_str());
                SetTextIfChanged(g_hEditSizeY, to_string((int)std::round(uiSizeH)).c_str());
                SetTextIfChanged(g_hEditVarXName, s.varXName.c_str());
                SetTextIfChanged(g_hEditVarYName, s.varYName.c_str());
                SetTextIfChanged(g_hEditVarSizeXName, g_currentVarSizeXName.c_str());
                SetTextIfChanged(g_hEditVarSizeYName, g_currentVarSizeYName.c_str());
                SetTextIfChanged(g_hEditVarAngleName, g_currentVarAngleName.c_str());
                SetTextIfChanged(g_hEditVar3DName, g_currentVar3DName.c_str());
                SendMessage(g_hChkIsInput, BM_SETCHECK, s.isInput ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(g_hComboInputType, CB_SETCURSEL, (WPARAM)s.inputType, 0);
                SendMessage(g_hChkUseVarX, BM_SETCHECK, s.useVarX ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(g_hChkUseVarY, BM_SETCHECK, s.useVarY ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(g_hChkUseVarSizeX, BM_SETCHECK, s.useVarSizeX ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(g_hChkUseVarSizeY, BM_SETCHECK, s.useVarSizeY ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(g_hChkUseVarAngle, BM_SETCHECK, s.useVarAngle ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(g_hChkIsEgp3D, BM_SETCHECK, s.isEgp3D ? BST_CHECKED : BST_UNCHECKED, 0);
                UpdateParentComboBox();
                UpdateInputControlsState();
                UpdateAttributeText();
                InvalidateRect(g_hBtnColorPicker, NULL, TRUE);
                RECT rc; GetClientRect(g_hMainWnd, &rc);
                RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                InvalidateRect(g_hMainWnd, &cRect, FALSE);
            }
        }
        else if (id == ID_DLG_BTN_DELETE) {
            int sel = (int)SendMessage(g_hPopupList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR && sel < (int)g_shapes.size()) {
                g_shapes.erase(g_shapes.begin() + sel);
                g_selectedShapeIndex = g_shapes.empty() ? -1 : min(sel, (int)g_shapes.size() - 1);
                PopulateShapesList(g_hPopupList);
                UpdateParentComboBox();
                UpdateAttributeText();
                AddLog(g_hMainWnd, "Deleted Shape from Manage Dialog.");
                RECT rc; GetClientRect(g_hMainWnd, &rc);
                RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                InvalidateRect(g_hMainWnd, &cRect, FALSE);
            }
        }
        else if (id == ID_DLG_BTN_CLEARALL) {
            g_shapes.clear();
            g_selectedShapeIndex = -1;
            PopulateShapesList(g_hPopupList);
            UpdateParentComboBox();
            UpdateAttributeText();
            AddLog(g_hMainWnd, "Cleared all shapes from Manage Dialog.");
            RECT rc; GetClientRect(g_hMainWnd, &rc);
            RECT cRect = GetCanvasRect(rc.right, rc.bottom);
            InvalidateRect(g_hMainWnd, &cRect, FALSE);
        }
        else if (id == IDCANCEL) {
            DestroyWindow(hwnd);
            g_hPopupDlg = NULL;
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        g_hPopupDlg = NULL;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


static int FindVariableIndexByName(const string& name) {
    for (size_t i = 0; i < g_variables.size(); ++i) {
        if (g_variables[i].name == name) return (int)i;
    }
    return -1;
}

static void SyncTextVariableIntoManager(const DrawShape& s) {
    if (s.type != TOOL_TEXT || !s.isInput || s.varName.empty()) return;
    int idx = FindVariableIndexByName(s.varName);
    if (idx < 0) g_variables.push_back(E2Variable(s.varName, s.inputType, VARIABLE_INPUT));
}

static void SyncAllTextVariablesIntoManager() {
    for (const DrawShape& s : g_shapes) SyncTextVariableIntoManager(s);
}

static string MakeUniqueVariableName() {
    SyncAllTextVariablesIntoManager();
    for (int n = 1; n < 10000; ++n) {
        string candidate = "Var" + to_string(n);
        if (FindVariableIndexByName(candidate) < 0) return candidate;
    }
    return "Var" + to_string((int)g_variables.size() + 1);
}

static void PopulateVariablesList(HWND hList) {
    if (!hList) return;
    SyncAllTextVariablesIntoManager();
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    for (const E2Variable& v : g_variables) {
        string dir = (v.direction == VARIABLE_INPUT) ? "Input" : (v.direction == VARIABLE_OUTPUT ? "Output" : "Persist");
        string item = v.name + " : " + InputTypeToString(v.type) + "  [" + dir + "]";
        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)item.c_str());
    }
}

// Draw each variable row in E2 colors: name = variable, type = type, [Input/Output/Persist] = directive
static void DrawVariableListItem(LPDRAWITEMSTRUCT d) {
    if (!d || d->itemID == (UINT)-1) return;
    HDC hdc = d->hDC;
    RECT rc = d->rcItem;
    const bool sel = (d->itemState & ODS_SELECTED) != 0;
    E2Palette P = E2GetPalette(g_darkTheme);
    COLORREF bg = sel ? (g_darkTheme ? RGB(64, 68, 76) : RGB(204, 226, 250)) : ThemeInputColor();
    HBRUSH bb = CreateSolidBrush(bg);
    FillRect(hdc, &rc, bb);
    DeleteObject(bb);
    if (d->itemID >= g_variables.size()) return;

    const E2Variable& v = g_variables[d->itemID];
    const char* dir = (v.direction == VARIABLE_INPUT) ? "Input" : (v.direction == VARIABLE_OUTPUT ? "Output" : "Persist");
    struct Seg { std::string text; COLORREF col; };
    Seg segs[] = {
        { v.name,                   P.variable  },
        { " : ",                    P.text      },
        { InputTypeToString(v.type), P.type     },
        { "  [",                    P.text      },
        { dir,                      P.directive },
        { "]",                      P.text      }
    };

    SetBkMode(hdc, TRANSPARENT);
    TEXTMETRICA tm; GetTextMetricsA(hdc, &tm);
    int x = rc.left + 6;
    int y = rc.top + ((rc.bottom - rc.top) - tm.tmHeight) / 2;
    for (size_t i = 0; i < sizeof(segs) / sizeof(segs[0]); i++) {
        SetTextColor(hdc, segs[i].col);
        TextOutA(hdc, x, y, segs[i].text.c_str(), (int)segs[i].text.size());
        SIZE sz = {0, 0};
        GetTextExtentPoint32A(hdc, segs[i].text.c_str(), (int)segs[i].text.size(), &sz);
        x += sz.cx;
    }
}

static int VariableListToIndex(int listIndex) {
    if (listIndex < 0 || listIndex >= (int)g_variables.size()) return -1;
    return listIndex;
}

static void RefreshVariablePopup() {
    if (g_hVarPopupList) PopulateVariablesList(g_hVarPopupList);
}

LRESULT CALLBACK VariablePopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hVarPopupList = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
            10, 10, 300, 155, hwnd, (HMENU)ID_VAR_LIST, GetModuleHandle(NULL), NULL);

        g_hVarNameLabel = CreateWindowA("STATIC", "Name", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
            10, 174, 45, 22, hwnd, NULL, GetModuleHandle(NULL), NULL);
        g_hVarNameEdit = CreateWindowA("EDIT", "Var1", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            58, 172, 145, 22, hwnd, (HMENU)ID_VAR_EDIT_NAME, GetModuleHandle(NULL), NULL);

        g_hVarTypeLabel = CreateWindowA("STATIC", "Type", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
            210, 174, 35, 22, hwnd, NULL, GetModuleHandle(NULL), NULL);
        g_hVarTypeCombo = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            247, 171, 63, 100, hwnd, (HMENU)ID_VAR_COMBO_TYPE, GetModuleHandle(NULL), NULL);
        SendMessageA(g_hVarTypeCombo, CB_ADDSTRING, 0, (LPARAM)"Num");
        SendMessageA(g_hVarTypeCombo, CB_ADDSTRING, 0, (LPARAM)"Vector");
        SendMessageA(g_hVarTypeCombo, CB_ADDSTRING, 0, (LPARAM)"String");
        SendMessage(g_hVarTypeCombo, CB_SETCURSEL, 0, 0);

        HWND hVarDirLabel = CreateWindowA("STATIC", "Direction", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
            10, 198, 55, 22, hwnd, NULL, GetModuleHandle(NULL), NULL);
        g_hVarDirectionCombo = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            70, 197, 100, 100, hwnd, (HMENU)ID_VAR_COMBO_DIRECTION, GetModuleHandle(NULL), NULL);
        SendMessageA(g_hVarDirectionCombo, CB_ADDSTRING, 0, (LPARAM)"Input");
        SendMessageA(g_hVarDirectionCombo, CB_ADDSTRING, 0, (LPARAM)"Output");
        SendMessageA(g_hVarDirectionCombo, CB_ADDSTRING, 0, (LPARAM)"Persist");
        SendMessage(g_hVarDirectionCombo, CB_SETCURSEL, 0, 0);

        g_hVarExpressionLabel = CreateWindowA("STATIC", "Expression", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP,
            10, 226, 65, 22, hwnd, NULL, GetModuleHandle(NULL), NULL);
        g_hVarExpressionEdit = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            78, 223, 232, 22, hwnd, (HMENU)ID_VAR_EDIT_EXPRESSION, GetModuleHandle(NULL), NULL);

        HWND hAdd = CreateWindowA("BUTTON", "Add", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            10, 257, 70, 25, hwnd, (HMENU)ID_VAR_BTN_ADD, GetModuleHandle(NULL), NULL);
        HWND hRename = CreateWindowA("BUTTON", "Rename", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            85, 257, 70, 25, hwnd, (HMENU)ID_VAR_BTN_RENAME, GetModuleHandle(NULL), NULL);
        HWND hDelete = CreateWindowA("BUTTON", "Delete", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            160, 257, 70, 25, hwnd, (HMENU)ID_VAR_BTN_DELETE, GetModuleHandle(NULL), NULL);
        HWND hClose = CreateWindowA("BUTTON", "Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_OWNERDRAW,
            235, 257, 75, 25, hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);

        HWND controls[] = { g_hVarNameLabel, g_hVarTypeLabel, hVarDirLabel, g_hVarExpressionLabel, g_hVarPopupList, g_hVarNameEdit, g_hVarTypeCombo, g_hVarDirectionCombo, g_hVarExpressionEdit, hAdd, hRename, hDelete, hClose };
        for (HWND h : controls) SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        // slightly larger font for the variable list (row height follows the font)
        if (!g_hVarListFont) g_hVarListFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
        SendMessage(g_hVarPopupList, WM_SETFONT, (WPARAM)g_hVarListFont, TRUE);
        SubclassEditControl(g_hVarNameEdit);
        PopulateVariablesList(g_hVarPopupList);
        break;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam; RECT rc; GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, ThemeBrush(ThemeWindowColor())); return 1;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        return ThemeCtlColor(msg, (HDC)wParam, (HWND)lParam);
    case WM_MEASUREITEM: {
        LPMEASUREITEMSTRUCT m = (LPMEASUREITEMSTRUCT)lParam;
        if (m && m->CtlType == ODT_LISTBOX && m->CtlID == ID_VAR_LIST) { m->itemHeight = 20; return TRUE; }
        break;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT d = (LPDRAWITEMSTRUCT)lParam;
        if (d && d->CtlType == ODT_BUTTON) { DrawIconButtonItem(d); return TRUE; }
        if (d && d->CtlType == ODT_LISTBOX && d->CtlID == ID_VAR_LIST) { DrawVariableListItem(d); return TRUE; }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == ID_VAR_LIST && code == LBN_SELCHANGE) {
            int sel = (int)SendMessage(g_hVarPopupList, LB_GETCURSEL, 0, 0);
            int varIdx = VariableListToIndex(sel);
            if (varIdx >= 0) {
                const E2Variable& v = g_variables[varIdx];
                SetWindowTextA(g_hVarNameEdit, v.name.c_str());
                SendMessage(g_hVarTypeCombo, CB_SETCURSEL, (WPARAM)min((int)v.type, (int)INPUT_STRING), 0);
                SendMessage(g_hVarDirectionCombo, CB_SETCURSEL, (WPARAM)v.direction, 0);
                SetWindowTextA(g_hVarExpressionEdit, v.expression.c_str());
            }
        }
        else if (id == ID_VAR_COMBO_TYPE && code == CBN_SELCHANGE) {
            int sel = (int)SendMessage(g_hVarPopupList, LB_GETCURSEL, 0, 0);
            int varIdx = VariableListToIndex(sel);
            int typeSel = (int)SendMessage(g_hVarTypeCombo, CB_GETCURSEL, 0, 0);
            if (varIdx >= 0 && typeSel >= INPUT_NUM && typeSel <= INPUT_STRING) {
                g_variables[varIdx].type = (InputType)typeSel;
                RefreshVariablePopup();
                SendMessage(g_hVarPopupList, LB_SETCURSEL, varIdx, 0);
            }
        }
        else if (id == ID_VAR_COMBO_DIRECTION && code == CBN_SELCHANGE) {
            int sel = (int)SendMessage(g_hVarPopupList, LB_GETCURSEL, 0, 0);
            int varIdx = VariableListToIndex(sel);
            int dirSel = (int)SendMessage(g_hVarDirectionCombo, CB_GETCURSEL, 0, 0);
            if (varIdx >= 0 && (dirSel == 0 || dirSel == 1 || dirSel == 2)) {
                g_variables[varIdx].direction = (dirSel == 1) ? VARIABLE_OUTPUT : (dirSel == 2 ? VARIABLE_PERSIST : VARIABLE_INPUT);
                RefreshVariablePopup();
                SendMessage(g_hVarPopupList, LB_SETCURSEL, varIdx, 0);
            }
        }
        else if (id == ID_VAR_EDIT_EXPRESSION && code == EN_CHANGE) {
            int sel = (int)SendMessage(g_hVarPopupList, LB_GETCURSEL, 0, 0);
            int varIdx = VariableListToIndex(sel);
            if (varIdx >= 0) {
                char exprBuf[512] = {0};
                GetWindowTextA(g_hVarExpressionEdit, exprBuf, sizeof(exprBuf));
                g_variables[varIdx].expression = exprBuf;
                RefreshVariablePopup();
                SendMessage(g_hVarPopupList, LB_SETCURSEL, sel, 0);
            }
        }
        else if (id == ID_VAR_BTN_ADD) {
            char nameBuf[128] = {0};
            GetWindowTextA(g_hVarNameEdit, nameBuf, sizeof(nameBuf));
            string varName = nameBuf;
            if (varName.empty()) varName = MakeUniqueVariableName();
            if (FindVariableIndexByName(varName) >= 0) {
                varName = MakeUniqueVariableName();
            }

            int typeSel = (int)SendMessage(g_hVarTypeCombo, CB_GETCURSEL, 0, 0);
            InputType type = (typeSel >= INPUT_NUM && typeSel <= INPUT_STRING) ? (InputType)typeSel : INPUT_NUM;
            int dirSel = (int)SendMessage(g_hVarDirectionCombo, CB_GETCURSEL, 0, 0);
            VariableDirection direction = (dirSel == 1) ? VARIABLE_OUTPUT : (dirSel == 2 ? VARIABLE_PERSIST : VARIABLE_INPUT);

            char exprBuf[512] = {0};
            GetWindowTextA(g_hVarExpressionEdit, exprBuf, sizeof(exprBuf));
            E2Variable newVar(varName, type, direction);
            newVar.expression = exprBuf;
            g_variables.push_back(newVar);
            RefreshVariablePopup();
            int newSel = (int)g_variables.size() - 1;
            SendMessage(g_hVarPopupList, LB_SETCURSEL, newSel, 0);
            SetWindowTextA(g_hVarNameEdit, varName.c_str());
            AddLog(g_hMainWnd, "Added E2 variable: " + varName);
        }
        else if (id == ID_VAR_BTN_RENAME) {
            int sel = (int)SendMessage(g_hVarPopupList, LB_GETCURSEL, 0, 0);
            int varIdx = VariableListToIndex(sel);
            if (varIdx >= 0) {
                char nameBuf[128] = {0};
                GetWindowTextA(g_hVarNameEdit, nameBuf, sizeof(nameBuf));
                string newName = nameBuf;
                if (!newName.empty() && (newName == g_variables[varIdx].name || FindVariableIndexByName(newName) < 0)) {
                    string oldName = g_variables[varIdx].name;
                    g_variables[varIdx].name = newName;
                    // Keep a canvas Text variable with the old name in sync.
                    for (DrawShape& s : g_shapes) {
                        if (s.type == TOOL_TEXT && s.isInput && s.varName == oldName) {
                            s.varName = newName;
                        }
                    }
                    RefreshVariablePopup();
                    SendMessage(g_hVarPopupList, LB_SETCURSEL, varIdx, 0);
                    AddLog(g_hMainWnd, "Renamed variable: " + newName);
                }
            }
        }
        else if (id == ID_VAR_BTN_DELETE) {
            int sel = (int)SendMessage(g_hVarPopupList, LB_GETCURSEL, 0, 0);
            int varIdx = VariableListToIndex(sel);
            if (varIdx >= 0) {
                string oldName = g_variables[varIdx].name;
                for (DrawShape& s : g_shapes) {
                    if (s.type == TOOL_TEXT && s.isInput && s.varName == oldName) s.isInput = false;
                }
                g_variables.erase(g_variables.begin() + varIdx);
                RefreshVariablePopup();
                AddLog(g_hMainWnd, "Removed E2 variable: " + oldName);
            }
        }
        else if (id == IDCANCEL) {
            DestroyWindow(hwnd);
            g_hVarPopupDlg = NULL;
            g_hVarPopupList = NULL;
            g_hVarNameLabel = NULL;
            g_hVarTypeLabel = NULL;
            g_hVarNameEdit = NULL;
            g_hVarTypeCombo = NULL;
            g_hVarDirectionCombo = NULL;
            g_hVarExpressionLabel = NULL;
            g_hVarExpressionEdit = NULL;
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        g_hVarPopupDlg = NULL;
        g_hVarPopupList = NULL;
        g_hVarNameLabel = NULL;
        g_hVarTypeLabel = NULL;
        g_hVarNameEdit = NULL;
        g_hVarTypeCombo = NULL;
        g_hVarDirectionCombo = NULL;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowVariableManagePopup(HWND hwndOwner) {
    if (g_hVarPopupDlg) {
        SetFocus(g_hVarPopupDlg);
        RefreshVariablePopup();
        return;
    }

    RECT rcBtn;
    GetWindowRect(g_hBtnVarsPopup, &rcBtn);
    g_hVarPopupDlg = CreateWindowExA(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, "VariablePopupClass", "Variable Manage",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rcBtn.left, rcBtn.bottom + 5, 330, 335,
        hwndOwner, NULL, GetModuleHandle(NULL), NULL);
}

void ShowManageShapesPopup(HWND hwndOwner) {
    if (g_hPopupDlg) {
        SetFocus(g_hPopupDlg);
        PopulateShapesList(g_hPopupList);
        return;
    }

    RECT rcBtn;
    GetWindowRect(g_hBtnShapesPopup, &rcBtn);

    g_hPopupDlg = CreateWindowExA(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, "ManageShapesPopupClass", "Manage Canvas Shapes",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rcBtn.left, rcBtn.bottom + 5, 295, 275,
        hwndOwner, NULL, GetModuleHandle(NULL), NULL);
}

ToolType GetActiveToolType() {
    if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
        return g_shapes[g_selectedShapeIndex].type;
    }
    return g_currentTool;
}

void UpdateToolButtonHighlights() {
    bool hasIcon = (g_currentTool == TOOL_SELECT || g_currentTool == TOOL_LINE || g_currentTool == TOOL_RECT || 
                    g_currentTool == TOOL_BOX || g_currentTool == TOOL_CIRCLE_OUTLINE || 
                    g_currentTool == TOOL_CIRCLE_SOLID || g_currentTool == TOOL_TEXT);
    const char* prefix = hasIcon ? "       " : "";

    string textSelect = string(prefix) + (g_currentTool == TOOL_SELECT ? "[ Select / Move ]" : "Select / Move");
    string textLine   = string(prefix) + (g_currentTool == TOOL_LINE ? "[ Line ]" : "Line");
    string textRect   = string(prefix) + (g_currentTool == TOOL_RECT ? "[ Rect Outline ]" : "Rect Outline");
    string textBox    = string(prefix) + (g_currentTool == TOOL_BOX ? "[ Box (Solid) ]" : "Box (Solid)");
    string textCircleO= string(prefix) + (g_currentTool == TOOL_CIRCLE_OUTLINE ? "[ Circle Outline ]" : "Circle Outline");
    string textCircleS= string(prefix) + (g_currentTool == TOOL_CIRCLE_SOLID ? "[ Circle (Solid) ]" : "Circle (Solid)");
    string textText   = string(prefix) + (g_currentTool == TOOL_TEXT ? "[ Text ]" : "Text");

    SetTextIfChanged(g_hBtnSelect, textSelect.c_str());
    SetTextIfChanged(g_hBtnLine, textLine.c_str());
    SetTextIfChanged(g_hBtnRect, textRect.c_str());
    SetTextIfChanged(g_hBtnBox, textBox.c_str());
    SetTextIfChanged(g_hBtnCircleOutline, textCircleO.c_str());
    SetTextIfChanged(g_hBtnCircleSolid, textCircleS.c_str());
    SetTextIfChanged(g_hBtnText, textText.c_str());
}

void UpdateInputControlsState() {
    bool enableInput = g_currentIsInput;
    SetCtrlDim(g_hLblVarName, !enableInput);
    SetCtrlEnabled(g_hEditTextVarName, enableInput);
    SetCtrlDim(g_hLblType, !enableInput);
    SetCtrlEnabled(g_hComboInputType, enableInput);

    // EGP3D uses one vector3 expression (for example player:pos()) instead of the 2D X/Y controls.
    SetCtrlEnabled(g_hChkIsEgp3D, true);
    SetCtrlEnabled(g_hEditVar3DName, g_currentIsEgp3D);

    SetCtrlDim(g_hLblPosX, g_currentIsEgp3D);
    SetCtrlEnabled(g_hEditPosX, !g_currentIsEgp3D && !g_currentUseVarX);
    SetCtrlLocked(g_hChkUseVarX, g_currentIsEgp3D);
    SetCtrlEnabled(g_hEditVarXName, !g_currentIsEgp3D && g_currentUseVarX);
    
    SetCtrlDim(g_hLblPosY, g_currentIsEgp3D);
    SetCtrlEnabled(g_hEditPosY, !g_currentIsEgp3D && !g_currentUseVarY);
    SetCtrlLocked(g_hChkUseVarY, g_currentIsEgp3D);
    SetCtrlEnabled(g_hEditVarYName, !g_currentIsEgp3D && g_currentUseVarY);
    SetCtrlEnabled(g_hEditSizeX, !g_currentUseVarSizeX);
    SetCtrlEnabled(g_hEditSizeY, !g_currentUseVarSizeY);
    SetCtrlEnabled(g_hEditAngle, !g_currentUseVarAngle);
    SetCtrlEnabled(g_hEditVarSizeXName, g_currentUseVarSizeX);
    SetCtrlEnabled(g_hEditVarSizeYName, g_currentUseVarSizeY);
    SetCtrlEnabled(g_hEditVarAngleName, g_currentUseVarAngle);

    SetCtrlDim(g_hLblInterval, !g_useInterval);
    SetCtrlEnabled(g_hEditIntervalMs, g_useInterval);
}

static void UpdateAttributeText() {
    if (!g_hAttrText) return;
    std::string text;
    if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size()) {
        const DrawShape& s = g_shapes[g_selectedShapeIndex];
        // All values here come from HudExporter::compute_attr, the same source as the E2 code generation
        // (pre-Scale numbers), so they always match the script
        HudExporter::ShapeAttr a = HudExporter::compute_attr(g_shapes, (size_t)g_selectedShapeIndex);
        auto typeName = [&]() -> std::string {
            switch (s.type) {
                case TOOL_LINE: return "Line";
                case TOOL_RECT: return "Rect";
                case TOOL_BOX: return "Box";
                case TOOL_CIRCLE_OUTLINE: return "Circle (Outline)";
                case TOOL_CIRCLE_SOLID: return "Circle (Solid)";
                case TOOL_TEXT: return "Text";
                default: return "Shape";
            }
        };
        std::string parent = "[ None ]";
        if (a.parentIsTracker) parent = "3D Tracker (EGP " + std::to_string(a.parentId) + ")";
        else if (a.parentId > 0 && s.parentIndex >= 0 && s.parentIndex < (int)g_shapes.size())
            parent = g_shapes[s.parentIndex].name + " (EGP " + std::to_string(a.parentId) + ")";

        text = "Type\t" + typeName() + "\r\n";
        text += "EGP Index\t" + std::to_string(a.id) + "\r\n";
        text += "Parent\t" + parent + "\r\n";
        text += "Vec2 Pos\t(" + a.posX + "," + a.posY + ")\r\n";
        if (a.isLine) text += "Vec2 End\t(" + a.endX + "," + a.endY + ")\r\n";
        else if (a.hasSize) text += "Vec2 Size\t(" + a.sizeX + "," + a.sizeY + ")\r\n";
        text += "Angle\t" + a.angle + (s.useVarAngle ? "" : " deg");
        if (!a.isText) text += "\r\nLine Width\t" + a.thickness;
        if (s.type == TOOL_TEXT) {
            text += "\r\nText\t" + s.textContent;
            text += "\r\nText Size\t" + a.textSize;
            if (s.isInput) text += "\r\nInput\t" + s.varName + " : " + InputTypeToString(s.inputType);
        }
    }
    SetWindowTextA(g_hAttrText, text.c_str());
    InvalidateRect(g_hAttrText, NULL, TRUE);
}


// Measure text size (screen pixels) at the given font size
static SIZE MeasureTextShape(const DrawShape& s, int fontSize) {
    SIZE ts = { 0, fontSize };
    HDC hdc = GetDC(NULL);
    HFONT f = CreateFontA(fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
    HFONT o = (HFONT)SelectObject(hdc, f);
    GetTextExtentPoint32A(hdc, s.textContent.c_str(), (int)s.textContent.size(), &ts);
    SelectObject(hdc, o);
    DeleteObject(f);
    ReleaseDC(NULL, hdc);
    return ts;
}

static bool GetSelectedResizeHandleRect(HWND hwnd, RECT& out) {
    if (g_selectedShapeIndex < 0 || g_selectedShapeIndex >= (int)g_shapes.size()) return false;
    RECT rc; GetClientRect(hwnd, &rc); RECT c = GetCanvasRect(rc.right, rc.bottom);
    const DrawShape& s = g_shapes[g_selectedShapeIndex];
    Point a = GetAbsolutePosition(g_selectedShapeIndex);
    Point b = { a.first + (s.p2.first - s.p1.first), a.second + (s.p2.second - s.p1.second) };
    POINT p1 = CanvasToScreen(a,c), p2 = CanvasToScreen(b,c);
    if (s.type == TOOL_TEXT) {
        // text: real bounds = font size (p2 does not define size) -> handle sits at bottom-right of the text box
        SIZE ts = MeasureTextShape(s, s.textSize);
        POINT origin = CanvasToScreen(TransformCanvasPoint(g_selectedShapeIndex, a), c);
        POINT o = RotateScreenOffset(origin, ts.cx + 3, s.textSize + 3, TotalShapeAngle(g_selectedShapeIndex));
        const int hh = 12;
        out = { o.x - hh, o.y - hh, o.x + hh, o.y + hh };
        return true;
    }
    int r=0,l=0,t=0,bottom=0;
    if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) {
        r=(int)std::sqrt((double)(p2.x-p1.x)*(p2.x-p1.x)+(double)(p2.y-p1.y)*(p2.y-p1.y));
        l=p1.x-r; t=p1.y-r; bottom=p1.y+r;
    } else { l=min(p1.x,p2.x); t=min(p1.y,p2.y); bottom=max(p1.y,p2.y); }
    int right = (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) ? p1.x+r : max(p1.x,p2.x);
    int handle=12; out={right-handle,bottom-handle,right+handle,bottom+handle}; return true;
}

static bool PointInSelectedResizeHandle(HWND hwnd, POINT pt) { RECT h; return GetSelectedResizeHandleRect(hwnd,h) && PtInRect(&h,pt); }

static POINT SnapLineEnd45(POINT start, POINT end) {
    double dx = (double)end.x - start.x;
    double dy = (double)end.y - start.y;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0) return end;
    const double pi = 3.14159265358979323846;
    double a = std::atan2(dy, dx);
    double step = pi / 4.0;
    double snapped = std::round(a / step) * step;
    return POINT{ start.x + (LONG)std::lround(std::cos(snapped) * len),
                   start.y + (LONG)std::lround(std::sin(snapped) * len) };
}

// Shape bounds in canvas coords (ignores rotation); used for snap and Center
static bool GetShapeBoundsCanvas(int idx, RECT canvasRect, double& l, double& t, double& r, double& b) {
    if (idx < 0 || idx >= (int)g_shapes.size()) return false;
    const DrawShape& s = g_shapes[idx];
    Point a1 = GetAbsolutePosition(idx);
    Point a2 = { a1.first + (s.p2.first - s.p1.first), a1.second + (s.p2.second - s.p1.second) };
    if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) {
        double dx = a2.first - a1.first, dy = a2.second - a1.second;
        double rad = std::sqrt(dx * dx + dy * dy);
        l = a1.first - rad; r = a1.first + rad; t = a1.second - rad; b = a1.second + rad;
    } else if (s.type == TOOL_TEXT) {
        SIZE ts = MeasureTextShape(s, s.textSize);
        double sx = 1920.0 / std::max(1L, (long)(canvasRect.right - canvasRect.left));
        double sy = 1080.0 / std::max(1L, (long)(canvasRect.bottom - canvasRect.top));
        l = a1.first; t = a1.second; r = l + ts.cx * sx; b = t + s.textSize * sy;
    } else {
        l = std::min(a1.first, a2.first);   r = std::max(a1.first, a2.first);
        t = std::min(a1.second, a2.second); b = std::max(a1.second, a2.second);
    }
    return true;
}

static bool IsDescendantOfShape(int idx, int ancestor) {
    int depth = 0;
    while (idx >= 0 && idx < (int)g_shapes.size() && depth++ < 50) {
        idx = g_shapes[idx].parentIndex;
        if (idx == ancestor) return true;
    }
    return false;
}

// Compute extra offset (adjX, adjY) so the dragged shape's edges/center snap to other shapes or canvas edges/center
// offX/offY = how far the shape will move from its current position (raw mouse position)
static void ComputeMoveSnap(int idx, RECT canvasRect, double offX, double offY, double& adjX, double& adjY) {
    adjX = adjY = 0.0;
    double l, t, r, b;
    if (!GetShapeBoundsCanvas(idx, canvasRect, l, t, r, b)) return;
    l += offX; r += offX; t += offY; b += offY;

    const double kSnapPx = 8.0;   // snap distance in screen pixels
    double thrX = kSnapPx * 1920.0 / std::max(1L, (long)(canvasRect.right - canvasRect.left));
    double thrY = kSnapPx * 1080.0 / std::max(1L, (long)(canvasRect.bottom - canvasRect.top));

    std::vector<double> tx = { 0.0, 960.0, 1920.0 };
    std::vector<double> ty = { 0.0, 540.0, 1080.0 };
    for (int i = 0; i < (int)g_shapes.size(); i++) {
        if (i == idx || IsDescendantOfShape(i, idx)) continue;   // children of the dragged shape move with it
        double il, it, ir, ib;
        if (!GetShapeBoundsCanvas(i, canvasRect, il, it, ir, ib)) continue;
        tx.push_back(il); tx.push_back((il + ir) / 2.0); tx.push_back(ir);
        ty.push_back(it); ty.push_back((it + ib) / 2.0); ty.push_back(ib);
    }

    const double mx[3] = { l, (l + r) / 2.0, r };
    const double my[3] = { t, (t + b) / 2.0, b };
    bool hasX = false, hasY = false;
    for (int a = 0; a < 3; a++) for (size_t k = 0; k < tx.size(); k++) {
        double d = tx[k] - mx[a];
        if (std::fabs(d) <= thrX && (!hasX || std::fabs(d) < std::fabs(adjX))) { adjX = d; hasX = true; }
    }
    for (int a = 0; a < 3; a++) for (size_t k = 0; k < ty.size(); k++) {
        double d = ty[k] - my[a];
        if (std::fabs(d) <= thrY && (!hasY || std::fabs(d) < std::fabs(adjY))) { adjY = d; hasY = true; }
    }
}

void SwitchViewMode(ViewMode mode) {
    g_currentView = mode;

    int showVisual = (mode == VIEW_VISUAL) ? SW_SHOW : SW_HIDE;
    int showScript = (mode == VIEW_SCRIPT) ? SW_SHOW : SW_HIDE;

    ShowCtrl(g_hLblTools, showVisual);
    ShowCtrl(g_hBtnSelect, showVisual);
    ShowCtrl(g_hBtnLine, showVisual);
    ShowCtrl(g_hBtnRect, showVisual);
    ShowCtrl(g_hBtnBox, showVisual);
    ShowCtrl(g_hBtnCircleOutline, showVisual);
    ShowCtrl(g_hBtnCircleSolid, showVisual);
    ShowCtrl(g_hBtnText, showVisual);
    ShowCtrl(g_hBtnExport, showVisual);
    ShowCtrl(g_hBtnClear, showVisual);
    ShowCtrl(g_hBtnShapesPopup, showVisual);
    ShowCtrl(g_hBtnVarsPopup, showVisual);
    ShowCtrl(g_hComboBg, showVisual);
    ShowCtrl(g_hChkShowGrid, showVisual);

    ShowCtrl(g_hBtnFlipX, showVisual);
    ShowCtrl(g_hBtnFlipY, showVisual);
    ShowCtrl(g_hBtnCenter, showVisual);
    ShowCtrl(g_hBtnClone, showVisual);
    ShowCtrl(g_hAttrText, showVisual);

    ShowCtrl(g_hHeaderTransform, showVisual);
    ShowCtrl(g_hLblPosX, showVisual);
    ShowCtrl(g_hEditPosX, showVisual);
    ShowCtrl(g_hChkUseVarX, showVisual);
    ShowCtrl(g_hEditVarXName, showVisual);
    ShowCtrl(g_hLblPosY, showVisual);
    ShowCtrl(g_hEditPosY, showVisual);
    ShowCtrl(g_hChkUseVarY, showVisual);
    ShowCtrl(g_hEditVarYName, showVisual);
    ShowCtrl(g_hChkIsEgp3D, showVisual);
    ShowCtrl(g_hEditVar3DName, showVisual && g_currentIsEgp3D);

    ShowCtrl(g_hHeaderAppearance, showVisual);
    ShowCtrl(g_hLblColorName, showVisual);
    ShowCtrl(g_hBtnColorPicker, showVisual);

    ShowCtrl(g_hHeaderProperties, showVisual);
    ShowCtrl(g_hLblParent, showVisual);
    ShowCtrl(g_hComboParent, showVisual);

    ToolType activeType = GetActiveToolType();
    bool isText = (activeType == TOOL_TEXT);

    ShowCtrl(g_hLblThickName, (!isText) ? showVisual : SW_HIDE);
    ShowCtrl(g_hEditThick, (!isText) ? showVisual : SW_HIDE);

    ShowCtrl(g_hLblTextContentName, isText ? showVisual : SW_HIDE);
    ShowCtrl(g_hEditTextContent, isText ? showVisual : SW_HIDE);
    ShowCtrl(g_hLblTextSizeName, isText ? showVisual : SW_HIDE);
    ShowCtrl(g_hEditTextSize, isText ? showVisual : SW_HIDE);
    ShowCtrl(g_hChkIsInput, isText ? showVisual : SW_HIDE);

    ShowCtrl(g_hLblVarName, isText ? showVisual : SW_HIDE);
    ShowCtrl(g_hEditTextVarName, isText ? showVisual : SW_HIDE);
    ShowCtrl(g_hLblType, isText ? showVisual : SW_HIDE);
    ShowCtrl(g_hComboInputType, isText ? showVisual : SW_HIDE);

    UpdateInputControlsState();

    ShowCtrl(g_hBtnCopyScript, showScript);
    ShowCtrl(g_hLblE2Name, showScript);
    ShowCtrl(g_hEditE2Name, showScript);
    ShowCtrl(g_hEditScriptLines, showScript);
    ShowCtrl(g_hEditScriptView, showScript);

    if (mode == VIEW_SCRIPT) {
        UpdateScriptViewText();
    }

    if (g_hMainWnd) {
        RECT rc;
        GetClientRect(g_hMainWnd, &rc);
        SendMessage(g_hMainWnd, WM_SIZE, 0, MAKELONG(rc.right - rc.left, rc.bottom - rc.top));
    }
}

void InitUIControls(HWND hwnd) {
    InitCommonControls();

    g_hUIFont = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");

    g_hBoldFont = CreateFontA(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");

    g_hCodeFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

    g_hBtnTabVisual = CreateWindowA("BUTTON", "Visual Editor", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 10, 8, 110, 26, hwnd, (HMENU)ID_TAB_VISUAL, NULL, NULL);
    g_hBtnTabScript = CreateWindowA("BUTTON", "Script (Read-Only)", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 125, 8, 130, 26, hwnd, (HMENU)ID_TAB_SCRIPT, NULL, NULL);
    g_hBtnTheme = CreateWindowA("BUTTON", "", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 260, 8, 30, 26, hwnd, (HMENU)ID_BTN_THEME, NULL, NULL);

    g_hBtnShapesPopup = CreateWindowA("BUTTON", "Shape Manage", WS_VISIBLE | WS_CHILD | BS_LEFT | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_SHAPES_POPUP, NULL, NULL);
    // Owner-drawn, not subclassed (old subclass had a NULL proc and broke WM_PAINT)

    g_hBtnVarsPopup = CreateWindowA("BUTTON", "Variable Manage", WS_VISIBLE | WS_CHILD | BS_LEFT | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_VARS_POPUP, NULL, NULL);

    g_hComboBg = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hwnd, (HMENU)ID_COMBO_BG, NULL, NULL);
    SendMessageA(g_hComboBg, CB_ADDSTRING, 0, (LPARAM)"white");
    SendMessageA(g_hComboBg, CB_ADDSTRING, 0, (LPARAM)"sky1");
    SendMessageA(g_hComboBg, CB_ADDSTRING, 0, (LPARAM)"sky2");
    SendMessageA(g_hComboBg, CB_ADDSTRING, 0, (LPARAM)"dark bg");
    SendMessage(g_hComboBg, CB_SETCURSEL, 0, 0);

    g_hChkUseInterval     = CreateWindowA("BUTTON", "Enable Interval", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_USE_INTERVAL, NULL, NULL);
    SendMessage(g_hChkUseInterval, BM_SETCHECK, BST_CHECKED, 0);
    g_hChkShowGrid        = CreateWindowA("BUTTON", "Show Grid", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_SHOW_GRID, NULL, NULL);
    SendMessage(g_hChkShowGrid, BM_SETCHECK, g_showGrid ? BST_CHECKED : BST_UNCHECKED, 0);
    g_hLblInterval        = CreateWindowA("STATIC", " Interval (ms)", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hEditIntervalMs     = CreateWindowA("EDIT", "100", WS_VISIBLE | WS_CHILD | ES_NUMBER, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_INTERVAL_MS, NULL, NULL);

    g_hLblTools = CreateWindowA("STATIC", "TOOLS", WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);

    g_hBtnSelect        = CreateWindowA("BUTTON", "       Select / Move", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_SELECT, NULL, NULL);
    g_hBtnLine          = CreateWindowA("BUTTON", "       Line", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_LINE, NULL, NULL);
    g_hBtnRect          = CreateWindowA("BUTTON", "       Rect Outline", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_RECT, NULL, NULL);
    g_hBtnBox           = CreateWindowA("BUTTON", "       Box (Solid)", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_BOX, NULL, NULL);
    g_hBtnCircleOutline = CreateWindowA("BUTTON", "       Circle Outline", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_CIRCLE_OUTLINE, NULL, NULL);
    g_hBtnCircleSolid   = CreateWindowA("BUTTON", "       Circle (Solid)", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_CIRCLE_SOLID, NULL, NULL);
    g_hBtnText          = CreateWindowA("BUTTON", "       Text", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_TEXT, NULL, NULL);

    // Tool buttons are owner-drawn (WM_DRAWITEM), no subclass needed

    g_hBtnExport = CreateWindowA("BUTTON", "EXPORT E2", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_EXPORT, NULL, NULL);
    g_hBtnClear  = CreateWindowA("BUTTON", "CLEAR ALL", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_CLEAR, NULL, NULL);

    g_hHeaderTransform  = CreateWindowA("STATIC", " Transform", WS_VISIBLE | WS_CHILD | SS_LEFT, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hHeaderAppearance = CreateWindowA("STATIC", " Appearance", WS_VISIBLE | WS_CHILD | SS_LEFT, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hHeaderProperties = CreateWindowA("STATIC", " Properties", WS_VISIBLE | WS_CHILD | SS_LEFT, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);

    g_hLblColorName       = CreateWindowA("STATIC", " Color", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hLblOpacityName     = CreateWindowA("STATIC", " Opacity", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hEditOpacity        = CreateWindowA("EDIT", "255", WS_VISIBLE | WS_CHILD | ES_NUMBER, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_OPACITY, NULL, NULL);
    g_hLblThickName       = CreateWindowA("STATIC", " Border Size", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hLblTextContentName = CreateWindowA("STATIC", " Text Content", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hLblTextSizeName    = CreateWindowA("STATIC", " Text Size", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hLblVarName         = CreateWindowA("STATIC", " Variable Name", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hLblType            = CreateWindowA("STATIC", " Input Type", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);

    g_hLblPosX            = CreateWindowA("STATIC", " Pos X", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hEditPosX           = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD | ES_NUMBER, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_POS_X, NULL, NULL);
    g_hChkUseVarX         = CreateWindowA("BUTTON", "Var", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_USE_VAR_X, NULL, NULL);
    g_hEditVarXName       = CreateWindowA("EDIT", "X", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_VAR_X_NAME, NULL, NULL);

    g_hLblPosY            = CreateWindowA("STATIC", " Pos Y", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hEditPosY           = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD | ES_NUMBER, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_POS_Y, NULL, NULL);
    g_hChkUseVarY         = CreateWindowA("BUTTON", "Var", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_USE_VAR_Y, NULL, NULL);
    g_hEditVarYName       = CreateWindowA("EDIT", "Y", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_VAR_Y_NAME, NULL, NULL);
    g_hChkUseVarSizeX     = CreateWindowA("BUTTON", "Var", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_USE_VAR_SIZE_X, NULL, NULL);
    g_hChkUseVarSizeY     = CreateWindowA("BUTTON", "Var", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_USE_VAR_SIZE_Y, NULL, NULL);
    g_hChkUseVarAngle     = CreateWindowA("BUTTON", "Var", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_USE_VAR_ANGLE, NULL, NULL);
    g_hEditVarSizeXName   = CreateWindowA("EDIT", "SizeX", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_VAR_SIZE_X_NAME, NULL, NULL);
    g_hEditVarSizeYName   = CreateWindowA("EDIT", "SizeY", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_VAR_SIZE_Y_NAME, NULL, NULL);
    g_hEditVarAngleName   = CreateWindowA("EDIT", "Angle", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_VAR_ANGLE_NAME, NULL, NULL);
    g_hLblSizeX            = CreateWindowA("STATIC", " Size X", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hEditSizeX            = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD | ES_NUMBER, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_SIZE_X, NULL, NULL);
    g_hLblSizeY            = CreateWindowA("STATIC", " Size Y", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hEditSizeY            = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD | ES_NUMBER, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_SIZE_Y, NULL, NULL);
    g_hLblAngle             = CreateWindowA("STATIC", " Angle", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hEditAngle            = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_ANGLE, NULL, NULL);
    g_hChkIsEgp3D         = CreateWindowA("BUTTON", "Is EGP3D", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_IS_EGP3D, NULL, NULL);
    g_hEditVar3DName      = CreateWindowA("EDIT", "Pos", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_VAR_3D_NAME, NULL, NULL);

    g_hLblParent          = CreateWindowA("STATIC", " Parent Shape", WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hComboParent        = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hwnd, (HMENU)ID_COMBO_PARENT, NULL, NULL);

    g_hBtnColorPicker   = CreateWindowA("BUTTON", "", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_COLOR_PICKER, NULL, NULL);
    g_hEditThick        = CreateWindowA("EDIT", "1", WS_VISIBLE | WS_CHILD | ES_NUMBER, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_THICKNESS, NULL, NULL);
    g_hEditTextContent  = CreateWindowA("EDIT", "Text", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_TEXT_CONTENT, NULL, NULL);
    g_hEditTextSize     = CreateWindowA("EDIT", "24", WS_VISIBLE | WS_CHILD | ES_NUMBER, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_TEXT_SIZE, NULL, NULL);
    g_hChkIsInput       = CreateWindowA("BUTTON", "Is Input variable", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 0, 0, 0, 0, hwnd, (HMENU)ID_CHK_IS_INPUT, NULL, NULL);
    g_hEditTextVarName  = CreateWindowA("EDIT", "Var1", WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_TEXT_VARNAME, NULL, NULL);

    g_hComboInputType   = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hwnd, (HMENU)ID_COMBO_INPUT_TYPE, NULL, NULL);
    SendMessageA(g_hComboInputType, CB_ADDSTRING, 0, (LPARAM)"Num");
    SendMessageA(g_hComboInputType, CB_ADDSTRING, 0, (LPARAM)"Vector");
    SendMessageA(g_hComboInputType, CB_ADDSTRING, 0, (LPARAM)"String");
    SendMessageA(g_hComboInputType, CB_ADDSTRING, 0, (LPARAM)"Bool");
    SendMessage(g_hComboInputType, CB_SETCURSEL, 0, 0);

    g_hBtnFlipX   = CreateWindowA("BUTTON", "Flip X", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_FLIP_X, NULL, NULL);
    g_hBtnFlipY   = CreateWindowA("BUTTON", "Flip Y", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_FLIP_Y, NULL, NULL);
    g_hBtnCenter  = CreateWindowA("BUTTON", "Center Canvas", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_CENTER, NULL, NULL);
    g_hBtnClone   = CreateWindowA("BUTTON", "Clone Shape", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)ID_BTN_CLONE, NULL, NULL);

    g_hAttrText = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN,
        0, 0, 0, 0, hwnd, NULL, NULL, NULL);

    g_hBtnCopyScript   = CreateWindowA("BUTTON", "       Copy", WS_CHILD | BS_OWNERDRAW, 0, 0, 80, 28, hwnd, (HMENU)ID_BTN_COPY_SCRIPT, NULL, NULL);

    g_hLblE2Name  = CreateWindowA("STATIC", "@name", WS_CHILD | SS_LEFTNOWORDWRAP | SS_CENTERIMAGE, 0, 0, 0, 0, hwnd, NULL, NULL, NULL);
    g_hEditE2Name = CreateWindowA("EDIT", g_e2Name.c_str(),
        WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_E2NAME, NULL, NULL);
    SendMessage(g_hEditE2Name, EM_LIMITTEXT, 120, 0);

    // Gutter is owner-drawn (DrawScriptGutter) so numbers stay dim and match real lines
    g_hEditScriptLines = CreateWindowA("STATIC", "", WS_CHILD | SS_OWNERDRAW,
        0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_SCRIPT_LINES, NULL, NULL);

    // Code box is a RichEdit for syntax colors (no built-in border; frame drawn in WM_PAINT)
    LoadLibraryA("Msftedit.dll");
    g_hEditScriptView = CreateWindowExW(0, L"RICHEDIT50W", L"",
        WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY | ES_NOHIDESEL | WS_VSCROLL | WS_HSCROLL,
        0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_SCRIPT_VIEW, NULL, NULL);
    g_scriptIsRich = (g_hEditScriptView != NULL);
    if (g_scriptIsRich) {
        SendMessage(g_hEditScriptView, EM_EXLIMITTEXT, 0, 0x7FFFFFF);
        SendMessage(g_hEditScriptView, EM_SETEVENTMASK, 0, ENM_SCROLL);
        SendMessage(g_hEditScriptView, EM_SETTARGETDEVICE, 0, 1);   // disable word-wrap
    } else {
        // No Msftedit.dll: fall back to plain EDIT (no colors)
        g_hEditScriptView = CreateWindowA("EDIT", "", WS_CHILD | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL | WS_HSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)ID_EDIT_SCRIPT_VIEW, NULL, NULL);
    }
    g_OldScriptViewProc = (WNDPROC)SetWindowLongPtr(g_hEditScriptView, GWLP_WNDPROC, (LONG_PTR)ScriptViewSubclassProc);

    HWND uiControls[] = { g_hBtnTabVisual, g_hBtnTabScript, g_hLblTools, g_hBtnSelect, g_hBtnLine, g_hBtnRect, g_hBtnBox, g_hBtnCircleOutline, g_hBtnCircleSolid, g_hBtnText,
                         g_hBtnExport, g_hBtnClear, g_hBtnShapesPopup, g_hBtnVarsPopup, g_hComboBg, g_hLblColorName, g_hLblOpacityName, g_hEditOpacity, g_hLblThickName, g_hLblTextContentName, g_hLblTextSizeName, g_hLblVarName, g_hLblType, g_hEditThick, g_hEditTextContent, g_hEditTextSize, g_hChkIsInput, g_hEditTextVarName, g_hComboInputType,
                         g_hLblPosX, g_hEditPosX, g_hChkUseVarX, g_hEditVarXName, g_hLblPosY, g_hEditPosY, g_hChkUseVarY, g_hEditVarYName, g_hLblSizeX, g_hEditSizeX, g_hChkUseVarSizeX, g_hEditVarSizeXName, g_hLblSizeY, g_hEditSizeY, g_hChkUseVarSizeY, g_hEditVarSizeYName, g_hLblAngle, g_hEditAngle, g_hChkUseVarAngle, g_hEditVarAngleName, g_hChkIsEgp3D, g_hEditVar3DName, g_hLblParent, g_hComboParent,
                         g_hChkUseInterval, g_hLblInterval, g_hEditIntervalMs, g_hChkShowGrid,
                         g_hBtnFlipX, g_hBtnFlipY, g_hBtnCenter, g_hBtnClone, g_hAttrText, g_hBtnCopyScript, g_hLblE2Name, g_hEditE2Name, g_hBtnTheme };
    for (HWND h : uiControls) {
        SendMessage(h, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
    }
    SendMessage(g_hHeaderTransform, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
    SendMessage(g_hHeaderAppearance, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
    SendMessage(g_hHeaderProperties, WM_SETFONT, (WPARAM)g_hBoldFont, TRUE);
    SendMessage(g_hAttrText, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
    {
        int tabs[2] = { 78, 156 };
        SendMessage(g_hAttrText, EM_SETTABSTOPS, 2, (LPARAM)tabs);
    }

    SendMessage(g_hEditScriptLines, WM_SETFONT, (WPARAM)g_hCodeFont, TRUE);
    SendMessage(g_hEditScriptView, WM_SETFONT, (WPARAM)g_hCodeFont, TRUE);

    HWND editControls[] = { g_hEditOpacity, g_hEditThick, g_hEditTextContent, g_hEditTextSize,
                            g_hEditTextVarName, g_hEditPosX, g_hEditPosY, g_hEditVarXName,
                            g_hEditVarYName, g_hEditSizeX, g_hEditSizeY, g_hEditAngle, g_hEditVarSizeXName,
                            g_hEditVarSizeYName, g_hEditVarAngleName, g_hEditVar3DName, g_hEditIntervalMs, g_hEditE2Name };
    for (HWND h : editControls) SubclassEditControl(h);

    UpdateToolButtonHighlights();
    UpdateParentComboBox();
    UpdateAttributeText();
}

// Draw input field borders in theme colors (instead of the Windows 3D/black border)
static void PaintThemedEditFrame(HWND hwnd) {
    if (!hwnd) return;
    HDC hdc = GetWindowDC(hwnd);
    if (!hdc) return;
    RECT r; GetWindowRect(hwnd, &r);
    OffsetRect(&r, -r.left, -r.top);
    COLORREF frame = g_darkTheme ? RGB(92, 92, 92) : RGB(160, 160, 166);
    HBRUSH fb = CreateSolidBrush(frame);
    FrameRect(hdc, &r, fb);
    DeleteObject(fb);
    ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK EditCtrlSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    WNDPROC oldProc = (WNDPROC)GetPropA(hwnd, "AmbataOldEditProc");
    if (msg == WM_NCPAINT) {
        // Parameter EDIT controls intentionally have no custom/non-client frame.
        // Their themed background is painted by WM_CTLCOLOREDIT, avoiding the
        // stacked gray rectangles that appeared around the text fields.
        return oldProc ? CallWindowProc(oldProc, hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
    }
    if (msg == WM_NCDESTROY) {
        LRESULT result = oldProc ? CallWindowProc(oldProc, hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
        RemovePropA(hwnd, "AmbataOldEditProc");
        return result;
    }
    return oldProc ? CallWindowProc(oldProc, hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
}

static void SubclassEditControl(HWND h) {
    if (!h || GetPropA(h, "AmbataOldEditProc")) return;

    // Parameter EDITs use a single flat themed surface. Remove the native
    // non-client/3D border so it cannot stack another gray rectangle on top
    // of the themed background painted by WM_CTLCOLOREDIT.
    LONG_PTR style = GetWindowLongPtr(h, GWL_STYLE);
    style &= ~(WS_BORDER | WS_DLGFRAME | WS_THICKFRAME);
    SetWindowLongPtr(h, GWL_STYLE, style);
    LONG_PTR exStyle = GetWindowLongPtr(h, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME);
    SetWindowLongPtr(h, GWL_EXSTYLE, exStyle);
    SetWindowPos(h, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    WNDPROC oldProc = (WNDPROC)SetWindowLongPtr(h, GWLP_WNDPROC, (LONG_PTR)EditCtrlSubclassProc);
    SetPropA(h, "AmbataOldEditProc", (HANDLE)oldProc);
}

LRESULT CALLBACK ScriptViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    LRESULT r = CallWindowProc(g_OldScriptViewProc, hwnd, msg, wParam, lParam);
    switch (msg) {   // code scrolled by any means -> scroll the gutter too
    case WM_VSCROLL: case WM_MOUSEWHEEL: case WM_KEYDOWN: case WM_LBUTTONUP:
    case WM_MOUSEMOVE: case WM_SIZE: case WM_SETFOCUS: case WM_PAINT:
        SyncScriptGutter();
        break;
    }
    return r;
}

static void ApplyTitleBarTheme(HWND hwnd) {
    if (!hwnd) return;
    HMODULE hDwm = LoadLibraryA("dwmapi.dll");
    if (!hDwm) return;
    typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
    DwmSetWindowAttributeFn fn = (DwmSetWindowAttributeFn)GetProcAddress(hDwm, "DwmSetWindowAttribute");
    if (fn) {
        const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_LOCAL = 20;
        BOOL dark = g_darkTheme ? TRUE : FALSE;
        fn(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_LOCAL, &dark, sizeof(dark));
    }
    FreeLibrary(hDwm);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_hMainWnd = hwnd;
        ApplyTitleBarTheme(hwnd);
        InitUIControls(hwnd);
        
        g_hAppIcon = LoadEditorIconFromFile("images/app_icon.ico");
        
        if (g_hAppIcon) {
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hAppIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hAppIcon);
        }
        break;

    case WM_KEYDOWN: {
        if (wParam == VK_SHIFT && g_isDrawing && g_currentTool == TOOL_LINE) {
            g_endPt = SnapLineEnd45(g_startPt, g_endPt);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        if (wParam == VK_DELETE) {
            if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                string deletedName = g_shapes[g_selectedShapeIndex].name;
                g_shapes.erase(g_shapes.begin() + g_selectedShapeIndex);
                g_selectedShapeIndex = -1;
                UpdateParentComboBox();
                UpdateAttributeText();
                AddLog(hwnd, "Deleted Shape: " + deletedName);
                RECT rc; GetClientRect(hwnd, &rc);
                RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                InvalidateRect(hwnd, &cRect, FALSE);
            }
        }
        break;
    }

    case WM_KEYUP: {
        if (wParam == VK_SHIFT && g_isDrawing && g_currentTool == TOOL_LINE) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt); g_endPt = pt;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        break;
    }

    case WM_SETCURSOR:
        if (g_currentView == VIEW_VISUAL && g_currentTool == TOOL_SELECT) {
            POINT cp; GetCursorPos(&cp); ScreenToClient(hwnd, &cp);
            if (PointInSelectedResizeHandle(hwnd, cp)) {
                SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
                return TRUE;
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hStatic = (HWND)lParam;

        if (hStatic == g_hHeaderTransform || hStatic == g_hHeaderAppearance || hStatic == g_hHeaderProperties) {
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, ThemeHeaderColor());
            SetTextColor(hdcStatic, ThemeTextColor());
            return (LRESULT)ThemeBrush(ThemeHeaderColor());
        }
        if (hStatic == g_hAttrText) {   // read-only EDIT sends this instead of WM_CTLCOLOREDIT
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, ThemeAttrColor());
            SetTextColor(hdcStatic, ThemeTextColor());
            return (LRESULT)ThemeBrush(ThemeAttrColor());
        }
        // labels marked "dim" (prop AmbataDim) use muted color
        if (GetPropA(hStatic, "AmbataDim")) {
            SetBkMode(hdcStatic, TRANSPARENT);
            SetBkColor(hdcStatic, ThemeWindowColor());
            SetTextColor(hdcStatic, ThemeMutedTextColor());
            return (LRESULT)ThemeBrush(ThemeWindowColor());
        }
        return ThemeCtlColor(msg, hdcStatic, hStatic);
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        if ((HWND)lParam == g_hAttrText) {
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, ThemeAttrColor());
            SetTextColor(hdc, ThemeTextColor());
            return (LRESULT)ThemeBrush(ThemeAttrColor());
        }
        return ThemeCtlColor(msg, hdc, (HWND)lParam);
    }

    case WM_CTLCOLORLISTBOX:
        return ThemeCtlColor(msg, (HDC)wParam, (HWND)lParam);

    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        LRESULT r = ThemeCtlColor(msg, hdc, (HWND)lParam);
        if (IsCtrlLocked((HWND)lParam)) SetTextColor(hdc, ThemeMutedTextColor());
        return r;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        if (pDIS->CtlType == ODT_STATIC && pDIS->CtlID == ID_EDIT_SCRIPT_LINES) {
            DrawScriptGutter(pDIS);
            return TRUE;
        }
        if (pDIS->CtlType == ODT_BUTTON && IsIconButtonId((int)pDIS->CtlID)) {
            DrawIconButtonItem(pDIS);
            return TRUE;
        }
        if (pDIS->CtlID == ID_BTN_COLOR_PICKER) {
            HBRUSH hBrush = CreateSolidBrush(g_currentColor);
            FillRect(pDIS->hDC, &pDIS->rcItem, hBrush);
            FrameRect(pDIS->hDC, &pDIS->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
            DeleteObject(hBrush);
            return TRUE;
        }
        break;
    }

    case WM_SIZE: {
        int winW = LOWORD(lParam);
        int winH = HIWORD(lParam);
        // Never let the Script editor become visible while the Visual Editor is active.
        HideScriptControlsInVisual();
        bool vis = (g_currentView == VIEW_VISUAL);
        g_layoutChanged = false;

        PlaceCtrl(g_hBtnTabVisual, 10, 8, 110, 26);
        PlaceCtrl(g_hBtnTabScript, 125, 8, 130, 26);
        PlaceCtrl(g_hBtnTheme, max(5, winW - 40), 8, 30, 26);

        RECT cRect = GetCanvasRect(winW, winH);
        int startY = cRect.top;

        int btnH = 22;
        int btnW = 115;

        // Above Canvas Left: Shape Manage, Variable Manage
        PlaceCtrl(g_hBtnShapesPopup, cRect.left, startY - 26, 115, 22); SetCtrlVisible(g_hBtnShapesPopup, vis);
        PlaceCtrl(g_hBtnVarsPopup, cRect.left + 120, startY - 26, 115, 22); SetCtrlVisible(g_hBtnVarsPopup, vis);
        PlaceCtrl(g_hChkShowGrid, cRect.left + 245, startY - 26, 90, 22); SetCtrlVisible(g_hChkShowGrid, vis);

        // Above Canvas Right: BG Combo, Interval
        PlaceCtrl(g_hComboBg, cRect.right - 100, startY - 26, 100, 120); SetCtrlVisible(g_hComboBg, vis);
        PlaceCtrl(g_hChkUseInterval, cRect.right - 350, startY - 26, 110, 22); SetCtrlVisible(g_hChkUseInterval, vis);
        PlaceCtrl(g_hLblInterval, cRect.right - 235, startY - 26, 80, 22); SetCtrlVisible(g_hLblInterval, vis);
        PlaceCtrl(g_hEditIntervalMs, cRect.right - 150, startY - 24, 45, 18); SetCtrlVisible(g_hEditIntervalMs, vis);

        PlaceCtrl(g_hLblTools, 10, startY, btnW, 16);
        PlaceCtrl(g_hBtnSelect, 10, startY + 20, btnW, btnH);
        PlaceCtrl(g_hBtnLine, 10, startY + 45, btnW, btnH);
        PlaceCtrl(g_hBtnRect, 10, startY + 70, btnW, btnH);
        PlaceCtrl(g_hBtnBox, 10, startY + 95, btnW, btnH);
        PlaceCtrl(g_hBtnCircleOutline, 10, startY + 120, btnW, btnH);
        PlaceCtrl(g_hBtnCircleSolid, 10, startY + 145, btnW, btnH);
        PlaceCtrl(g_hBtnText, 10, startY + 170, btnW, btnH);
        PlaceCtrl(g_hBtnExport, 10, startY + 205, btnW, 25);
        PlaceCtrl(g_hBtnClear, 10, startY + 235, btnW, 25);

        int rLeft = winW - 300;
        int propW = 290;

        ToolType activeType = GetActiveToolType();
        bool isText = (activeType == TOOL_TEXT);

        // --- 1) Transform Category ---
        PlaceCtrl(g_hHeaderTransform, rLeft + 1, startY + 1, propW - 2, 19); SetCtrlVisible(g_hHeaderTransform, vis);

        // EGP3D toggle sits above the normal 2D position rows.
        PlaceCtrl(g_hChkIsEgp3D, rLeft + 1, startY + 21, 95, 20); SetCtrlVisible(g_hChkIsEgp3D, vis);
        PlaceCtrl(g_hEditVar3DName, rLeft + 108, startY + 23, propW - 110, 17); SetCtrlVisible(g_hEditVar3DName, vis && g_currentIsEgp3D);

        PlaceCtrl(g_hLblPosX, rLeft + 1, startY + 43, 103, 21); SetCtrlVisible(g_hLblPosX, vis);
        PlaceCtrl(g_hEditPosX, rLeft + 108, startY + 45, 42, 17); SetCtrlVisible(g_hEditPosX, vis);
        PlaceCtrl(g_hChkUseVarX, rLeft + 153, startY + 43, 38, 20); SetCtrlVisible(g_hChkUseVarX, vis);
        PlaceCtrl(g_hEditVarXName, rLeft + 193, startY + 45, 95, 17); SetCtrlVisible(g_hEditVarXName, vis);

        PlaceCtrl(g_hLblPosY, rLeft + 1, startY + 65, 103, 21); SetCtrlVisible(g_hLblPosY, vis);
        PlaceCtrl(g_hEditPosY, rLeft + 108, startY + 67, 42, 17); SetCtrlVisible(g_hEditPosY, vis);
        PlaceCtrl(g_hChkUseVarY, rLeft + 153, startY + 65, 38, 20); SetCtrlVisible(g_hChkUseVarY, vis);
        PlaceCtrl(g_hEditVarYName, rLeft + 193, startY + 67, 95, 17); SetCtrlVisible(g_hEditVarYName, vis);

        PlaceCtrl(g_hLblSizeX, rLeft + 1, startY + 87, 103, 21); SetCtrlVisible(g_hLblSizeX, vis);
        PlaceCtrl(g_hEditSizeX, rLeft + 108, startY + 89, 42, 17); SetCtrlVisible(g_hEditSizeX, vis);
        PlaceCtrl(g_hChkUseVarSizeX, rLeft + 153, startY + 87, 38, 20); SetCtrlVisible(g_hChkUseVarSizeX, vis);
        PlaceCtrl(g_hEditVarSizeXName, rLeft + 193, startY + 89, 95, 17); SetCtrlVisible(g_hEditVarSizeXName, vis);
        PlaceCtrl(g_hLblSizeY, rLeft + 1, startY + 109, 103, 21); SetCtrlVisible(g_hLblSizeY, vis);
        PlaceCtrl(g_hEditSizeY, rLeft + 108, startY + 111, 42, 17); SetCtrlVisible(g_hEditSizeY, vis);
        PlaceCtrl(g_hChkUseVarSizeY, rLeft + 153, startY + 109, 38, 20); SetCtrlVisible(g_hChkUseVarSizeY, vis);
        PlaceCtrl(g_hEditVarSizeYName, rLeft + 193, startY + 111, 95, 17); SetCtrlVisible(g_hEditVarSizeYName, vis);
        PlaceCtrl(g_hLblAngle, rLeft + 1, startY + 131, 103, 21); SetCtrlVisible(g_hLblAngle, vis);
        PlaceCtrl(g_hEditAngle, rLeft + 108, startY + 133, 42, 17); SetCtrlVisible(g_hEditAngle, vis);
        PlaceCtrl(g_hChkUseVarAngle, rLeft + 153, startY + 131, 38, 20); SetCtrlVisible(g_hChkUseVarAngle, vis);
        PlaceCtrl(g_hEditVarAngleName, rLeft + 193, startY + 133, 95, 17); SetCtrlVisible(g_hEditVarAngleName, vis);

        int currentY = startY + 153;

        // --- 2) Appearance Category ---
        PlaceCtrl(g_hHeaderAppearance, rLeft + 1, currentY, propW - 2, 19); SetCtrlVisible(g_hHeaderAppearance, vis);
        currentY += 20;

        PlaceCtrl(g_hLblColorName, rLeft + 1, currentY, 103, 21); SetCtrlVisible(g_hLblColorName, vis);
        PlaceCtrl(g_hBtnColorPicker, rLeft + 107, currentY + 1, propW - 109, 19); SetCtrlVisible(g_hBtnColorPicker, vis);
        currentY += 22;

        PlaceCtrl(g_hLblOpacityName, rLeft + 1, currentY, 103, 21); SetCtrlVisible(g_hLblOpacityName, vis);
        PlaceCtrl(g_hEditOpacity, rLeft + 108, currentY + 2, propW - 110, 17); SetCtrlVisible(g_hEditOpacity, vis);
        currentY += 22;

        if (!isText) {
            PlaceCtrl(g_hLblThickName, rLeft + 1, currentY, 103, 21); SetCtrlVisible(g_hLblThickName, vis);
            PlaceCtrl(g_hEditThick, rLeft + 108, currentY + 2, propW - 110, 17); SetCtrlVisible(g_hEditThick, vis);
            SetCtrlVisible(g_hLblThickName, vis);
            SetCtrlVisible(g_hEditThick, vis);
            SetCtrlVisible(g_hLblTextContentName, false);
            SetCtrlVisible(g_hEditTextContent, false);
            SetCtrlVisible(g_hLblTextSizeName, false);
            SetCtrlVisible(g_hEditTextSize, false);
            currentY += 22;
        } else {
            PlaceCtrl(g_hLblTextContentName, rLeft + 1, currentY, 103, 21); SetCtrlVisible(g_hLblTextContentName, vis);
            PlaceCtrl(g_hEditTextContent, rLeft + 108, currentY + 2, propW - 110, 17); SetCtrlVisible(g_hEditTextContent, vis);
            SetCtrlVisible(g_hLblTextContentName, vis);
            SetCtrlVisible(g_hEditTextContent, vis);
            currentY += 22;

            PlaceCtrl(g_hLblTextSizeName, rLeft + 1, currentY, 103, 21); SetCtrlVisible(g_hLblTextSizeName, vis);
            PlaceCtrl(g_hEditTextSize, rLeft + 108, currentY + 2, propW - 110, 17); SetCtrlVisible(g_hEditTextSize, vis);
            SetCtrlVisible(g_hLblTextSizeName, vis);
            SetCtrlVisible(g_hEditTextSize, vis);
            SetCtrlVisible(g_hLblThickName, false);
            SetCtrlVisible(g_hEditThick, false);
            currentY += 22;
        }

        // --- 3) Properties Category ---
        PlaceCtrl(g_hHeaderProperties, rLeft + 1, currentY, propW - 2, 19); SetCtrlVisible(g_hHeaderProperties, vis);
        currentY += 20;

        PlaceCtrl(g_hLblParent, rLeft + 1, currentY, 103, 21); SetCtrlVisible(g_hLblParent, vis);
        PlaceCtrl(g_hComboParent, rLeft + 108, currentY + 2, propW - 110, 120); SetCtrlVisible(g_hComboParent, vis);
        currentY += 24;

        if (isText) {
            PlaceCtrl(g_hChkIsInput, rLeft + 1, currentY, propW - 2, 20); SetCtrlVisible(g_hChkIsInput, vis);
            PlaceCtrl(g_hLblVarName, rLeft + 1, currentY + 22, 103, 21); SetCtrlVisible(g_hLblVarName, vis);
            PlaceCtrl(g_hEditTextVarName, rLeft + 108, currentY + 24, propW - 110, 17); SetCtrlVisible(g_hEditTextVarName, vis);
            PlaceCtrl(g_hLblType, rLeft + 1, currentY + 45, 103, 21); SetCtrlVisible(g_hLblType, vis);
            PlaceCtrl(g_hComboInputType, rLeft + 108, currentY + 47, propW - 110, 120); SetCtrlVisible(g_hComboInputType, vis);

            SetCtrlVisible(g_hChkIsInput, vis);
            SetCtrlVisible(g_hLblVarName, vis);
            SetCtrlVisible(g_hEditTextVarName, vis);
            SetCtrlVisible(g_hLblType, vis);
            SetCtrlVisible(g_hComboInputType, vis);
            currentY += 70;
        } else {
            SetCtrlVisible(g_hChkIsInput, false);
            SetCtrlVisible(g_hLblVarName, false);
            SetCtrlVisible(g_hEditTextVarName, false);
            SetCtrlVisible(g_hLblType, false);
            SetCtrlVisible(g_hComboInputType, false);
        }

        UpdateInputControlsState();

        g_propGridBottom = currentY + 2;   // grid ends at the last row, not reaching the Flip/Center/Clone buttons
        int actionY = currentY + 12;
        PlaceCtrl(g_hBtnFlipX, rLeft, actionY, 110, 24); SetCtrlVisible(g_hBtnFlipX, vis);
        PlaceCtrl(g_hBtnFlipY, rLeft + 115, actionY, 115, 24); SetCtrlVisible(g_hBtnFlipY, vis);
        PlaceCtrl(g_hBtnCenter, rLeft, actionY + 30, propW, 24); SetCtrlVisible(g_hBtnCenter, vis);
        PlaceCtrl(g_hBtnClone, rLeft, actionY + 60, propW, 24); SetCtrlVisible(g_hBtnClone, vis);

        // Attribute panel sits below Clone Shape with enough breathing room.
        int attrTop = actionY + 96;
        int attrBottom = cRect.bottom;
        PlaceCtrl(g_hAttrText, rLeft + 1, attrTop + 25, propW - 2, max(30, attrBottom - attrTop - 27));
        SetCtrlVisible(g_hAttrText, vis);

        PlaceCtrl(g_hBtnCopyScript, 10, 48, 80, 28);
        PlaceCtrl(g_hLblE2Name, 102, 48, 42, 24);
        PlaceCtrl(g_hEditE2Name, 146, 48, 320, 24);
        // Code frame is drawn in WM_PAINT at (10,81)-(winW-10,winH-38); content sits 1px inside
        PlaceCtrl(g_hEditScriptLines, 11, 82, 46, winH - 121);
        PlaceCtrl(g_hEditScriptView, 57, 82, winW - 68, winH - 121);

        // Repaint only what is needed: real resize = whole window, panel relayout only = right panel
        static int lastW = -1, lastH = -1;
        bool resized = (winW != lastW || winH != lastH);
        lastW = winW; lastH = winH;
        if (resized) {
            // On resize: repaint the whole window and all child controls (avoids tool-button ghosting)
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        } else if (g_layoutChanged) {
            RECT panel = { winW - 310, 40, winW, winH };
            InvalidateRect(hwnd, &panel, FALSE);
        }
        g_layoutChanged = false;
        break;
    }

    case WM_ERASEBKGND:
        return 1;   // WM_PAINT draws the background itself (double-buffered); don't let the system erase it first (cause of flicker)

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (id == ID_EDIT_SCRIPT_VIEW && code == EN_VSCROLL) {   // code scrolled -> scroll the gutter too
            SyncScriptGutter();
            break;
        }

        // locked (dim) checkbox: clicking doesn't change the value; flip it back immediately
        if (code == BN_CLICKED && lParam && IsCtrlLocked((HWND)lParam)) {
            HWND hc = (HWND)lParam;
            LRESULT st = SendMessage(hc, BM_GETCHECK, 0, 0);
            SendMessage(hc, BM_SETCHECK, st == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED, 0);
            break;
        }

        if (id == ID_TAB_VISUAL) {
            SwitchViewMode(VIEW_VISUAL);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (id == ID_TAB_SCRIPT) {
            UpdateScriptViewText();
            SwitchViewMode(VIEW_SCRIPT);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (id == ID_BTN_THEME) {
            g_darkTheme = !g_darkTheme;
            ApplyTitleBarTheme(hwnd);
            ApplyThemeToControls(hwnd);
        }

        if (g_currentView == VIEW_VISUAL) {
            if (id == ID_BTN_SELECT)         { g_currentTool = TOOL_SELECT; AddLog(hwnd, "Selected Tool: Select / Move"); }
            if (id == ID_BTN_LINE)           { g_currentTool = TOOL_LINE; AddLog(hwnd, "Selected Tool: Line"); }
            if (id == ID_BTN_RECT)           { g_currentTool = TOOL_RECT; AddLog(hwnd, "Selected Tool: Rect Outline"); }
            if (id == ID_BTN_BOX)            { g_currentTool = TOOL_BOX; AddLog(hwnd, "Selected Tool: Box (Solid)"); }
            if (id == ID_BTN_CIRCLE_OUTLINE) { g_currentTool = TOOL_CIRCLE_OUTLINE; AddLog(hwnd, "Selected Tool: Circle Outline"); }
            if (id == ID_BTN_CIRCLE_SOLID)   { g_currentTool = TOOL_CIRCLE_SOLID; AddLog(hwnd, "Selected Tool: Circle (Solid)"); }
            if (id == ID_BTN_TEXT)           { 
                g_currentTool = TOOL_TEXT; 
                char buf[256];
                GetWindowTextA(g_hEditTextContent, buf, 256);
                g_currentText = buf;
                AddLog(hwnd, "Selected Tool: Text"); 
            }

            if (id == ID_BTN_SHAPES_POPUP) {
                ShowManageShapesPopup(hwnd);
            }
            if (id == ID_BTN_VARS_POPUP) {
                ShowVariableManagePopup(hwnd);
            }

            if (id == ID_COMBO_BG && code == CBN_SELCHANGE) {
                int sel = (int)SendMessage(g_hComboBg, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    g_bgMode = sel;
                    LoadBgImage();
                    RECT rc; GetClientRect(hwnd, &rc);
                    RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                    InvalidateRect(hwnd, &cRect, FALSE);
                    UpdateAttributeText();
                    RECT attrArea = { rc.right - 250, cRect.top, rc.right, cRect.bottom };
                    InvalidateRect(hwnd, &attrArea, FALSE);
                }
            }

            if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                auto& s = g_shapes[g_selectedShapeIndex];

                if (id == ID_BTN_FLIP_X) {
                    swap(s.p1.first, s.p2.first);
                    AddLog(hwnd, "Flipped Shape horizontally.");
                }
                else if (id == ID_BTN_FLIP_Y) {
                    swap(s.p1.second, s.p2.second);
                    AddLog(hwnd, "Flipped Shape vertically.");
                }
                else if (id == ID_BTN_CENTER) {
                    // Move the whole shape (p1 and p2 together) so its bounds center is at the canvas center
                    // (moving only p1 would change a circle's radius)
                    RECT rcC; GetClientRect(hwnd, &rcC);
                    RECT cvR = GetCanvasRect(rcC.right, rcC.bottom);
                    double bl, bt, br, bb;
                    if (GetShapeBoundsCanvas(g_selectedShapeIndex, cvR, bl, bt, br, bb)) {
                        double mvx = 960.0 - (bl + br) / 2.0;
                        double mvy = 540.0 - (bt + bb) / 2.0;
                        s.p1.first += mvx; s.p1.second += mvy;
                        s.p2.first += mvx; s.p2.second += mvy;
                    }
                    SetTextIfChanged(g_hEditPosX, to_string((int)s.p1.first).c_str());
                    SetTextIfChanged(g_hEditPosY, to_string((int)s.p1.second).c_str());
                SetTextIfChanged(g_hEditAngle, to_string((int)std::round(s.angle)).c_str());
                double uiSizeW = std::abs(s.p2.first - s.p1.first);
                double uiSizeH = std::abs(s.p2.second - s.p1.second);
                if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) { double rr = std::sqrt((s.p2.first-s.p1.first)*(s.p2.first-s.p1.first) + (s.p2.second-s.p1.second)*(s.p2.second-s.p1.second)); uiSizeW = uiSizeH = rr; }
                SetTextIfChanged(g_hEditSizeX, to_string((int)std::round(uiSizeW)).c_str());
                SetTextIfChanged(g_hEditSizeY, to_string((int)std::round(uiSizeH)).c_str());
                    AddLog(hwnd, "Centered Shape on Canvas.");
                }
                else if (id == ID_BTN_CLONE) {
                    DrawShape clone = s;
                    clone.p1.first += 20; clone.p1.second += 20;
                    clone.p2.first += 20; clone.p2.second += 20;
                    
                    if (clone.type == TOOL_LINE) clone.name = "line" + to_string(++g_lineCount);
                    else if (clone.type == TOOL_RECT) clone.name = "rect" + to_string(++g_rectCount);
                    else if (clone.type == TOOL_BOX) clone.name = "box" + to_string(++g_boxCount);
                    else if (clone.type == TOOL_CIRCLE_OUTLINE) clone.name = "circle_outline" + to_string(++g_circleOutlineCount);
                    else if (clone.type == TOOL_CIRCLE_SOLID) clone.name = "circle_solid" + to_string(++g_circleSolidCount);
                    else if (clone.type == TOOL_TEXT) clone.name = "text" + to_string(++g_textCount);

                    g_shapes.push_back(clone);
                    g_selectedShapeIndex = (int)g_shapes.size() - 1;
                    UpdateParentComboBox();
                    AddLog(hwnd, "Cloned selected Shape.");
                }
            }

            UpdateToolButtonHighlights();

            RECT rc; GetClientRect(hwnd, &rc);

            if (id == ID_BTN_COLOR_PICKER) {
                CHOOSECOLOR cc = { sizeof(CHOOSECOLOR) };
                cc.hwndOwner = hwnd;
                cc.lpCustColors = g_custColors;
                cc.rgbResult = g_currentColor;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                if (ChooseColor(&cc)) {
                    g_currentColor = cc.rgbResult;
                    if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                        g_shapes[g_selectedShapeIndex].color = g_currentColor;
                    }
                    AddLog(hwnd, "Color updated.");
                    InvalidateRect(g_hBtnColorPicker, NULL, TRUE);
                    RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                    InvalidateRect(hwnd, &cRect, FALSE);
                }
            }
            else if (id == ID_EDIT_POS_X && code == EN_CHANGE) {
                char buf[16];
                GetWindowTextA(g_hEditPosX, buf, 16);
                g_currentPosX = atof(buf);
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    auto& s = g_shapes[g_selectedShapeIndex];
                    double width = s.p2.first - s.p1.first;
                    s.p1.first = g_currentPosX;
                    s.p2.first = s.p1.first + width;
                    RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                    InvalidateRect(hwnd, &cRect, FALSE);
                }
            }
            else if (id == ID_EDIT_POS_Y && code == EN_CHANGE) {
                char buf[16];
                GetWindowTextA(g_hEditPosY, buf, 16);
                g_currentPosY = atof(buf);
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    auto& s = g_shapes[g_selectedShapeIndex];
                    double height = s.p2.second - s.p1.second;
                    s.p1.second = g_currentPosY;
                    s.p2.second = s.p1.second + height;
                    RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                    InvalidateRect(hwnd, &cRect, FALSE);
                }
            }
            else if (id == ID_EDIT_SIZE_X && code == EN_CHANGE) {
                char buf[32]; GetWindowTextA(g_hEditSizeX, buf, 32);
                double v = atof(buf);
                if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size() && v > 0) {
                    auto& s = g_shapes[g_selectedShapeIndex];
                    if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) {
                        double r = v;   // circle Size = radius (matches the value E2 takes)
                        double olddx = s.p2.first - s.p1.first, oldy = s.p2.second - s.p1.second;
                        double len = sqrt(olddx*olddx + oldy*oldy); if (len < 0.001) len = 1;
                        s.p2.first = s.p1.first + olddx / len * r; s.p2.second = s.p1.second + oldy / len * r;
                    } else { s.p2.first = s.p1.first + (s.p2.first >= s.p1.first ? v : -v); }
                    InvalidateRect(hwnd, NULL, FALSE); UpdateAttributeText();
                }
            }
            else if (id == ID_EDIT_SIZE_Y && code == EN_CHANGE) {
                char buf[32]; GetWindowTextA(g_hEditSizeY, buf, 32);
                double v = atof(buf);
                if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size() && v > 0) {
                    auto& s = g_shapes[g_selectedShapeIndex];
                    if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) {
                        double r = v; double dx=s.p2.first-s.p1.first, dy=s.p2.second-s.p1.second; double len=sqrt(dx*dx+dy*dy); if(len<0.001)len=1;
                        s.p2.first=s.p1.first+dx/len*r; s.p2.second=s.p1.second+dy/len*r;
                    } else { s.p2.second = s.p1.second + (s.p2.second >= s.p1.second ? v : -v); }
                    InvalidateRect(hwnd, NULL, FALSE); UpdateAttributeText();
                }
            }
            else if (id == ID_CHK_USE_VAR_SIZE_X) {
                g_currentUseVarSizeX = (SendMessage(g_hChkUseVarSizeX, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size()) g_shapes[g_selectedShapeIndex].useVarSizeX = g_currentUseVarSizeX;
                UpdateInputControlsState(); InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (id == ID_CHK_USE_VAR_SIZE_Y) {
                g_currentUseVarSizeY = (SendMessage(g_hChkUseVarSizeY, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size()) g_shapes[g_selectedShapeIndex].useVarSizeY = g_currentUseVarSizeY;
                UpdateInputControlsState(); InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (id == ID_CHK_USE_VAR_ANGLE) {
                g_currentUseVarAngle = (SendMessage(g_hChkUseVarAngle, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size()) g_shapes[g_selectedShapeIndex].useVarAngle = g_currentUseVarAngle;
                UpdateInputControlsState(); InvalidateRect(hwnd, NULL, FALSE);
            }
            else if ((id == ID_EDIT_VAR_SIZE_X_NAME || id == ID_EDIT_VAR_SIZE_Y_NAME || id == ID_EDIT_VAR_ANGLE_NAME) && code == EN_CHANGE) {
                char buf[64] = {0}; GetWindowTextA((HWND)lParam, buf, sizeof(buf));
                if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    auto& s = g_shapes[g_selectedShapeIndex];
                    if (id == ID_EDIT_VAR_SIZE_X_NAME) { g_currentVarSizeXName = buf; s.varSizeXName = buf; }
                    else if (id == ID_EDIT_VAR_SIZE_Y_NAME) { g_currentVarSizeYName = buf; s.varSizeYName = buf; }
                    else { g_currentVarAngleName = buf; s.varAngleName = buf; }
                    UpdateScriptViewText();
                }
            }
            else if (id == ID_EDIT_ANGLE && code == EN_CHANGE) {
                char buf[32]; GetWindowTextA(g_hEditAngle, buf, 32);
                double v = atof(buf);
                if (g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].angle = v; g_currentAngle = v;
                    InvalidateRect(hwnd, NULL, FALSE); UpdateAttributeText();
                }
            }
            else if (id == ID_CHK_USE_VAR_X) {
                g_currentUseVarX = (SendMessage(g_hChkUseVarX, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].useVarX = g_currentUseVarX;
                }
                UpdateInputControlsState();
                InvalidateRect(g_hChkUseVarX, NULL, TRUE);
            }
            else if (id == ID_CHK_USE_VAR_Y) {
                g_currentUseVarY = (SendMessage(g_hChkUseVarY, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].useVarY = g_currentUseVarY;
                }
                UpdateInputControlsState();
                InvalidateRect(g_hChkUseVarY, NULL, TRUE);
            }
            else if (id == ID_EDIT_VAR_X_NAME && code == EN_CHANGE) {
                char buf[64];
                GetWindowTextA(g_hEditVarXName, buf, 64);
                g_currentVarXName = buf;
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].varXName = g_currentVarXName;
                }
            }
            else if (id == ID_EDIT_VAR_Y_NAME && code == EN_CHANGE) {
                char buf[64];
                GetWindowTextA(g_hEditVarYName, buf, 64);
                g_currentVarYName = buf;
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].varYName = g_currentVarYName;
                }
            }
            else if (id == ID_CHK_IS_EGP3D) {
                g_currentIsEgp3D = (SendMessage(g_hChkIsEgp3D, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].isEgp3D = g_currentIsEgp3D;
                }
                SetCtrlVisible(g_hEditVar3DName, g_currentIsEgp3D && g_currentView == VIEW_VISUAL);
                UpdateInputControlsState();
                InvalidateRect(g_hChkIsEgp3D, NULL, TRUE);
                RECT client; GetClientRect(hwnd, &client);
                SendMessage(hwnd, WM_SIZE, 0, MAKELONG(client.right, client.bottom));
                InvalidateRect(hwnd, NULL, FALSE);
            }
            else if (id == ID_EDIT_VAR_3D_NAME && code == EN_CHANGE) {
                char buf[128];
                GetWindowTextA(g_hEditVar3DName, buf, 128);
                g_currentVar3DName = buf;
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].var3DName = g_currentVar3DName;
                }
            }
            else if (id == ID_CHK_SHOW_GRID) {
                g_showGrid = (SendMessage(g_hChkShowGrid, BM_GETCHECK, 0, 0) == BST_CHECKED);
                InvalidateRect(g_hChkShowGrid, NULL, TRUE);
                RECT gridRc; GetClientRect(hwnd, &gridRc);
                RECT gridCanvas = GetCanvasRect(gridRc.right, gridRc.bottom);
                InvalidateRect(hwnd, &gridCanvas, FALSE);
                AddLog(hwnd, g_showGrid ? "Grid: ON" : "Grid: OFF (center axes kept)");
            }
            else if (id == ID_CHK_USE_INTERVAL) {
                g_useInterval = (SendMessage(g_hChkUseInterval, BM_GETCHECK, 0, 0) == BST_CHECKED);
                UpdateInputControlsState();
                InvalidateRect(g_hChkUseInterval, NULL, TRUE);
            }
            else if (id == ID_EDIT_INTERVAL_MS && code == EN_CHANGE) {
                char buf[16];
                GetWindowTextA(g_hEditIntervalMs, buf, 16);
                int val = atoi(buf);
                if (val > 0) {
                    g_intervalMs = val;
                }
            }
            else if (id == ID_COMBO_PARENT && code == CBN_SELCHANGE) {
                int sel = (int)SendMessage(g_hComboParent, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR && g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    auto& child = g_shapes[g_selectedShapeIndex];

                    Point oldAbsPos = GetAbsolutePosition(g_selectedShapeIndex);

                    if (sel == 0) {
                        g_currentParentIndex = -1;
                    } else {
                        int targetIdx = -1;
                        int currentCount = 0;
                        for (size_t i = 0; i < g_shapes.size(); i++) {
                            if ((int)i == g_selectedShapeIndex) continue;
                            currentCount++;
                            if (currentCount == sel) {
                                targetIdx = (int)i;
                                break;
                            }
                        }
                        g_currentParentIndex = targetIdx;
                    }

                    child.parentIndex = g_currentParentIndex;

                    Point newParentAbsPos = GetAbsolutePosition(child.parentIndex);
                    if (child.parentIndex == -1) newParentAbsPos = { 0.0, 0.0 };

                    double width = child.p2.first - child.p1.first;
                    double height = child.p2.second - child.p1.second;

                    child.p1.first = oldAbsPos.first - newParentAbsPos.first;
                    child.p1.second = oldAbsPos.second - newParentAbsPos.second;
                    child.p2.first = child.p1.first + width;
                    child.p2.second = child.p1.second + height;

                    SetTextIfChanged(g_hEditPosX, to_string((int)child.p1.first).c_str());
                    SetTextIfChanged(g_hEditPosY, to_string((int)child.p1.second).c_str());

                    RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                    InvalidateRect(hwnd, &cRect, FALSE);
                    UpdateAttributeText();
                    RECT attrArea = { rc.right - 250, cRect.top, rc.right, cRect.bottom };
                    InvalidateRect(hwnd, &attrArea, FALSE);
                }
            }
            else if (id == ID_EDIT_OPACITY && code == EN_CHANGE) {
                char buf[10];
                GetWindowTextA(g_hEditOpacity, buf, 10);
                int val = atoi(buf);
                if (val >= 0 && val <= 255) {
                    if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                        g_shapes[g_selectedShapeIndex].opacity = val;
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                }
            }
            else if (id == ID_EDIT_THICKNESS && code == EN_CHANGE) {
                char buf[10];
                GetWindowTextA(g_hEditThick, buf, 10);
                int val = atoi(buf);
                if (val >= 1 && val <= 50) {
                    g_currentThickness = val;
                    if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                        g_shapes[g_selectedShapeIndex].thickness = g_currentThickness;
                        RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                        InvalidateRect(hwnd, &cRect, FALSE);
                    }
                }
            }
            else if (id == ID_EDIT_TEXT_CONTENT && code == EN_CHANGE) {
                char buf[256];
                GetWindowTextA(g_hEditTextContent, buf, 256);
                g_currentText = buf;
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].textContent = g_currentText;
                    RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                    InvalidateRect(hwnd, &cRect, FALSE);
                }
            }
            else if (id == ID_EDIT_TEXT_SIZE && code == EN_CHANGE) {
                char buf[10];
                GetWindowTextA(g_hEditTextSize, buf, 10);
                int val = atoi(buf);
                if (val >= 1 && val <= 200) {
                    g_currentTextSize = val;
                    if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                        g_shapes[g_selectedShapeIndex].textSize = g_currentTextSize;
                        RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                        InvalidateRect(hwnd, &cRect, FALSE);
                    }
                }
            }
            else if (id == ID_EDIT_TEXT_VARNAME && code == EN_CHANGE) {
                char buf[256];
                GetWindowTextA(g_hEditTextVarName, buf, 256);
                g_currentVarName = buf;
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].varName = g_currentVarName;
                }
                RefreshVariablePopup();
            }
            else if (id == ID_COMBO_INPUT_TYPE && code == CBN_SELCHANGE) {
                int sel = (int)SendMessage(g_hComboInputType, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    g_currentInputType = (InputType)sel;
                    if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                        g_shapes[g_selectedShapeIndex].inputType = g_currentInputType;
                    }
                    RefreshVariablePopup();
                }
            }
            else if (id == ID_CHK_IS_INPUT) {
                g_currentIsInput = (SendMessage(g_hChkIsInput, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (g_selectedShapeIndex != -1 && g_selectedShapeIndex < (int)g_shapes.size()) {
                    g_shapes[g_selectedShapeIndex].isInput = g_currentIsInput;
                }
                
                UpdateInputControlsState();
                RefreshVariablePopup();

                RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                InvalidateRect(hwnd, &cRect, FALSE);
            }
            else if (id == ID_BTN_EXPORT) {
                HudExporter::export_to_file("output.txt", g_shapes, g_variables, g_useInterval, g_intervalMs, g_e2Name);
                AddLog(hwnd, "Exported E2 code to output.txt successfully.");
            }
            else if (id == ID_BTN_CLEAR) {
                g_shapes.clear();
                g_variables.clear();
                g_selectedShapeIndex = -1;
                g_lineCount = 0;
                g_rectCount = 0;
                g_boxCount = 0;
                g_circleOutlineCount = 0;
                g_circleSolidCount = 0;
                g_textCount = 0;
                UpdateParentComboBox();
                UpdateAttributeText();
                if (g_hPopupDlg && g_hPopupList) PopulateShapesList(g_hPopupList);
                AddLog(hwnd, "Cleared all shapes from canvas.");
                RECT cRect = GetCanvasRect(rc.right, rc.bottom);
                InvalidateRect(hwnd, &cRect, FALSE);
            }

            if (id >= ID_BTN_SELECT && id <= ID_BTN_CLEAR) {
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                SendMessage(hwnd, WM_SIZE, 0, MAKELONG(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top));
            }
        }
        else if (g_currentView == VIEW_SCRIPT) {
            if (id == ID_EDIT_E2NAME && code == EN_CHANGE) {
                char nameBuf[256] = {0};
                GetWindowTextA(g_hEditE2Name, nameBuf, sizeof(nameBuf));
                g_e2Name = nameBuf;
                UpdateScriptViewText();
            }
            else if (id == ID_BTN_COPY_SCRIPT) {
                string codeStr = HudExporter::generate_e2_code(g_shapes, g_variables, g_useInterval, g_intervalMs, g_e2Name);
                CopyTextToClipboard(hwnd, codeStr);
                AddLog(hwnd, "Copied E2 script to clipboard.");
            }
        }
        break;
    }

    case WM_MOUSEMOVE: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        RECT rc; GetClientRect(hwnd, &rc);
        RECT cRect = GetCanvasRect(rc.right, rc.bottom);
        POINT oldMouse = g_mousePos;

        if (pt.x >= cRect.left && pt.x <= cRect.right && pt.y >= cRect.top && pt.y <= cRect.bottom) {
            Point cPt = ScreenToCanvas(pt, cRect);
            g_mousePos.x = (long)cPt.first;
            g_mousePos.y = (long)cPt.second;
        }

        if (g_currentView == VIEW_VISUAL) {
            if (g_isResizingShape && g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size() &&
                g_shapes[g_selectedShapeIndex].type == TOOL_TEXT) {
                // Dragging a text corner changes Text Size (E2 egpText has font size, no width)
                auto& s = g_shapes[g_selectedShapeIndex];
                Point aP = GetAbsolutePosition(g_selectedShapeIndex);
                POINT origin = CanvasToScreen(TransformCanvasPoint(g_selectedShapeIndex, aP), cRect);
                const double rad = -TotalShapeAngle(g_selectedShapeIndex) * 3.14159265358979323846 / 180.0;
                double mx = pt.x - origin.x, my = pt.y - origin.y;
                double lx = mx * std::cos(rad) - my * std::sin(rad) - 3.0;   // undo rotation + subtract 3px margin
                double ly = mx * std::sin(rad) + my * std::cos(rad) - 3.0;
                SIZE ts = MeasureTextShape(s, s.textSize);
                double k = (ts.cx > 0 && s.textSize > 0) ? (double)ts.cx / s.textSize : 0.5;   // width/height ratio
                // project mouse position onto the box diagonal (k, 1) -> new size
                int ns = (int)std::lround((lx * k + ly) / (k * k + 1.0));
                ns = std::max(1, std::min(200, ns));
                if (ns != s.textSize) {
                    s.textSize = ns;
                    g_currentTextSize = ns;
                    SetTextIfChanged(g_hEditTextSize, to_string(ns).c_str());
                }
                UpdateAttributeText(); InvalidateRect(hwnd, &cRect, FALSE);
            }
            else if (g_isResizingShape && g_selectedShapeIndex >= 0 && g_selectedShapeIndex < (int)g_shapes.size()) {
                Point cp = ScreenToCanvas(pt, cRect); auto& s = g_shapes[g_selectedShapeIndex];
                Point absP = GetAbsolutePosition(g_selectedShapeIndex);
                // absP is the shape's absolute origin. Convert the mouse back to the
                // shape's local coordinates by adding the local p1 offset back through
                // the parent origin. The old code subtracted absP and then compared that
                // delta against s.p1, effectively subtracting p1 twice and pulling the
                // opposite corner left while resizing.
                Point parentOrigin = { absP.first - s.p1.first, absP.second - s.p1.second };
                double nx = cp.first - parentOrigin.first;
                double ny = cp.second - parentOrigin.second;
                double w = std::max(5.0, std::abs(nx - s.p1.first));
                double h = std::max(5.0, std::abs(ny - s.p1.second));
                if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) {
                    double dx = cp.first - absP.first, dy = cp.second - absP.second;
                    double r = std::max(2.5, std::sqrt(dx*dx + dy*dy));
                    s.p2.first = s.p1.first + r; s.p2.second = s.p1.second;
                    w = h = r;
                } else {
                    s.p2.first = nx;
                    s.p2.second = ny;
                }
                SetTextIfChanged(g_hEditSizeX, to_string((int)std::round(w)).c_str());
                SetTextIfChanged(g_hEditSizeY, to_string((int)std::round(h)).c_str());
                UpdateAttributeText(); InvalidateRect(hwnd, &cRect, FALSE);
            }
            else if (g_isDraggingShape && g_selectedShapeIndex != -1) {
                Point currentCanvasPt = ScreenToCanvas(pt, cRect);
                auto& s = g_shapes[g_selectedShapeIndex];
                
                double dx = currentCanvasPt.first - g_dragOffset.first;
                double dy = currentCanvasPt.second - g_dragOffset.second;
                g_dragOffset = currentCanvasPt;

                // Accumulate the raw mouse position, then apply snap on top (snapping the real position would make it impossible to leave a snap point)
                g_dragRawP1.first += dx;
                g_dragRawP1.second += dy;
                double adjX = 0.0, adjY = 0.0;
                if (!(GetKeyState(VK_SHIFT) & 0x8000)) {   // hold Shift = no snap
                    ComputeMoveSnap(g_selectedShapeIndex, cRect, g_dragRawP1.first - s.p1.first, g_dragRawP1.second - s.p1.second, adjX, adjY);
                }
                double mvx = g_dragRawP1.first + adjX - s.p1.first;
                double mvy = g_dragRawP1.second + adjY - s.p1.second;
                s.p1.first += mvx;  s.p1.second += mvy;
                s.p2.first += mvx;  s.p2.second += mvy;

                SetTextIfChanged(g_hEditPosX, to_string((int)s.p1.first).c_str());
                SetTextIfChanged(g_hEditPosY, to_string((int)s.p1.second).c_str());
                SetTextIfChanged(g_hEditAngle, to_string((int)std::round(s.angle)).c_str());
                double uiSizeW = std::abs(s.p2.first - s.p1.first);
                double uiSizeH = std::abs(s.p2.second - s.p1.second);
                if (s.type == TOOL_CIRCLE_OUTLINE || s.type == TOOL_CIRCLE_SOLID) { double rr = std::sqrt((s.p2.first-s.p1.first)*(s.p2.first-s.p1.first) + (s.p2.second-s.p1.second)*(s.p2.second-s.p1.second)); uiSizeW = uiSizeH = rr; }
                SetTextIfChanged(g_hEditSizeX, to_string((int)std::round(uiSizeW)).c_str());
                SetTextIfChanged(g_hEditSizeY, to_string((int)std::round(uiSizeH)).c_str());
                UpdateAttributeText();
            }

            if (g_isDrawing) {
                g_endPt = (g_currentTool == TOOL_LINE && (GetKeyState(VK_SHIFT) & 0x8000))
                    ? SnapLineEnd45(g_startPt, pt) : pt;
            }

            if (g_isDrawing || g_isDraggingShape) {
                RECT redrawArea = { cRect.left, cRect.top, cRect.right, cRect.bottom + 50 };
                InvalidateRect(hwnd, &redrawArea, FALSE);
                if (g_isDraggingShape) {
                    RECT attributeArea = { rc.right - 250, cRect.top, rc.right, cRect.bottom };
                    InvalidateRect(hwnd, &attributeArea, FALSE);
                }
            } else if (g_mousePos.x != oldMouse.x || g_mousePos.y != oldMouse.y) {
                // Plain mouse move: update only the cursor bar, don't repaint the whole canvas
                RECT bar = { cRect.left, cRect.bottom, cRect.right, cRect.bottom + 24 };
                InvalidateRect(hwnd, &bar, FALSE);
            }
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        if (g_currentView != VIEW_VISUAL) break;

        SetFocus(hwnd);

        RECT rc; GetClientRect(hwnd, &rc);
        RECT canvasRect = GetCanvasRect(rc.right, rc.bottom);

        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        if (pt.x >= canvasRect.left && pt.x <= canvasRect.right &&
            pt.y >= canvasRect.top && pt.y <= canvasRect.bottom) {
            
            if (g_currentTool == TOOL_SELECT) {
                if (PointInSelectedResizeHandle(hwnd, pt)) {
                    g_isResizingShape = true; g_isDraggingShape = false; break;
                }
                g_selectedShapeIndex = -1;
                HDC hdc = GetDC(hwnd);

                for (int i = (int)g_shapes.size() - 1; i >= 0; i--) {
                    Point absP1 = GetAbsolutePosition(i);
                    Point absP2 = { absP1.first + (g_shapes[i].p2.first - g_shapes[i].p1.first), absP1.second + (g_shapes[i].p2.second - g_shapes[i].p1.second) };

                    POINT s1 = CanvasToScreen(absP1, canvasRect);
                    POINT s2 = CanvasToScreen(absP2, canvasRect);
                    
                    int minX, maxX, minY, maxY;
                    
                    if (g_shapes[i].type == TOOL_CIRCLE_OUTLINE || g_shapes[i].type == TOOL_CIRCLE_SOLID) {
                        int rad = (int)sqrt(pow(s2.x - s1.x, 2) + pow(s2.y - s1.y, 2));
                        minX = s1.x - rad;
                        maxX = s1.x + rad;
                        minY = s1.y - rad;
                        maxY = s1.y + rad;
                    } else if (g_shapes[i].type == TOOL_TEXT) {
                        HFONT hFont = CreateFontA(g_shapes[i].textSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
                        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
                        SIZE textSize;
                        GetTextExtentPoint32A(hdc, g_shapes[i].textContent.c_str(), g_shapes[i].textContent.size(), &textSize);
                        SelectObject(hdc, oldFont);
                        DeleteObject(hFont);

                        minX = s1.x; 
                        maxX = s1.x + textSize.cx;
                        minY = s1.y;
                        maxY = s1.y + g_shapes[i].textSize;
                    } else {
                        minX = min(s1.x, s2.x); maxX = max(s1.x, s2.x);
                        minY = min(s1.y, s2.y); maxY = max(s1.y, s2.y);
                    }

                    if (pt.x >= minX - 5 && pt.x <= maxX + 5 && pt.y >= minY - 5 && pt.y <= maxY + 5) {
                        g_selectedShapeIndex = i;
                        g_isDraggingShape = true;
                        g_dragOffset = ScreenToCanvas(pt, canvasRect);
                        g_dragRawP1 = g_shapes[i].p1;

                        g_currentColor = g_shapes[i].color;
                        g_currentThickness = g_shapes[i].thickness;
                        g_currentText = g_shapes[i].textContent;
                        g_currentTextSize = g_shapes[i].textSize;
                        g_currentIsInput = g_shapes[i].isInput;
                        g_currentVarName = g_shapes[i].varName;
                        g_currentInputType = g_shapes[i].inputType;

                        g_currentPosX = g_shapes[i].p1.first;
                        g_currentPosY = g_shapes[i].p1.second;
                        g_currentAngle = g_shapes[i].angle;
                        g_currentParentIndex = g_shapes[i].parentIndex;
                        g_currentUseVarX = g_shapes[i].useVarX;
                        g_currentUseVarY = g_shapes[i].useVarY;
                        g_currentUseVarSizeX = g_shapes[i].useVarSizeX;
                        g_currentUseVarSizeY = g_shapes[i].useVarSizeY;
                        g_currentUseVarAngle = g_shapes[i].useVarAngle;
                        g_currentVarSizeXName = g_shapes[i].varSizeXName.empty() ? "SizeX" : g_shapes[i].varSizeXName;
                        g_currentVarSizeYName = g_shapes[i].varSizeYName.empty() ? "SizeY" : g_shapes[i].varSizeYName;
                        g_currentVarAngleName = g_shapes[i].varAngleName.empty() ? "Angle" : g_shapes[i].varAngleName;
                        g_currentVarXName = g_shapes[i].varXName;
                        g_currentVarYName = g_shapes[i].varYName;
                        g_currentIsEgp3D = g_shapes[i].isEgp3D;
                        g_currentVar3DName = g_shapes[i].var3DName.empty() ? "Pos" : g_shapes[i].var3DName;

                        SetTextIfChanged(g_hEditOpacity, to_string(g_shapes[i].opacity).c_str());
                        SetTextIfChanged(g_hEditThick, to_string(g_currentThickness).c_str());
                        SetTextIfChanged(g_hEditTextContent, g_currentText.c_str());
                        SetTextIfChanged(g_hEditTextSize, to_string(g_currentTextSize).c_str());
                        SetTextIfChanged(g_hEditTextVarName, g_currentVarName.c_str());
                        SendMessage(g_hChkIsInput, BM_SETCHECK, g_currentIsInput ? BST_CHECKED : BST_UNCHECKED, 0);
                        SendMessage(g_hComboInputType, CB_SETCURSEL, (WPARAM)g_currentInputType, 0);

                        SetTextIfChanged(g_hEditPosX, to_string((int)g_currentPosX).c_str());
                        SetTextIfChanged(g_hEditPosY, to_string((int)g_currentPosY).c_str());
                        SetTextIfChanged(g_hEditAngle, to_string((int)std::round(g_currentAngle)).c_str());
                        double uiSizeW = std::abs(g_shapes[i].p2.first - g_shapes[i].p1.first);
                        double uiSizeH = std::abs(g_shapes[i].p2.second - g_shapes[i].p1.second);
                        if (g_shapes[i].type == TOOL_CIRCLE_OUTLINE || g_shapes[i].type == TOOL_CIRCLE_SOLID) { double rr = std::sqrt((g_shapes[i].p2.first-g_shapes[i].p1.first)*(g_shapes[i].p2.first-g_shapes[i].p1.first) + (g_shapes[i].p2.second-g_shapes[i].p1.second)*(g_shapes[i].p2.second-g_shapes[i].p1.second)); uiSizeW = uiSizeH = rr; }
                        SetTextIfChanged(g_hEditSizeX, to_string((int)std::round(uiSizeW)).c_str());
                        SetTextIfChanged(g_hEditSizeY, to_string((int)std::round(uiSizeH)).c_str());
                        SendMessage(g_hChkUseVarX, BM_SETCHECK, g_currentUseVarX ? BST_CHECKED : BST_UNCHECKED, 0);
                        SendMessage(g_hChkUseVarY, BM_SETCHECK, g_currentUseVarY ? BST_CHECKED : BST_UNCHECKED, 0);
                        SendMessage(g_hChkUseVarSizeX, BM_SETCHECK, g_currentUseVarSizeX ? BST_CHECKED : BST_UNCHECKED, 0);
                        SendMessage(g_hChkUseVarSizeY, BM_SETCHECK, g_currentUseVarSizeY ? BST_CHECKED : BST_UNCHECKED, 0);
                        SendMessage(g_hChkUseVarAngle, BM_SETCHECK, g_currentUseVarAngle ? BST_CHECKED : BST_UNCHECKED, 0);
                        SetTextIfChanged(g_hEditVarXName, g_currentVarXName.c_str());
                        SetTextIfChanged(g_hEditVarYName, g_currentVarYName.c_str());
                        SetTextIfChanged(g_hEditVarSizeXName, g_currentVarSizeXName.c_str());
                        SetTextIfChanged(g_hEditVarSizeYName, g_currentVarSizeYName.c_str());
                        SetTextIfChanged(g_hEditVarAngleName, g_currentVarAngleName.c_str());
                        SetTextIfChanged(g_hEditVar3DName, g_currentVar3DName.c_str());
                        SendMessage(g_hChkIsEgp3D, BM_SETCHECK, g_currentIsEgp3D ? BST_CHECKED : BST_UNCHECKED, 0);

                        UpdateParentComboBox();
                        UpdateAttributeText();
                        UpdateInputControlsState();

                        AddLog(hwnd, "Selected Shape: " + g_shapes[i].name);
                        InvalidateRect(g_hBtnColorPicker, NULL, TRUE);
                        break;
                    }
                }
                ReleaseDC(hwnd, hdc);

                if (g_selectedShapeIndex == -1) {
                    g_isDraggingShape = false;
                }

                SendMessage(hwnd, WM_SIZE, 0, MAKELONG(rc.right - rc.left, rc.bottom - rc.top));
                InvalidateRect(hwnd, &canvasRect, FALSE);
                RECT attributeArea = { rc.right - 250, canvasRect.top, rc.right, canvasRect.bottom };
                InvalidateRect(hwnd, &attributeArea, FALSE);
            } else if (g_currentTool != TOOL_NONE) {
                g_isDrawing = true;
                g_startPt = pt;
                g_endPt = pt;
            }
        }
        break;
    }

    case WM_LBUTTONUP:
        if (g_currentView == VIEW_VISUAL) {
            if (g_isDraggingShape) {
                g_isDraggingShape = false;
            }
            if (g_isResizingShape) {
                g_isResizingShape = false;
                UpdateAttributeText();
            }
            if (g_isDrawing) {
                g_isDrawing = false;
                RECT rc; GetClientRect(hwnd, &rc);
                RECT canvasRect = GetCanvasRect(rc.right, rc.bottom);

                Point p1 = ScreenToCanvas(g_startPt, canvasRect);
                Point p2 = ScreenToCanvas(g_endPt, canvasRect);

                DrawShape shape(g_currentTool, p1, p2, g_currentColor, g_currentThickness, "", false);

                if (g_currentTool == TOOL_LINE) shape.name = "line" + to_string(++g_lineCount);
                else if (g_currentTool == TOOL_RECT) shape.name = "rect" + to_string(++g_rectCount);
                else if (g_currentTool == TOOL_BOX) shape.name = "box" + to_string(++g_boxCount);
                else if (g_currentTool == TOOL_CIRCLE_OUTLINE) shape.name = "circle_outline" + to_string(++g_circleOutlineCount);
                else if (g_currentTool == TOOL_CIRCLE_SOLID) shape.name = "circle_solid" + to_string(++g_circleSolidCount);
                else if (g_currentTool == TOOL_TEXT) shape.name = "text" + to_string(++g_textCount);

                if (g_currentTool == TOOL_TEXT) {
                    char bufText[256], bufVar[256], bufSize[10];
                    GetWindowTextA(g_hEditTextContent, bufText, 256);
                    GetWindowTextA(g_hEditTextVarName, bufVar, 256);
                    GetWindowTextA(g_hEditTextSize, bufSize, 10);
                    
                    string textToPlace = bufText;
                    if (textToPlace.empty()) textToPlace = "Text";

                    string varToPlace = bufVar;
                    if (varToPlace.empty()) varToPlace = "Var1";

                    int sizeVal = atoi(bufSize);
                    if (sizeVal <= 0) sizeVal = 24;

                    shape.p2 = std::make_pair(p1.first + 100.0, p1.second + 30.0);
                    shape.textContent = textToPlace;
                    shape.textSize = sizeVal;
                    shape.isInput = g_currentIsInput;
                    shape.varName = varToPlace;
                    shape.inputType = g_currentInputType;
                }

                if (g_currentParentIndex >= 0 && g_currentParentIndex < (int)g_shapes.size()) {
                    Point parentAbsPos = GetAbsolutePosition(g_currentParentIndex);
                    double w = shape.p2.first - shape.p1.first;
                    double h = shape.p2.second - shape.p1.second;
                    shape.p1.first -= parentAbsPos.first;
                    shape.p1.second -= parentAbsPos.second;
                    shape.p2.first = shape.p1.first + w;
                    shape.p2.second = shape.p1.second + h;
                    shape.parentIndex = g_currentParentIndex;
                }
                
                g_shapes.push_back(shape);
                g_selectedShapeIndex = (int)g_shapes.size() - 1;
                
                UpdateParentComboBox();
                UpdateAttributeText();
                if (g_hPopupDlg && g_hPopupList) PopulateShapesList(g_hPopupList);
                AddLog(hwnd, "Placed new shape on canvas.");
                RECT redrawArea = { canvasRect.left, canvasRect.top, canvasRect.right, canvasRect.bottom + 50 };
                InvalidateRect(hwnd, &redrawArea, FALSE);
            }
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdcReal = BeginPaint(hwnd, &ps);

        RECT rc; GetClientRect(hwnd, &rc);
        int winW = rc.right - rc.left, winH = rc.bottom - rc.top;

        // Double-buffer only the dirty region (rcPaint), no full-window bitmap each time
        int pw = max(1L, ps.rcPaint.right - ps.rcPaint.left);
        int ph = max(1L, ps.rcPaint.bottom - ps.rcPaint.top);
        HDC hdc = CreateCompatibleDC(hdcReal);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdcReal, pw, ph);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);
        SetViewportOrgEx(hdc, -ps.rcPaint.left, -ps.rcPaint.top, NULL);

        HBRUSH bgWindow = CreateSolidBrush(ThemeWindowColor());
        FillRect(hdc, &rc, bgWindow);
        DeleteObject(bgWindow);

        DrawTopWhiteHeaderShadow(hdc, winW);

        RECT cRect = GetCanvasRect(winW, winH);
        DrawTopToolbarSeparators(hdc, cRect);

        if (g_currentView == VIEW_VISUAL) {
            DrawCanvasGrid(hdc, cRect);
            DrawShapes(hdc, cRect);
            DrawPreviewShape(hdc);

            int gridBottom = (g_propGridBottom > cRect.top) ? g_propGridBottom : cRect.top + 200;
            RECT propRect = { winW - 300, cRect.top, winW - 10, gridBottom };
            DrawVSPropertyGridBackground(hdc, propRect);

            // Attribute panel starts below the Clone button with a small gap.
            // gridBottom is currentY + 2, while Clone ends at currentY + 96.
            int attributeTop = gridBottom + 106;
            RECT attrRect = { winW - 300, attributeTop, winW - 10, cRect.bottom };
            DrawShapeAttributePanel(hdc, attrRect);
        }

        if (g_currentView == VIEW_SCRIPT) {
            // Thin frame around the code area (gutter + code), blended with the theme
            E2Palette P = E2GetPalette(g_darkTheme);
            HPEN fp = CreatePen(PS_SOLID, 1, P.frame);
            HPEN ofp = (HPEN)SelectObject(hdc, fp);
            HBRUSH ofb = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, 10, 81, winW - 10, winH - 38);
            SelectObject(hdc, ofb);
            SelectObject(hdc, ofp);
            DeleteObject(fp);
        }

        DrawCanvasBottomBar(hdc, cRect, winW, winH);

        SetViewportOrgEx(hdc, 0, 0, NULL);
        BitBlt(hdcReal, ps.rcPaint.left, ps.rcPaint.top, pw, ph, hdc, 0, 0, SRCCOPY);

        SelectObject(hdc, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hdc);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        if (g_hBgBitmap) DeleteObject(g_hBgBitmap);
        if (g_hUIFont) DeleteObject(g_hUIFont);
        if (g_hBoldFont) DeleteObject(g_hBoldFont);
        if (g_hCodeFont) DeleteObject(g_hCodeFont);
        if (g_hVarListFont) { DeleteObject(g_hVarListFont); g_hVarListFont = NULL; }
        if (g_hAppIcon) DestroyIcon(g_hAppIcon);
        if (g_gdiplusToken) {
            Gdiplus::GdiplusShutdown(g_gdiplusToken);
            g_gdiplusToken = 0;
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
        g_gdiplusToken = 0;
    }

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "E2_HUD_Studio";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_DBLCLKS;
    RegisterClassA(&wc);

    WNDCLASSA wcPopup = {};
    wcPopup.lpfnWndProc = ManageShapesPopupProc;
    wcPopup.hInstance = hInstance;
    wcPopup.lpszClassName = "ManageShapesPopupClass";
    wcPopup.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcPopup.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wcPopup);

    WNDCLASSA wcVarPopup = {};
    wcVarPopup.lpfnWndProc = VariablePopupProc;
    wcVarPopup.hInstance = hInstance;
    wcVarPopup.lpszClassName = "VariablePopupClass";
    wcVarPopup.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcVarPopup.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wcVarPopup);

    HWND hwnd = CreateWindowExA(0, "E2_HUD_Studio", "Ambata Editor",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    RECT rc;
    GetClientRect(hwnd, &rc);
    SendMessage(hwnd, WM_SIZE, 0, MAKELONG(rc.right - rc.left, rc.bottom - rc.top));

    SwitchViewMode(VIEW_VISUAL);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
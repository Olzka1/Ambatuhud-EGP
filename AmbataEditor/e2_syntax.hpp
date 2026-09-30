#pragma once
// ไฮไลต์สี E2 (Expression2) แบบเดียวกับตัวแก้ไขใน Wiremod
// - E2Highlight()      : แยก token แล้วคืนช่วงตัวอักษรพร้อมสี (ไม่ผูกกับ Windows control)
// - E2ApplyToRichEdit(): ใส่ข้อความ + สี ลง RichEdit (read-only) ในรอบเดียว
//
// โหมด dark ใช้พาเลตต์ตรงกับ E2 editor ใน Garry's Mod
// โหมด light ใช้สีเดียวกันแต่เข้มขึ้น เพื่อให้อ่านออกบนพื้นขาว

#ifndef _RICHEDIT_VER
#define _RICHEDIT_VER 0x0500
#endif

#include <windows.h>
#include <richedit.h>
#include <string>
#include <vector>
#include <cctype>

struct E2Palette {
    COLORREF text, directive, number, function, variable, string, keyword, comment, pp, type, constant;
    COLORREF bg;          // พื้นหลังโค้ด
    COLORREF gutterBg;    // พื้นหลังแถบเลขบรรทัด
    COLORREF gutterNum;   // สีเลขบรรทัด (จาง)
    COLORREF gutterLine;  // เส้นแบ่งระหว่างเลขบรรทัดกับโค้ด (จางมาก)
    COLORREF frame;       // กรอบรอบพื้นที่โค้ด
};

inline E2Palette E2GetPalette(bool dark) {
    E2Palette p;
    if (dark) {
        p.text      = RGB(210, 213, 217);
        p.directive = RGB(214, 216, 160);
        p.number    = RGB(214, 174, 174);
        p.function  = RGB(178, 178, 216);
        p.variable  = RGB(170, 210, 170);
        p.string    = RGB(162, 164, 170);
        p.keyword   = RGB(170, 210, 210);
        p.comment   = RGB(138, 142, 148);
        p.pp        = RGB(204, 126, 204);
        p.type      = RGB(210, 170, 124);
        p.constant  = RGB(208, 174, 208);
        p.bg        = RGB(41, 41, 41);
        p.gutterBg  = RGB(41, 41, 41);
        p.gutterNum = RGB(112, 116, 124);
        p.gutterLine= RGB(58, 61, 67);
        p.frame     = RGB(76, 79, 85);
    } else {
        p.text      = RGB(30, 30, 30);
        p.directive = RGB(140, 120, 0);
        p.number    = RGB(190, 70, 70);
        p.function  = RGB(50, 60, 200);
        p.variable  = RGB(20, 130, 40);
        p.string    = RGB(110, 110, 110);
        p.keyword   = RGB(0, 130, 140);
        p.comment   = RGB(120, 120, 120);
        p.pp        = RGB(170, 20, 170);
        p.type      = RGB(200, 100, 20);
        p.constant  = RGB(160, 60, 160);
        p.bg        = RGB(255, 255, 255);
        p.gutterBg  = RGB(250, 250, 251);
        p.gutterNum = RGB(176, 176, 182);
        p.gutterLine= RGB(232, 232, 236);
        p.frame     = RGB(200, 200, 205);
    }
    return p;
}

struct E2Span {
    int start;
    int len;
    COLORREF color;
};

inline bool E2IsIdStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
inline bool E2IsIdChar(char c)  { return std::isalnum((unsigned char)c) || c == '_'; }

inline bool E2IsKeyword(const std::string& w) {
    static const char* kw[] = { "if", "elseif", "else", "while", "for", "foreach", "switch", "case", "default",
                                "break", "continue", "return", "local", "function", "event", "try", "catch", "do" };
    for (size_t i = 0; i < sizeof(kw) / sizeof(kw[0]); i++) if (w == kw[i]) return true;
    return false;
}

inline bool E2IsTypeName(const std::string& w) {
    static const char* ty[] = { "number", "vector", "vector2", "vector4", "string", "entity", "array", "table", "angle",
                                "wirelink", "matrix", "matrix2", "matrix4", "quaternion", "bone", "ranger", "complex",
                                "normal", "xwl", "xvd", "xgt", "xrd", "xdc", "xbn", "xhs", "xcl", "xrd" };
    for (size_t i = 0; i < sizeof(ty) / sizeof(ty[0]); i++) if (w == ty[i]) return true;
    return false;
}

// s ต้องใช้ '\n' คั่นบรรทัดเท่านั้น (ไม่มี '\r')
inline std::vector<E2Span> E2Highlight(const std::string& s, const E2Palette& P) {
    std::vector<E2Span> spans;
    const int n = (int)s.size();
    int i = 0;
    bool lineStart = true;   // ยังไม่เจออะไรนอกจากช่องว่างในบรรทัดนี้
    int dirMode = 0;         // 0 = โค้ดปกติ, 1 = รายการตัวแปร (@inputs/@outputs/@persist), 2 = ข้อความหลัง @name ฯลฯ

    auto add = [&](int a, int b, COLORREF c) { if (b > a) { E2Span sp = { a, b - a, c }; spans.push_back(sp); } };

    while (i < n) {
        char c = s[i];

        if (c == '\n') { i++; lineStart = true; dirMode = 0; continue; }
        if (c == ' ' || c == '\t') { i++; continue; }

        if (dirMode == 2) {   // ส่วนที่เหลือของบรรทัด @name / @model ... ใช้สี directive
            int j = i;
            while (j < n && s[j] != '\n') j++;
            add(i, j, P.directive);
            i = j;
            continue;
        }

        if (c == '#') {
            if (i + 1 < n && s[i + 1] == '[') {   // #[ ... ]#
                size_t e = s.find("]#", (size_t)i + 2);
                int j = (e == std::string::npos) ? n : (int)e + 2;
                add(i, j, P.comment);
                i = j; lineStart = false;
                continue;
            }
            if (lineStart) {
                int j = i + 1;
                while (j < n && std::isalpha((unsigned char)s[j])) j++;
                std::string w = s.substr(i + 1, j - i - 1);
                if (w == "ifdef" || w == "ifndef" || w == "else" || w == "endif" || w == "include" ||
                    w == "error" || w == "undef" || w == "define") {
                    add(i, j, P.pp);
                    i = j; lineStart = false;
                    continue;
                }
            }
            int j = i;
            while (j < n && s[j] != '\n') j++;
            add(i, j, P.comment);
            i = j;
            continue;
        }

        if (c == '@' && lineStart) {
            int j = i + 1;
            while (j < n && E2IsIdChar(s[j])) j++;
            std::string w = s.substr(i + 1, j - i - 1);
            add(i, j, P.directive);
            dirMode = (w == "inputs" || w == "outputs" || w == "persist") ? 1 : 2;
            i = j; lineStart = false;
            continue;
        }
        lineStart = false;

        if (c == '"') {
            int j = i + 1;
            while (j < n && s[j] != '"' && s[j] != '\n') {
                if (s[j] == '\\' && j + 1 < n && s[j + 1] != '\n') j++;
                j++;
            }
            if (j < n && s[j] == '"') j++;
            add(i, j, P.string);
            i = j;
            continue;
        }

        if (std::isdigit((unsigned char)c) || (c == '.' && i + 1 < n && std::isdigit((unsigned char)s[i + 1]))) {
            int j = i;
            if (c == '0' && i + 1 < n && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
                j += 2;
                while (j < n && std::isxdigit((unsigned char)s[j])) j++;
            } else {
                while (j < n && (std::isdigit((unsigned char)s[j]) || s[j] == '.')) j++;
                if (j < n && (s[j] == 'e' || s[j] == 'E')) {
                    int k = j + 1;
                    if (k < n && (s[k] == '+' || s[k] == '-')) k++;
                    if (k < n && std::isdigit((unsigned char)s[k])) {
                        j = k;
                        while (j < n && std::isdigit((unsigned char)s[j])) j++;
                    }
                }
            }
            add(i, j, P.number);
            i = j;
            continue;
        }

        if (E2IsIdStart(c)) {
            int j = i;
            while (j < n && E2IsIdChar(s[j])) j++;
            std::string w = s.substr(i, j - i);

            bool afterColon = (i > 0 && s[i - 1] == ':');          // Res:vector2 / Res:x()
            int q = j;
            while (q < n && (s[q] == ' ' || s[q] == '\t')) q++;
            char next = (q < n) ? s[q] : 0;

            bool colored = true;
            COLORREF col = P.text;

            if (dirMode == 1) {
                col = afterColon ? P.type : P.variable;
            } else if (E2IsKeyword(w)) {
                col = P.keyword;
            } else if (afterColon && next == '(') {
                col = P.function;
            } else if (next == '(') {
                col = P.function;
            } else if (E2IsTypeName(w) && (afterColon || std::islower((unsigned char)w[0]))) {
                col = P.type;
            } else if (std::isupper((unsigned char)w[0])) {
                col = P.variable;
            } else if (w[0] == '_' && w.size() > 1 && std::isupper((unsigned char)w[1])) {
                col = P.constant;
            } else {
                colored = false;
            }
            if (colored) add(i, j, col);
            i = j;
            continue;
        }

        i++;   // ตัวดำเนินการ / วงเล็บ: ใช้สีข้อความปกติ
    }
    return spans;
}

// ใส่โค้ดลง RichEdit พร้อมสี  (codeAnyEol จะมี \r\n หรือ \n ก็ได้)
inline void E2ApplyToRichEdit(HWND h, const std::string& codeAnyEol, const E2Palette& P) {
    if (!h) return;

    std::string t;
    t.reserve(codeAnyEol.size());
    for (size_t k = 0; k < codeAnyEol.size(); k++) if (codeAnyEol[k] != '\r') t += codeAnyEol[k];

    std::vector<E2Span> spans = E2Highlight(t, P);

    // RichEdit นับตัวคั่นบรรทัด 1 ตัวอักษร จึงใช้ '\r' ตัวเดียวคั่นบรรทัด ทำให้ตำแหน่งตรงกับ index ใน t
    std::string rt = t;
    for (size_t k = 0; k < rt.size(); k++) if (rt[k] == '\n') rt[k] = '\r';

    // WM_SETREDRAW(TRUE) ทำให้หน้าต่างที่ซ่อนอยู่ "โผล่ขึ้นมา" (ตั้ง WS_VISIBLE) จึงใช้เฉพาะตอนที่มองเห็นอยู่แล้ว
    const bool wasVisible = IsWindowVisible(h) != FALSE;
    if (wasVisible) SendMessage(h, WM_SETREDRAW, FALSE, 0);
    SendMessage(h, EM_SETREADONLY, FALSE, 0);
    SendMessage(h, EM_HIDESELECTION, TRUE, 0);
    SendMessage(h, EM_SETBKGNDCOLOR, 0, (LPARAM)P.bg);
    SetWindowTextA(h, rt.c_str());

    CHARFORMAT2A cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.dwEffects = 0;                       // ห้ามใช้ auto-color
    cf.crTextColor = P.text;
    SendMessage(h, EM_SETSEL, 0, -1);
    SendMessage(h, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    for (size_t k = 0; k < spans.size(); k++) {
        cf.crTextColor = spans[k].color;
        SendMessage(h, EM_SETSEL, spans[k].start, spans[k].start + spans[k].len);
        SendMessage(h, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    }

    // เว้นขอบซ้ายเล็กน้อย ไม่ให้ตัวอักษรติดเส้นแบ่งเลขบรรทัด (ไม่กระทบความสูงบรรทัด)
    PARAFORMAT pf;
    ZeroMemory(&pf, sizeof(pf));
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_STARTINDENT;
    pf.dxStartIndent = 120;   // twips (~8px)
    SendMessage(h, EM_SETSEL, 0, -1);
    SendMessage(h, EM_SETPARAFORMAT, 0, (LPARAM)&pf);

    SendMessage(h, EM_SETSEL, 0, 0);
    SendMessage(h, EM_HIDESELECTION, FALSE, 0);
    SendMessage(h, EM_SETREADONLY, TRUE, 0);
    if (wasVisible) {
        SendMessage(h, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(h, NULL, TRUE);
    }
}

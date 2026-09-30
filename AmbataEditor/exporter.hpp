#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include <windows.h>
#include "model.hpp"

using namespace std;

class HudExporter {
public:
    string path;

    HudExporter(string file_path = "") : path(file_path) {}

    static Point get_abs_pos(const vector<DrawShape>& shapes, int index) {
        if (index < 0 || index >= (int)shapes.size()) return { 0.0, 0.0 };
        Point p = shapes[index].p1;
        int pIdx = shapes[index].parentIndex;
        int depth = 0;
        while (pIdx >= 0 && pIdx < (int)shapes.size() && depth < 50) {
            p.first += shapes[pIdx].p1.first;
            p.second += shapes[pIdx].p1.second;
            pIdx = shapes[pIdx].parentIndex;
            depth++;
        }
        return p;
    }

    // @name ของ E2 ต้องอยู่บรรทัดเดียว: ตัด CR/LF ทิ้ง และตัดช่องว่างหัวท้าย
    static string sanitize_e2_name(const string& raw) {
        string out;
        for (size_t i = 0; i < raw.size(); i++) {
            if (raw[i] != '\r' && raw[i] != '\n') out += raw[i];
        }
        size_t a = out.find_first_not_of(" \t");
        if (a == string::npos) return "myass hud";
        size_t b = out.find_last_not_of(" \t");
        return out.substr(a, b - a + 1);
    }

    static string sanitize_expr(const string& raw, const string& fallback) {
        string out = raw;
        size_t a = out.find_first_not_of(" \t");
        if (a == string::npos) return fallback;
        size_t b = out.find_last_not_of(" \t");
        return out.substr(a, b - a + 1);
    }

    // ------------------------------------------------------------------
    // ค่าที่ "E2 ได้รับจริง" ของแต่ละ shape
    // ใช้ร่วมกันทั้งตอนสร้างโค้ด E2 และใน Attribute panel ของ editor
    // (ตัวเลขทั้งหมดคือค่าก่อนคูณ Scale เหมือนที่เห็นในสคริปต์)
    // ------------------------------------------------------------------
    struct ShapeAttr {
        int id = 0;                  // EGP index ในสคริปต์ที่ export
        int parentId = 0;            // 0 = ไม่มี parent
        bool parentIsTracker = false;
        int trackerId = 0;           // > 0 ถ้าเป็น Egp3D
        string fn;                   // ฟังก์ชัน E2 เช่น egpBox / egpCircleOutline
        bool isLine = false, isText = false, hasSize = false;
        string posX, posY;           // ตำแหน่งที่ส่งให้ E2 (box/circle = จุดศูนย์กลาง, line/text = จุดเริ่ม)
        string sizeX, sizeY;         // box/rect = กว้าง/สูง, circle = รัศมี (ตามที่ E2 รับ)
        string endX, endY;           // line = ปลายเส้น
        string angle;
        string thickness;            // egpSize(id, n)
        string textSize;
    };

    static string fmt_num(double v) {
        if (std::fabs(v) < 0.0005) return "0";
        char buf[64];
        snprintf(buf, sizeof(buf), "%.3f", v);
        string s = buf;
        if (s.find('.') != string::npos) {
            while (!s.empty() && s.back() == '0') s.pop_back();
            if (!s.empty() && s.back() == '.') s.pop_back();
        }
        return s;
    }

    static int tracker_count(const vector<DrawShape>& shapes) {
        int n = 0;
        for (size_t i = 0; i < shapes.size(); i++) if (shapes[i].isEgp3D) n++;
        return n;
    }

    // tracker ถูกสร้างก่อน จึงเลื่อน id ของ shape ทุกตัวไปข้างหลัง
    static int visual_id(const vector<DrawShape>& shapes, size_t i) {
        return tracker_count(shapes) + (int)i + 1;
    }

    static int tracker_id_of(const vector<DrawShape>& shapes, size_t i) {
        if (i >= shapes.size() || !shapes[i].isEgp3D) return 0;
        int n = 0;
        for (size_t j = 0; j <= i; j++) if (shapes[j].isEgp3D) n++;
        return n;
    }

    static ShapeAttr compute_attr(const vector<DrawShape>& shapes, size_t i) {
        ShapeAttr a;
        if (i >= shapes.size()) return a;
        const DrawShape& s = shapes[i];
        a.id = visual_id(shapes, i);
        a.trackerId = tracker_id_of(shapes, i);
        if (s.isEgp3D) {
            a.parentId = a.trackerId;
            a.parentIsTracker = true;
        } else if (s.parentIndex >= 0 && s.parentIndex < (int)shapes.size()) {
            a.parentId = visual_id(shapes, (size_t)s.parentIndex);
        }

        const bool is3d = s.isEgp3D;
        const double w = s.p2.first - s.p1.first;
        const double h = s.p2.second - s.p1.second;

        // Egp3D: ตำแหน่งจริงมาจาก tracker จึงวาง shape ที่ 0,0 เทียบกับ tracker
        string x1 = is3d ? "0" : (s.useVarX ? s.varXName : fmt_num(s.p1.first));
        string y1 = is3d ? "0" : (s.useVarY ? s.varYName : fmt_num(s.p1.second));
        string x2 = is3d ? fmt_num(w) : (s.useVarX ? s.varXName : fmt_num(s.p2.first));
        string y2 = is3d ? fmt_num(h) : (s.useVarY ? s.varYName : fmt_num(s.p2.second));

        a.angle = s.useVarAngle ? sanitize_expr(s.varAngleName, "Angle") : fmt_num(s.angle);
        a.thickness = to_string(s.thickness);
        a.textSize = to_string(s.textSize);

        switch (s.type) {
        case TOOL_LINE:
            a.fn = "egpLine"; a.isLine = true;
            a.posX = x1; a.posY = y1; a.endX = x2; a.endY = y2;
            break;
        case TOOL_BOX:
        case TOOL_RECT: {
            a.fn = (s.type == TOOL_BOX) ? "egpBox" : "egpBoxOutline";
            a.hasSize = true;
            a.posX = is3d ? "0" : (s.useVarX ? s.varXName : fmt_num(s.p1.first + w / 2.0));   // E2 รับจุดศูนย์กลาง
            a.posY = is3d ? "0" : (s.useVarY ? s.varYName : fmt_num(s.p1.second + h / 2.0));
            a.sizeX = s.useVarSizeX ? s.varSizeXName : fmt_num(std::fabs(w));
            a.sizeY = s.useVarSizeY ? s.varSizeYName : fmt_num(std::fabs(h));
            break;
        }
        case TOOL_CIRCLE_SOLID:
        case TOOL_CIRCLE_OUTLINE: {
            a.fn = (s.type == TOOL_CIRCLE_SOLID) ? "egpCircle" : "egpCircleOutline";
            a.hasSize = true;
            a.posX = x1; a.posY = y1;
            // รัศมี = ระยะจาก p1 ถึง p2 (ตรงกับที่ editor วาด) ใช้ทั้งแกน X และ Y จึงเป็นวงกลมเสมอ
            double rad = std::sqrt(w * w + h * h);
            a.sizeX = s.useVarSizeX ? s.varSizeXName : fmt_num(rad);
            a.sizeY = s.useVarSizeY ? s.varSizeYName : fmt_num(rad);
            break;
        }
        case TOOL_TEXT:
            a.fn = "egpText"; a.isText = true;
            a.posX = x1; a.posY = y1;
            break;
        default:
            break;
        }
        return a;
    }

    static string generate_e2_code(const vector<DrawShape>& shapes, const vector<E2Variable>& variables = {}, bool useInterval = true, int intervalMs = 100,
                                   const string& e2Name = "myass hud") {
        ostringstream ss;
        ss << "@name " << sanitize_e2_name(e2Name) << "\r\n";

        vector<string> inputVars;
        vector<string> outputVars;
        vector<string> persistVars;
        vector<string> variableExpressions;
        for (const E2Variable& v : variables) {
            string varName = v.name;
            replace(varName.begin(), varName.end(), ' ', '_');
            if (varName.empty()) continue;
            string decl = varName + ":" + InputTypeToString(v.type);
            if (v.direction == VARIABLE_OUTPUT) outputVars.push_back(decl);
            else if (v.direction == VARIABLE_PERSIST) persistVars.push_back(decl);
            else inputVars.push_back(decl);

            string expr = sanitize_expr(v.expression, "");
            if (!expr.empty()) {
                variableExpressions.push_back("    " + varName + " = (" + expr + ")");
            }
        }

        // Backward compatibility: a Text shape marked as an input is also an E2 variable.
        for (size_t i = 0; i < shapes.size(); i++) {
            if (shapes[i].type == TOOL_TEXT && shapes[i].isInput) {
                string varName = shapes[i].varName.empty() ? shapes[i].textContent : shapes[i].varName;
                replace(varName.begin(), varName.end(), ' ', '_');
                if (varName.empty()) varName = "Input_" + to_string(i + 1);
                string decl = varName + ":" + InputTypeToString(shapes[i].inputType);
                bool exists = false;
                for (const string& x : inputVars) if (x == decl) exists = true;
                for (const string& x : outputVars) if (x == decl) exists = true;
                if (!exists) inputVars.push_back(decl);
            }
        }

        ss << "@inputs EGP:wirelink";
        for (const auto& inVar : inputVars) ss << " " << inVar;
        ss << "\r\n";
        if (!outputVars.empty()) {
            ss << "@outputs";
            for (const auto& outVar : outputVars) ss << " " << outVar;
            ss << "\r\n";
        }

        ss << "@persist X Y Res:vector2 ProjRes:vector2 Scale:vector2";
        for (const auto& p : persistVars) ss << " " << p;
        ss << "\r\n\r\n";
        
        if (useInterval) {
            ss << "interval(" << intervalMs << ")\r\n\r\n";
        }

        if (!variableExpressions.empty()) {
            for (const string& exprLine : variableExpressions) ss << exprLine << "\r\n";
            ss << "\r\n";
        }

        ss << "if ( first() | duped() )\r\n{\r\n";
        ss << "    EGP:egpClear()\r\n";
        ss << "    Res = egpScrSize(owner())\r\n";
        ss << "    X   = Res:x()\r\n";
        ss << "    Y   = Res:y()\r\n";
        ss << "    Res /= 2\r\n";
        ss << "    ProjRes = vec2( 1920, 1080 )\r\n";
        ss << "    Scale = vec2(X/ProjRes:x(), Y/ProjRes:y())\r\n\r\n";

        // EGP3D uses a dedicated invisible 3DTracker object. The visible EGP
        // shape is parented to that tracker, exactly like the Wiremod example:
        //   EGP:egp3DTracker(Tracker, TargetPos)
        //   EGP:egp... (Visual)
        //   EGP:egpParent(Visual, Tracker)
        // Reserve the first IDs for trackers and shift visible object IDs after them.
        int trackerCount = 0;
        for (size_t i = 0; i < shapes.size(); i++) {
            if (shapes[i].isEgp3D) trackerCount++;
        }

        auto visualId = [&shapes](size_t shapeIndex) -> int {
            return visual_id(shapes, shapeIndex);
        };

        int trackerId = 0;
        for (size_t i = 0; i < shapes.size(); i++) {
            const auto& s = shapes[i];
            if (s.isEgp3D) {
                ++trackerId;
                string pos3DStr = sanitize_expr(s.var3DName, "Pos");
                ss << "    EGP:egp3DTracker( " << trackerId << ", " << pos3DStr << " )\r\n";
            }
        }
        if (trackerCount > 0) ss << "\r\n";

        for (size_t i = 0; i < shapes.size(); i++) {
            const auto& s = shapes[i];
            ShapeAttr a = compute_attr(shapes, i);
            int egp_id = a.id;
            int r = GetRValue(s.color), g = GetGValue(s.color), b = GetBValue(s.color);

            if (s.type == TOOL_LINE) {
                ss << "    EGP:egpLine( " << egp_id << ", vec2(" << a.posX << "*Scale:x(), " << a.posY << "*Scale:y()), vec2(" << a.endX << "*Scale:x(), " << a.endY << "*Scale:y()) )\r\n";
                ss << "    EGP:egpSize( " << egp_id << ", " << a.thickness << " )\r\n";
            }
            else if (a.hasSize) {   // box / rect / circle : ใช้ค่าชุดเดียวกับ Attribute panel
                ss << "    EGP:" << a.fn << "( " << egp_id << ", vec2(" << a.posX << "*Scale:x(), " << a.posY << "*Scale:y()), vec2(" << a.sizeX << "*Scale:x(), " << a.sizeY << "*Scale:y()) )\r\n";
                ss << "    EGP:egpSize( " << egp_id << ", " << a.thickness << " )\r\n";
            }
            else if (s.type == TOOL_TEXT) {
                string posStr = "vec2(" + a.posX + "*Scale:x(), " + a.posY + "*Scale:y())";

                if (s.isInput) {
                    string varName = s.varName.empty() ? s.textContent : s.varName;
                    replace(varName.begin(), varName.end(), ' ', '_');
                    if (s.inputType == INPUT_NUM || s.inputType == INPUT_VECTOR || s.inputType == INPUT_BOOL) {
                        ss << "    EGP:egpText( " << egp_id << ", toString(" << varName << "), " << posStr << " )\r\n";
                    } else {
                        ss << "    EGP:egpText( " << egp_id << ", " << varName << ", " << posStr << " )\r\n";
                    }
                } else {
                    ss << "    EGP:egpText( " << egp_id << ", \"" << s.textContent << "\", " << posStr << " )\r\n";
                }
                ss << "    EGP:egpSize( " << egp_id << ", " << a.textSize << " )\r\n";
            }

            // Parent commands are emitted AFTER every visual object exists (see loop below).
            ss << "    EGP:egpColor( " << egp_id << ", vec(" << r << ", " << g << ", " << b << ") )\r\n";
            ss << "    EGP:egpAlpha( " << egp_id << ", " << max(0, min(255, s.opacity)) << " )\r\n";
        }

        // Apply all parent relationships only after ALL visual objects exist.
        // This is important when the parent is a shape created later in the
        // editor; emitting egpParent during object creation can silently fail.
        for (size_t i = 0; i < shapes.size(); i++) {
            const auto& s = shapes[i];
            int egp_id = visualId(i);

            if (s.isEgp3D) {
                int currentTrackerId = 0;
                for (size_t j = 0; j <= i; j++) {
                    if (shapes[j].isEgp3D) ++currentTrackerId;
                }
                ss << "    EGP:egpParent( " << egp_id << ", " << currentTrackerId << " )\r\n";
            }
            else if (s.parentIndex >= 0 && s.parentIndex < (int)shapes.size()) {
                int parent_egp_id = visualId((size_t)s.parentIndex);
                ss << "    EGP:egpParent( " << egp_id << ", " << parent_egp_id << " )\r\n";
            }
        }

        ss << "}\r\n\r\n";

        for (size_t i = 0; i < shapes.size(); i++) {
            const auto& s = shapes[i];
            int egp_id = visualId(i);

            if (s.isEgp3D) {
                // IMPORTANT: egpPos(vector) must update the 3DTracker itself,
                // not the ordinary Box/Text/etc. The visible object follows
                // the tracker through egpParent(), matching Wiremod's usage.
                int currentTrackerId = 0;
                for (size_t j = 0; j <= i; j++) {
                    if (shapes[j].isEgp3D) ++currentTrackerId;
                }
                string pos3DStr = sanitize_expr(s.var3DName, "Pos");
                ss << "EGP:egpPos( " << currentTrackerId << ", " << pos3DStr << " )\r\n";
            }
            else if (s.useVarX || s.useVarY) {
                ShapeAttr a = compute_attr(shapes, i);
                ss << "EGP:egpPos( " << egp_id << ", vec2(" << a.posX << "*Scale:x(), " << a.posY << "*Scale:y()) )\r\n";
            }

            if (s.type == TOOL_TEXT && s.isInput) {
                string varName = s.varName.empty() ? s.textContent : s.varName;
                replace(varName.begin(), varName.end(), ' ', '_');
                
                if (s.inputType == INPUT_NUM || s.inputType == INPUT_VECTOR || s.inputType == INPUT_BOOL) {
                    ss << "EGP:egpSetText( " << egp_id << ", toString(" << varName << ") )\r\n";
                } else {
                    ss << "EGP:egpSetText( " << egp_id << ", " << varName << " )\r\n";
                }
            }
        }

        // Angle: อยู่นอก if(first()) เพื่อให้ทำงานทุกรอบ (รองรับมุมที่เป็นตัวแปร)
        // วางหลัง egpPos/egpParent เสมอ
        for (size_t i = 0; i < shapes.size(); i++) {
            const auto& s = shapes[i];
            int egp_id = visualId(i);
            string angleStr = compute_attr(shapes, i).angle;
            if (s.useVarAngle || std::fabs(s.angle) > 0.0001) {
                ss << "EGP:egpAngle( " << egp_id << ", " << angleStr << " )\r\n";
            }
        }

        return ss.str();
    }

    static void export_to_file(const string& filename, const vector<DrawShape>& shapes, const vector<E2Variable>& variables, bool useInterval = true, int intervalMs = 100,
                               const string& e2Name = "E2 HUD Studio Export") {
        ofstream file(filename);
        file << generate_e2_code(shapes, variables, useInterval, intervalMs, e2Name);
        file.close();
    }
};
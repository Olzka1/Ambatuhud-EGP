#ifndef MODEL_HPP
#define MODEL_HPP

#include <windows.h>
#include <string>
#include <vector>
#include <utility>

typedef std::pair<double, double> Point;

enum ViewMode {
    VIEW_VISUAL,
    VIEW_SCRIPT
};

enum ToolType {
    TOOL_NONE,
    TOOL_SELECT,
    TOOL_LINE,
    TOOL_RECT,
    TOOL_BOX,
    TOOL_CIRCLE_OUTLINE,
    TOOL_CIRCLE_SOLID,
    TOOL_TEXT
};

enum InputType {
    INPUT_NUM,
    INPUT_VECTOR,
    INPUT_STRING,
    // E2 booleans are represented by number (0/1) in the generated declaration.
    // Keeping a distinct editor type lets the UI expose a Bool parameter cleanly.
    INPUT_BOOL
};

enum VariableDirection {
    VARIABLE_INPUT,
    VARIABLE_OUTPUT,
    VARIABLE_PERSIST
};

struct E2Variable {
    std::string name;
    InputType type;
    VariableDirection direction;
    // Optional E2 expression applied when this variable is exported.
    std::string expression;

    E2Variable(const std::string& n = "Var1", InputType t = INPUT_NUM, VariableDirection d = VARIABLE_INPUT)
        : name(n), type(t), direction(d), expression("") {}
};

inline std::string InputTypeToString(InputType type) {
    switch (type) {
        case INPUT_NUM:    return "number";
        case INPUT_VECTOR: return "vector";
        case INPUT_STRING: return "string";
        case INPUT_BOOL:   return "number";
        default:           return "number";
    }
}

struct Project {
    int width;
    int height;

    static Project create_new(int w, int h) {
        Project p;
        p.width = w;
        p.height = h;
        return p;
    }
};

struct DrawShape {
    ToolType type;
    Point p1;
    Point p2;
    COLORREF color;
    int opacity;
    int thickness;
    std::string textContent;
    int textSize = 24;
    double angle = 0.0;
    bool isInput;
    std::string varName;
    InputType inputType;
    int parentIndex = -1;       
    bool useVarX = false;
    bool useVarY = false; 
    bool useVarSizeX = false;
    bool useVarSizeY = false;
    bool useVarAngle = false;
    std::string varSizeXName = "SizeX";
    std::string varSizeYName = "SizeY";
    std::string varAngleName = "Angle";
    std::string varXName;  
    std::string varYName;
    bool isEgp3D = false;
    std::string var3DName;
    std::string name;

    DrawShape() 
        : type(TOOL_NONE), p1({0,0}), p2({0,0}), color(RGB(0,0,0)), opacity(255),
          thickness(1), textContent(""), textSize(24), isInput(false), varName("Var1"), 
          inputType(INPUT_NUM), parentIndex(-1), useVarX(false), useVarY(false), useVarSizeX(false), useVarSizeY(false), useVarAngle(false),
          varSizeXName("SizeX"), varSizeYName("SizeY"), varAngleName("Angle"),
          varXName("X"), varYName("Y"), isEgp3D(false), var3DName("Pos"), name("shape") {}

    DrawShape(ToolType t, Point pt1, Point pt2, COLORREF c, int thick, 
              std::string text = "", bool input = false, std::string var = "Var1", 
              InputType inType = INPUT_NUM)
        : type(t), p1(pt1), p2(pt2), color(c), opacity(255), thickness(thick), 
          textContent(text), textSize(24), angle(0.0), isInput(input), varName(var), inputType(inType),
          parentIndex(-1), useVarX(false), useVarY(false), useVarSizeX(false), useVarSizeY(false), useVarAngle(false),
          varSizeXName("SizeX"), varSizeYName("SizeY"), varAngleName("Angle"),
          varXName("X"), varYName("Y"), isEgp3D(false), var3DName("Pos"), name("shape") {}
};

#endif // MODEL_HPP
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <sstream> 
#include <functional>

using namespace std;

enum TokClass { TK_KW, TK_ID, TK_NUM, TK_OP, TK_SEP, TK_EOF };

struct Token {
    TokClass cls;
    string lex;
    int index;
    int pos;
};

class Lexer {
public:
    vector<string> ids, nums;
    vector<string> keywords = { "def","if","else","begin","end","return","print","input" };
    vector<string> operators = { "+","-","*","/","%","=","+=","-=","*=","/=","%=","==","!=","<",">","<=",">=" };
    vector<string> separators = { "(",")",":",",",";" };

    int findIndex(const vector<string>& v, const string& s) {
        auto it = find(v.begin(), v.end(), s);
        return (it != v.end()) ? static_cast<int>(distance(v.begin(), it)) : -1;
    }
    void addUnique(vector<string>& v, const string& s) {
        if (findIndex(v, s) == -1) v.push_back(s);
    }

    vector<Token> tokenize(const string& filename) {
        ifstream in(filename);
        if (!in) { cerr << "Error: Cannot open " << filename << endl; return {}; }
        string src;
        { string line; while (getline(in, line)) src += line + " "; }
        src += " ";

        vector<Token> tokens;
        size_t i = 0; int pos = 0;
        auto emit = [&](TokClass cls, const string& lex, int idx) {
            tokens.push_back({ cls, lex, idx, pos++ });
            };

        while (i < src.length()) {
            char c = src[i];
            if (isspace((unsigned char)c)) { i++; continue; }
            if (c == '#') { while (i < src.length() && src[i] != '\n') i++; continue; }

            if (isalpha(c) || c == '_') {
                string lex(1, c);
                while (i + 1 < src.length() && (isalnum(src[i + 1]) || src[i + 1] == '_')) lex += src[++i];
                int kw = findIndex(keywords, lex);
                if (kw != -1) emit(TK_KW, lex, kw);
                else { addUnique(ids, lex); emit(TK_ID, lex, findIndex(ids, lex)); }
                i++; continue;
            }
            if (isdigit(c)) {
                string lex(1, c);
                while (i + 1 < src.length() && isdigit(src[i + 1])) lex += src[++i];
                addUnique(nums, lex); emit(TK_NUM, lex, findIndex(nums, lex));
                i++; continue;
            }
            if (string("+-*/%=!<>").find(c) != string::npos) {
                string lex(1, c);
                if (i + 1 < src.length()) {
                    string two = lex + src[i + 1];
                    if (findIndex(operators, two) != -1) { lex = two; i++; }
                }
                int op = findIndex(operators, lex);
                if (op != -1) emit(TK_OP, lex, op);
                i++; continue;
            }
            if (string("():,;").find(c) != string::npos) {
                string s(1, c); int sep = findIndex(separators, s);
                if (sep != -1) emit(TK_SEP, s, sep);
                i++; continue;
            }
            i++;
        }
        emit(TK_EOF, "#", 0);
        return tokens;
    }

    void printTokens(const vector<Token>& tokens) const {
        cout << "\nТОКЕНЫ:\n";
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& t = tokens[i];
            cout << i << ": ";
            switch (t.cls) {
            case TK_KW: cout << "KEYWORD(" << t.lex << ")"; break;
            case TK_ID: cout << "ID(" << t.lex << ")"; break;
            case TK_NUM: cout << "NUM(" << t.lex << ")"; break;
            case TK_OP: cout << "OP(" << t.lex << ")"; break;
            case TK_SEP: cout << "SEP(" << t.lex << ")"; break;
            case TK_EOF: cout << "EOF"; break;
            }
            cout << "\n";
        }
    }
};

enum NodeType {
    NT_Unknown, NT_Program, NT_StmtList, NT_Stmt,
    NT_FuncDef, NT_FuncCall, NT_ParamList, NT_Block,
    NT_IfStmt, NT_ElsePart, NT_ReturnStmt, NT_PrintStmt,
    NT_InputStmt, NT_Assign, NT_Expr, NT_BinOp, NT_UnOp,
    NT_Id, NT_Num, NT_String, NT_ExprList
};

struct TreeNode {
    NodeType type;
    string value;
    string dataType;
    int line;
    vector<TreeNode*> children;

    TreeNode(NodeType t = NT_Unknown, string v = "", string dt = "int", int ln = 0)
        : type(t), value(v), dataType(dt), line(ln) {
    }

    ~TreeNode() {
        for (auto* ch : children) delete ch;
        children.clear();
    }

    void addChild(TreeNode* child) {
        if (child) children.push_back(child);
    }

    void print(int depth = 0) const {
        string indent(depth * 2, ' ');
        cout << indent << nodeTypeToString(type);
        if (!value.empty()) cout << "[" << value << "]";
        if (!dataType.empty() && dataType != "int") cout << "<" << dataType << ">";
        cout << "\n";
        for (auto* ch : children) if (ch) ch->print(depth + 1);
    }

private:
    string nodeTypeToString(NodeType t) const {
        static const map<NodeType, string> names = {
            {NT_Program, "Program"}, {NT_StmtList, "StmtList"}, {NT_Stmt, "Stmt"},
            {NT_FuncDef, "FuncDef"}, {NT_FuncCall, "FuncCall"}, {NT_ParamList, "ParamList"},
            {NT_Block, "Block"}, {NT_IfStmt, "IfStmt"}, {NT_ElsePart, "ElsePart"},
            {NT_ReturnStmt, "Return"}, {NT_PrintStmt, "Print"}, {NT_InputStmt, "Input"},
            {NT_Assign, "Assign"}, {NT_Expr, "Expr"}, {NT_BinOp, "BinOp"}, {NT_UnOp, "UnOp"},
            {NT_Id, "Id"}, {NT_Num, "Num"}, {NT_String, "String"}, {NT_ExprList, "ExprList"}
        };
        auto it = names.find(t);
        return (it != names.end()) ? it->second : "Unknown";
    }
};

struct SymbolEntry {
    string name, type;
    bool isFunction, isDefined, isUsed;
    vector<string> paramTypes;
    int scope, line;
    SymbolEntry(string n = "", string t = "int", bool func = false, int sc = 0)
        : name(n), type(t), isFunction(func), isDefined(false), isUsed(false), scope(sc), line(0) {
    }
};

class SymbolTable {
    vector<map<string, SymbolEntry>> scopes;
    int currentScope;
public:
    SymbolTable() : currentScope(0) { enterScope(); }
    void enterScope() { scopes.emplace_back(); currentScope++; }
    void exitScope() { if (scopes.size() > 1) { scopes.pop_back(); currentScope--; } }

    bool addSymbol(const string& name, const string& type, bool isFunction = false,
        const vector<string>& params = {}, int ln = 0) {
        if (scopes.back().count(name)) {
            cerr << "[СЕМАНТИКА] Ошибка: символ '" << name << "' уже объявлен (строка " << ln << ")\n";
            return false;
        }
        SymbolEntry entry(name, type, isFunction, currentScope);
        entry.line = ln;
        if (isFunction) { entry.paramTypes = params; entry.isDefined = true; }
        scopes.back()[name] = entry;
        return true;
    }

    SymbolEntry* findSymbol(const string& name) {
        for (int i = (int)scopes.size() - 1; i >= 0; --i) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end()) { it->second.isUsed = true; return &(it->second); }
        }
        return nullptr;
    }

    string checkExprType(TreeNode* expr) {
        if (!expr) return "void";
        if (expr->type == NT_Num) return "int";
        if (expr->type == NT_Id) { auto* sym = findSymbol(expr->value); return sym ? sym->type : "unknown"; }
        if (expr->type == NT_BinOp) return "int";
        return "int";
    }

    bool checkVarDeclared(const string& name, int line) {
        auto* sym = findSymbol(name);
        if (!sym) {
            cerr << "[СЕМАНТИКА] Предупреждение: переменная '" << name << "' не объявлена (строка " << line << ")\n";
            return false;
        }
        return true;
    }
};

class CodeGenerator {
    SymbolTable* symTable;  
    vector<string> code, dataDecls;
    map<string, bool> declared;
    int tempCnt;
    int labelCounter;
    string curFunc;
    map<string, int> currentParams;
    string entryFunction;
    bool semanticError;

public:
    CodeGenerator(SymbolTable* st) : symTable(st), tempCnt(0), labelCounter(0), curFunc("main"), entryFunction("main"), semanticError(false) {}

    bool hasSemanticError() const { return semanticError; }
    void clearSemanticError() { semanticError = false; }

    string generate(TreeNode* root) {
        code.clear(); dataDecls.clear(); declared.clear(); tempCnt = 0; labelCounter = 0;
        semanticError = false;

        emitHeader();
        if (root) generateNode(root);

        if (semanticError) {
            return "";
        }

        emitFooter();
        return assemble();
    }

    bool saveToFile(const string& filename, TreeNode* root) {
        string asmCode = generate(root);
        if (asmCode.empty()) {
            if (semanticError) {
                cerr << "[КОД] Генерация прервана из-за семантических ошибок.\n";
            }
            return false;
        }
        ofstream out(filename);
        if (!out) { cerr << "[КОД] Ошибка создания файла " << filename << "\n"; return false; }
        out << asmCode; out.close();
        cout << "\n[КОД] Сохранено: " << filename << "\n";
        return true;
    }

    void printCode(TreeNode* root) {
        cout << "\n=== ОБЪЕКТНЫЙ КОД (IBM PC Macro Assembler) ===\n";
        cout << generate(root);
        cout << "==============================================\n";
    }

private:
    void emit(const string& line, int indent = 0) { code.push_back(string(indent * 4, ' ') + line); }
    void emitData(const string& line) { dataDecls.push_back("    " + line); }
    string newTemp() { return "_t" + to_string(tempCnt++); }
    string newLabel() { return "L" + to_string(labelCounter++); }
    string varName(const string& name) {
        string r = name; for (char& c : r) if (!isalnum(c) && c != '_') c = '_'; return r;
    }

    void emitHeader() {
        emit("; ===========================================");
        emit("; Сгенерированный код транслятором");
        emit("; ===========================================");
        emit("");
        emit(".MODEL SMALL");
        emit(".STACK 100h");
        emit("");
        emit("DATA SEGMENT");
        emit("DATA ENDS");
        emit("");
        emit("CODE SEGMENT");
        emit("    ASSUME CS:CODE, DS:DATA");
        emit("");
    }

    void emitPrintNumber() {
        emit("");
        emit("; === Процедура вывода целого числа (PrintNumber) ===");
        emit("PrintNumber PROC");
        emit("    PUSH AX"); emit("    PUSH BX"); emit("    PUSH CX"); emit("    PUSH DX"); emit("    PUSH SI");
        emit("    MOV BX, 10"); emit("    MOV CX, 0"); emit("    MOV SI, SP");
        emit("    CMP AX, 0"); emit("    JGE PrintNumber_positive");
        emit("    NEG AX"); emit("    PUSH AX"); emit("    MOV AH, 02h"); emit("    MOV DL, '-'"); emit("    INT 21h"); emit("    POP AX");
        emit("PrintNumber_positive:");
        emit("PrintNumber_convert:");
        emit("    XOR DX, DX"); emit("    DIV BX"); emit("    ADD DL, '0'"); emit("    PUSH DX"); emit("    INC CX");
        emit("    CMP AX, 0"); emit("    JNE PrintNumber_convert");
        emit("PrintNumber_print:");
        emit("    POP DX"); emit("    MOV AH, 02h"); emit("    INT 21h"); emit("    LOOP PrintNumber_print");
        emit("    POP SI"); emit("    POP DX"); emit("    POP CX"); emit("    POP BX"); emit("    POP AX");
        emit("    RET");
        emit("PrintNumber ENDP");
    }

    void emitReadNumber() {
        emit("");
        emit("; === Процедура ввода целого числа (ReadNumber) для 8086 ===");
        emit("ReadNumber PROC");
        emit("    PUSH BX"); emit("    PUSH CX"); emit("    PUSH DX");
        emit("    PUSH SI");
        emit("    XOR AX, AX");
        emit("    PUSH AX");
        emit("    XOR SI, SI");

        emit("ReadNumber_loop:");
        emit("    MOV AH, 01h"); emit("    INT 21h");
        emit("    CMP AL, 0Dh"); emit("    JE ReadNumber_done");
        emit("    CMP AL, '-'"); emit("    JNE ReadNumber_digit");
        emit("    MOV SI, 1"); emit("    JMP ReadNumber_loop");

        emit("ReadNumber_digit:");
        emit("    CMP AL, '0'"); emit("    JB ReadNumber_loop");
        emit("    CMP AL, '9'"); emit("    JA ReadNumber_loop");
        emit("    SUB AL, '0'");
        emit("    XOR BX, BX");
        emit("    MOV BL, AL");
        emit("    POP AX");
        emit("    MOV CX, 10");
        emit("    MUL CX");
        emit("    ADD AX, BX");
        emit("    PUSH AX");
        emit("    JMP ReadNumber_loop");

        emit("ReadNumber_done:");
        emit("    POP AX");
        emit("    CMP SI, 0"); emit("    JE ReadNumber_exit");
        emit("    NEG AX");
        emit("ReadNumber_exit:");
        emit("    POP SI"); emit("    POP DX"); emit("    POP CX"); emit("    POP BX");
        emit("    RET");
        emit("ReadNumber ENDP");
    }

    void emitFooter(const string& entryFunc = "main") {
        emit("START:");
        emit("    MOV AX, DATA", 1);
        emit("    MOV DS, AX", 1);
        emit("");

        emit("    CALL " + entryFunction, 1);
        emit("");

        emit("    MOV AH, 08h", 1);
        emit("    INT 21h", 1);
        emit("");

        emit("    MOV AH, 4Ch", 1);
        emit("    INT 21h", 1);
        emit("");
        emitPrintNumber();
        emitReadNumber();
        emit("");
        emit("CODE ENDS");
        emit("");
        emit("END START");
    }

    string assemble() {
        string result;
        for (const auto& line : code) result += line + "\n";
        size_t pos = result.find("DATA ENDS");
        if (pos != string::npos && !dataDecls.empty()) {
            string insert; for (const auto& d : dataDecls) insert += d + "\n";
            result.insert(pos, insert);
        }
        return result;
    }

    void generateNode(TreeNode* node) {
        if (!node || semanticError) return;
        switch (node->type) {
        case NT_Program: generateProgram(node); break;
        case NT_FuncDef: generateFuncDef(node); break;
        case NT_Block: generateBlock(node); break;
        case NT_StmtList: generateStmtList(node); break;
        case NT_ReturnStmt: generateReturn(node); break;
        case NT_PrintStmt: generatePrint(node); break;
        case NT_Assign: generateAssign(node); break;
        case NT_IfStmt: generateIfStmt(node); break;
        case NT_Expr: case NT_BinOp: generateExpr(node); break;
        case NT_Id: generateId(node); break;
        case NT_Num: generateNum(node); break;
        case NT_FuncCall: generateFuncCall(node); break;
        default: for (auto* c : node->children) generateNode(c); break;
        }
    }

    void generateProgram(TreeNode* node) {
        emit("; === Главная программа ===");

        vector<TreeNode*> functions;
        vector<TreeNode*> statements;

        function<void(TreeNode*)> collect = [&](TreeNode* n) {
            if (!n) return;
            if (n->type == NT_FuncDef) {
                functions.push_back(n);
            }
            else if (n->type == NT_StmtList || n->type == NT_Block) {
                for (auto* ch : n->children) collect(ch);
            }
            else if (n->type != NT_Program) {
                statements.push_back(n);
            }
            };
        for (auto* ch : node->children) collect(ch);

        for (auto* func : functions) {
            emit("");
            generateFuncDef(func);
        }

        if (!functions.empty()) {
            entryFunction = varName(functions[0]->value);
        }
    }

    void generateFuncDef(TreeNode* node) {
        string fname = varName(node->value);
        curFunc = fname;
        currentParams.clear();

        emit("; === Функция: " + node->value + " ===");
        emit(fname + " PROC");
        emit("    PUSH BP", 1);
        emit("    MOV BP, SP", 1);

        int paramOffset = 4;
        for (size_t i = 0; i < node->children.size(); ++i) {
            TreeNode* child = node->children[i];
            if (child && child->type == NT_Id) {
                currentParams[child->value] = paramOffset;
                paramOffset += 2;
            }
            else break;
        }

        TreeNode* body = nullptr;
        for (auto* child : node->children) {
            if (child && (child->type == NT_Block || child->type == NT_StmtList)) {
                body = child;
                break;
            }
        }

        if (body) {
            generateNode(body);
        }

        string epilogueLabel = "_epilogue_" + fname;
        emit(epilogueLabel + ":", 0);
        emit("    POP BP", 1);
        emit("    RET", 1);
        emit(fname + " ENDP");
    }

    void generateReturn(TreeNode* node) {
        if (!node->children.empty()) {
            generateExpr(node->children[0]);
        }
        string epilogueLabel = "_epilogue_" + curFunc;
        emit("    JMP " + epilogueLabel, 1);
    }

    void generateBlock(TreeNode* node) {
        for (auto* c : node->children) generateNode(c);
    }

    void generateStmtList(TreeNode* node) {
        for (auto* c : node->children) generateNode(c);
    }

    void generatePrint(TreeNode* node) {
        emit("; print", 1);
        if (!node->children.empty()) {
            generateExpr(node->children[0]);
            emit("    PUSH AX", 1);
            emit("    CALL PrintNumber", 1);
            emit("    POP AX", 1);
        }
        emit("", 1);
    }

    void generateIfStmt(TreeNode* node) {
        string Lend = newLabel();
        string Lelse = node->children.size() >= 3 ? newLabel() : "";

        if (!node->children.empty()) {
            generateExpr(node->children[0]);
            emit("    CMP AX, 0", 1);
        }

        if (!Lelse.empty()) {
            emit("    JE " + Lelse, 1);
        }
        else {
            emit("    JE " + Lend, 1);
        }

        if (node->children.size() >= 2) {
            generateNode(node->children[1]);
        }

        if (!Lelse.empty()) {
            emit("    JMP " + Lend, 1);
            emit(Lelse + ":", 0);
            if (node->children.size() >= 3) {
                generateNode(node->children[2]);
            }
        }
        emit(Lend + ":", 0);
    }

    void generateAssign(TreeNode* node) {
        if (node->children.size() < 2) return;

        TreeNode* left = node->children[0];
        TreeNode* right = node->children[1];
        int line = node->line;

        if (left->type != NT_Id) {
            cerr << "[СЕМАНТИКА] ОШИБКА: левая часть присваивания должна быть переменной (строка " << line << ")\n";
            semanticError = true;
            return;
        }

        string vname = varName(left->value);

        if (declared.find(vname) == declared.end()) {
            if (currentParams.find(left->value) == currentParams.end()) {
                emitData(vname + " DW 0  ; variable");
                declared[vname] = true;

                if (!symTable->findSymbol(left->value)) {
                    symTable->addSymbol(left->value, "int", false, {}, line);
                }
            }
        }

        string leftType = "int";
        string rightType = symTable->checkExprType(right);
        if (rightType == "unknown") {
            cerr << "[СЕМАНТИКА] Предупреждение: неизвестный тип выражения (строка " << line << ")\n";
        }

        generateExpr(right);
        emit("    MOV [" + vname + "], AX", 1);
    }

    void generateExpr(TreeNode* node) {
        if (!node || semanticError) return;
        switch (node->type) {
        case NT_BinOp: generateBinOp(node); break;
        case NT_Id: generateId(node); break;
        case NT_Num: generateNum(node); break;
        case NT_FuncCall: generateFuncCall(node); break;
        case NT_Expr:
            if (!node->value.empty() && node->value == "input") {
                emit("    CALL ReadNumber", 1);
                break;
            }
            if (!node->children.empty()) {
                generateExpr(node->children[0]);
            }
            break;
        case NT_InputStmt:
            emit("    CALL ReadNumber", 1);
            break;
        default: for (auto* c : node->children) generateExpr(c); break;
        }
    }

    void generateBinOp(TreeNode* node) {
        if (node->children.size() < 2) return;
        string op = node->value;

        if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
            generateExpr(node->children[0]);
            emit("    PUSH AX", 1);
            generateExpr(node->children[1]);
            emit("    POP BX", 1);
            emit("    CMP BX, AX", 1);

            string Ltrue = newLabel(), Lend = newLabel();
            if (op == "==") emit("    JE " + Ltrue, 1);
            else if (op == "!=") emit("    JNE " + Ltrue, 1);
            else if (op == "<") emit("    JL " + Ltrue, 1);
            else if (op == ">") emit("    JG " + Ltrue, 1);
            else if (op == "<=") emit("    JLE " + Ltrue, 1);
            else if (op == ">=") emit("    JGE " + Ltrue, 1);

            emit("    XOR AX, AX", 1);
            emit("    JMP " + Lend, 1);
            emit(Ltrue + ":", 0);
            emit("    MOV AX, 1", 1);
            emit(Lend + ":", 0);
            return;
        }

        generateExpr(node->children[0]);
        emit("    PUSH AX", 1);
        generateExpr(node->children[1]);
        emit("    POP BX", 1);

        if (op == "+") emit("    ADD AX, BX", 1);
        else if (op == "-") {
            emit("    XCHG AX, BX", 1);
            emit("    SUB AX, BX", 1);
        }
        else if (op == "*") {
            emit("    IMUL BX", 1);
        }
        else if (op == "/") {
            emit("    XCHG AX, BX", 1);
            emit("    CWD", 1);
            emit("    IDIV BX", 1);
        }
        else if (op == "%") {
            emit("    XCHG AX, BX", 1);
            emit("    CWD", 1);
            emit("    IDIV BX", 1);
            emit("    MOV AX, DX", 1);
        }
    }

    void generateId(TreeNode* node) {
        string name = node->value;
        int line = node->line;

        auto* sym = symTable->findSymbol(name);
        if (!sym) {
            cerr << "[СЕМАНТИКА] ОШИБКА: переменная '" << name << "' не объявлена (строка " << line << ")\n";
            semanticError = true;
            return;
        }

        string vname = varName(name);

        auto paramIt = currentParams.find(name);
        if (paramIt != currentParams.end()) {
            emit("    MOV AX, [BP+" + to_string(paramIt->second) + "]", 1);
            return;
        }

        if (declared.find(vname) == declared.end()) {
            emitData(vname + " DW 0  ; variable");
            declared[vname] = true;
        }
        emit("    MOV AX, [" + vname + "]", 1);
    }

    void generateNum(TreeNode* node) {
        int val = 0;
        try { val = stoi(node->value); }
        catch (...) { val = 0; }
        emit("    MOV AX, " + to_string(val), 1);
    }

    void generateFuncCall(TreeNode* node) {
        string fname = varName(node->value);
        int line = node->line;

        auto* sym = symTable->findSymbol(fname);
        if (!sym || !sym->isFunction) {
            cerr << "[СЕМАНТИКА] ОШИБКА: функция '" << node->value << "' не объявлена (строка " << line << ")\n";
            semanticError = true;
            return;
        }

        emit("; call " + node->value, 1);

        if (!node->children.empty()) {
            generateExpr(node->children[0]);
            emit("    PUSH AX", 1);
        }

        emit("    CALL " + fname, 1);

        if (!node->children.empty()) {
            emit("    ADD SP, 2", 1);
        }
    }
};

class ExtendedPrecedenceParser {
    vector<Token> tokens;
    vector<pair<string, vector<string>>> rules;
    string startSymbol = "Prog";
    map<string, set<string>> First, Last;
    map<pair<string, string>, set<char>> precTable;
    set<vector<string>> LT, RT;
    set<string> terminals = { "#","id","num","def","if","else","begin","end","return","print","input",
        "+","-","*","/","%","=","+=","-=","*=","/=","%=","==","!=","<",">","<=",">=","(",")",":",",",";" };

    SymbolTable* symTable;
    TreeNode* syntaxTreeRoot;
    vector<TreeNode*> nodeStack;
    CodeGenerator* codeGen;

public:
    ExtendedPrecedenceParser(const vector<Token>& t)
        : tokens(t), symTable(new SymbolTable()), syntaxTreeRoot(nullptr), codeGen(new CodeGenerator(symTable)) {
        initRules(); initPrecedenceMatrix(); buildTriples();
    }
    ~ExtendedPrecedenceParser() { delete symTable; delete syntaxTreeRoot; delete codeGen; }

    CodeGenerator* getCodeGenerator() { return codeGen; }

    void initRules() {
        using S = vector<string>;
        rules = {
            {"Prog", {"#", "StmtList", "#"}}, {"StmtList", {"Stmt", ";", "StmtList"}}, {"StmtList", {"Stmt", ";"}},
            {"StmtList", {"ReturnStmt", ";"}}, {"StmtList", {"ReturnStmt", ";", "StmtList"}},
            {"StmtList", {"PrintStmt", ";"}}, {"StmtList", {"PrintStmt", ";", "StmtList"}},
            {"StmtList", {"InputStmt", ";"}}, {"StmtList", {"InputStmt", ";", "StmtList"}},
            {"StmtList", {"Assign", ";"}}, {"StmtList", {"Assign", ";", "StmtList"}},
            {"StmtList", {"IfStmt", ";"}}, {"StmtList", {"IfStmt", ";", "StmtList"}},
            {"StmtList", {"FuncDef", ";"}}, {"StmtList", {"FuncDef", ";", "StmtList"}},
            {"StmtList", {"def", "id", "(", "Expr", ")", ":", "Block", ";"}},
            {"StmtList", {"def", "id", "(", "Expr", ")", ":", "Block", ";", "StmtList"}},
            {"StmtList", {"def", "id", "(", ")", ":", "Block", ";"}},
            {"StmtList", {"def", "id", "(", ")", ":", "Block", ";", "StmtList"}},
            {"Stmt", {"Assign"}}, {"Stmt", {"PrintStmt"}}, {"Stmt", {"InputStmt"}},
            {"Stmt", {"ReturnStmt"}}, {"Stmt", {"FuncDef"}}, {"Stmt", {"IfStmt"}},
            {"Assign", {"id", "=", "Expr"}}, {"PrintStmt", {"print", "(", "Expr", ")"}},
            {"ReturnStmt", {"return", "Expr"}},
            {"FuncDef", {"def", "id", "(", "ExprList", ")", ":", "Block"}},
            {"FuncDef", {"def", "id", "(", "Expr", ")", ":", "Block"}},
            {"Block", {"begin", "StmtList", "end"}},
            {"IfStmt", {"if", "Expr", ":", "Block", "ElsePart"}}, {"IfStmt", {"if", "Expr", ":", "Block"}},
            {"ElsePart", {"else", "Block"}}, {"ExprList", {"Expr", ",", "ExprList"}}, {"ExprList", {"Expr"}},
            {"Expr", {"Expr", "+", "Expr"}}, {"Expr", {"Expr", "-", "Expr"}}, {"Expr", {"Expr", "*", "Expr"}},
            {"Expr", {"Expr", "/", "Expr"}}, {"Expr", {"Expr", "%", "Expr"}}, {"Expr", {"(", "Expr", ")"}},
            {"Expr", {"id"}}, {"Expr", {"num"}}, {"Expr", {"id", "(", "ExprList", ")"}},
            {"Expr", {"input", "(", ")"}}, {"Expr", {"id", "(", "Expr", ")"}}
        };
    }

    bool isTerminal(const string& s) const { return terminals.count(s) > 0; }

    void initPrecedenceMatrix() {
        auto add = [&](const string& a, const string& b, char rel) { precTable[{a, b}].insert(rel); };
        add("def", "id", '='); add("id", "(", '='); add("print", "(", '='); add("input", "(", '=');
        add("(", ")", '='); add(")", ":", '='); add(":", "begin", '<'); add("else", "begin", '<');
        for (auto& s : { "id","num","def","if","begin","return","print","input" }) {
            add("#", s, '<'); add(";", s, '<'); add(":", s, '<'); add("(", s, '<'); add("begin", s, '<'); add(",", s, '<');
        }
        for (auto& op : { "+","-","*","/","%","=" }) {
            add(op, "id", '<'); add(op, "num", '<'); add(op, "(", '<'); add(op, "input", '<');
        }
        for (auto& kw : { "return","if" }) { add(kw, "id", '<'); add(kw, "num", '<'); add(kw, "(", '<'); add(kw, "input", '<'); }
        for (auto& op : { "=","+=","-=","*=","/=","%=" }) { add(op, "id", '<'); add(op, "num", '<'); add(op, "(", '<'); add(op, "input", '<'); }
        for (auto& low : { "+","-" }) for (auto& high : { "*","/","%" }) add(low, high, '<');
        add("id", "(", '<');
        for (auto& e : { "id","num",")","end" }) { add(e, ")", '>'); add(e, ";", '>'); add(e, ",", '>'); add(e, ":", '>'); add(e, "end", '>'); add(e, "#", '>'); }
        add(";", "#", '>'); add(";", "end", '>'); add(")", ")", '>'); add(")", ";", '>'); add(")", ",", '>');
        for (auto& op : { "+","-","*","/","%" }) { add(")", op, '>'); add(")", "=", '>'); add(")", ":", '>'); add(")", "end", '>'); add(")", "else", '>'); }
        for (auto& op : { "+","-","*","/","%" }) {
            add("id", op, '>');
            add("num", op, '>');
            add(")", op, '>');
        }
        add("id", "=", '<');
        add("=", ";", '>');
        add("=", "end", '>');
        for (auto& high : { "*","/","%" }) for (auto& low : { "+","-" }) add(high, low, '>');
        for (auto& op : { "+","-","*","/","%" }) add(op, op, '>');
        add("end", ";", '>'); add("end", "else", '>'); add("else", "begin", '<');
        for (auto& kw : { "return","if","print","input" }) add(kw, ";", '>');
        add("return", "begin", '<'); add("if", "begin", '<');
        add("(", "(", '<'); for (auto& op : { "+","-","*","/","%","==","!=","<",">","<=",">=" }) add("(", op, '<');
        for (auto& op : { "+","-","*","/","%","==","!=","<",">","<=",">=" }) add(op, ")", '>');
        add("input", ")", '>'); add("begin", ";", '<'); add(":", ";", '<'); add("else", ";", '>'); add("begin", "end", '=');
        add("Expr", ")", '>'); add(";", ";", '<'); add("#", "def", '<'); add("#", ";", '>'); add("#", "#", '=');
    }

    void buildTriples() { LT.clear(); RT.clear(); RT.insert({ "(","Expr",")" }); RT.insert({ "id","(",")" }); }

    string getRightTerminal(const vector<string>& stack, int idx) {
        for (int i = idx; i >= 0; --i) if (isTerminal(stack[i])) return stack[i]; return "#";
    }
    string topTerminal(const vector<string>& stack) { return getRightTerminal(stack, stack.size() - 1); }
    string symbolOf(size_t idx) {
        if (idx >= tokens.size()) return "#";
        const Token& t = tokens[idx];
        if (t.cls == TK_ID) return "id"; if (t.cls == TK_NUM) return "num";
        if (t.cls == TK_KW || t.cls == TK_OP || t.cls == TK_SEP) return t.lex; return "#";
    }

    int findHandleStart(const vector<string>& stack) {
        vector<pair<int, string>> termStack;
        for (int i = 0; i < (int)stack.size(); ++i) if (isTerminal(stack[i])) termStack.push_back({ i, stack[i] });
        vector<int> candidates;
        for (int i = (int)termStack.size() - 1; i > 0; --i) {
            auto it = precTable.find({ termStack[i - 1].second, termStack[i].second });
            if (it != precTable.end() && it->second.count('<') && !it->second.count('='))
                candidates.push_back(termStack[i - 1].first + 1);
        }
        for (int h : candidates) { vector<string> handle(stack.begin() + h, stack.end()); if (!findRule(handle).empty()) return h; }
        return 1;
    }

    string findRule(const vector<string>& rhs) {
        for (auto& [lhs, r] : rules) {
            if (r.size() == rhs.size()) { bool ok = true; for (size_t i = 0; i < rhs.size(); ++i) if (r[i] != rhs[i]) { ok = false; break; } if (ok) return lhs; }
        }
        return "";
    }

    TreeNode* createTreeNode(const string& lhs, const vector<string>& handle, int handleStart) {
        vector<TreeNode*> children;
        for (size_t i = handleStart; i < nodeStack.size() && i < handleStart + handle.size(); ++i)
            if (i < nodeStack.size() && nodeStack[i]) children.push_back(nodeStack[i]);
        int line = (handleStart < (int)nodeStack.size() && nodeStack[handleStart]) ? nodeStack[handleStart]->line : 0;

        if (lhs == "Expr") {
            if (handle.size() == 1 && (handle[0] == "id" || handle[0] == "num")) {
                if (!children.empty() && children[0]) return children[0];
                return new TreeNode(handle[0] == "id" ? NT_Id : NT_Num, handle[0] == "id" ? "unknown" : "0", "int", line);
            }
            if (handle.size() == 3 && (handle[1] == "+" || handle[1] == "-" || handle[1] == "*" || handle[1] == "/" || handle[1] == "%")) {
                auto* op = new TreeNode(NT_BinOp, handle[1], "int", line);
                if (children.size() >= 2) { op->addChild(children[0]); op->addChild(children[1]); }
                return op;
            }
            if (handle.size() == 3 && handle[0] == "(" && handle[2] == ")") return children.empty() ? new TreeNode(NT_Expr, "", "int", line) : children[0];
            if (handle.size() == 4 && handle[0] == "id" && handle[1] == "(" && handle[3] == ")") {
                string fn = (!children.empty() && children[0] && children[0]->type == NT_Id) ? children[0]->value : "unknown";
                auto* call = new TreeNode(NT_FuncCall, fn, "int", line);
                if (children.size() >= 2) call->addChild(children[1]);
                return call;
            }
            if (handle.size() == 3 && (handle[1] == "==" || handle[1] == "!=" || handle[1] == "<" || handle[1] == ">" || handle[1] == "<=" || handle[1] == ">=")) {
                auto* cmp = new TreeNode(NT_BinOp, handle[1], "int", line);
                if (children.size() >= 2) { cmp->addChild(children[0]); cmp->addChild(children[1]); }
                return cmp;
            }
            if (handle.size() == 3 && handle[0] == "input" && handle[1] == "(" && handle[2] == ")") {
                return new TreeNode(NT_Expr, "input", "int", line);
            }
        }
        if (lhs == "ReturnStmt" && handle.size() == 2 && handle[0] == "return") {
            auto* ret = new TreeNode(NT_ReturnStmt, "return", "void", line);
            if (!children.empty()) { ret->addChild(children[0]); ret->dataType = symTable->checkExprType(children[0]); }
            return ret;
        }
        if (lhs == "PrintStmt" && handle.size() == 4 && handle[0] == "print") {
            auto* p = new TreeNode(NT_PrintStmt, "print", "void", line);
            if (!children.empty()) p->addChild(children[0]); return p;
        }
        if (lhs == "Block" && handle.size() == 3 && handle[0] == "begin" && handle[2] == "end") {
            auto* b = new TreeNode(NT_Block, "block", "void", line);
            for (auto* c : children) b->addChild(c); symTable->exitScope(); return b;
        }
        if (lhs == "FuncDef" && handle.size() == 7 && handle[0] == "def") {
            string fn = handle.size() > 1 ? handle[1] : "unknown";
            auto* f = new TreeNode(NT_FuncDef, fn, "void", line);
            if (children.size() >= 2) { f->addChild(children[0]); f->addChild(children[1]); }
            symTable->addSymbol(fn, "void", true, {}, line); return f;
        }
        if (lhs == "FuncDef" && handle.size() == 6 && handle[0] == "def") {
            string fn = handle.size() > 1 ? handle[1] : "unknown";
            auto* f = new TreeNode(NT_FuncDef, fn, "void", line);
            if (!children.empty()) f->addChild(children[0]);
            symTable->addSymbol(fn, "void", true, {}, line);
            return f;
        }
        if (lhs == "IfStmt" && handle.size() >= 3 && handle[0] == "if") {
            auto* ifs = new TreeNode(NT_IfStmt, "if", "void", line);
            if (!children.empty()) ifs->addChild(children[0]);
            if (children.size() >= 2) ifs->addChild(children[1]);
            if (children.size() >= 3) ifs->addChild(children[2]);
            return ifs;
        }

        if (lhs == "Assign" && handle.size() == 3 && handle[1] == "=") {
            auto* assign = new TreeNode(NT_Assign, "=", "void", line);
            if (children.size() >= 2) {
                assign->addChild(children[0]);
                assign->addChild(children[1]);
            }
            return assign;
        }

        if (lhs == "StmtList") {
            if (handle.size() == 8 && handle[0] == "def") {
                string fname = (children.size() > 0 && children[0]) ? children[0]->value : "unknown";
                TreeNode* paramExpr = (children.size() > 1) ? children[1] : nullptr;
                TreeNode* bodyBlock = (children.size() > 2) ? children[2] : nullptr;

                auto* f = new TreeNode(NT_FuncDef, fname, "void", line);
                if (paramExpr) f->addChild(paramExpr);
                if (bodyBlock) f->addChild(bodyBlock);
                symTable->addSymbol(fname, "void", true, {}, line);
                return f;
            }

            if (handle.size() == 9 && handle[0] == "def") {
                string fname = (children.size() > 0 && children[0]) ? children[0]->value : "unknown";
                TreeNode* paramExpr = (children.size() > 1) ? children[1] : nullptr;
                TreeNode* bodyBlock = (children.size() > 2) ? children[2] : nullptr;
                TreeNode* restStmts = (children.size() > 3) ? children[3] : nullptr;

                auto* f = new TreeNode(NT_FuncDef, fname, "void", line);
                if (paramExpr) f->addChild(paramExpr);
                if (bodyBlock) f->addChild(bodyBlock);
                symTable->addSymbol(fname, "void", true, {}, line);

                auto* sl = new TreeNode(NT_StmtList, "", "void", line);
                sl->addChild(f);
                if (restStmts) sl->addChild(restStmts);
                return sl;
            }

            if (handle.size() == 6 && handle[0] == "def") {
                string fname = (children.size() > 0 && children[0]) ? children[0]->value : "unknown";
                TreeNode* bodyBlock = (children.size() > 1) ? children[1] : nullptr;

                auto* f = new TreeNode(NT_FuncDef, fname, "void", line);
                if (bodyBlock) f->addChild(bodyBlock);
                symTable->addSymbol(fname, "void", true, {}, line);
                return f;
            }

            if (handle.size() == 7 && handle[0] == "def") {
                string fname = (children.size() > 0 && children[0]) ? children[0]->value : "unknown";
                TreeNode* bodyBlock = (children.size() > 1) ? children[1] : nullptr;
                TreeNode* restStmts = (children.size() > 2) ? children[2] : nullptr;

                auto* f = new TreeNode(NT_FuncDef, fname, "void", line);
                if (bodyBlock) f->addChild(bodyBlock);
                symTable->addSymbol(fname, "void", true, {}, line);

                auto* sl = new TreeNode(NT_StmtList, "", "void", line);
                sl->addChild(f);
                if (restStmts) sl->addChild(restStmts);
                return sl;
            }

            auto* sl = new TreeNode(NT_StmtList, "", "void", line);
            for (auto* ch : children) sl->addChild(ch);
            return sl;
        }
        if (lhs == "Prog") { auto* pr = new TreeNode(NT_Program, "main", "void", line); if (!children.empty()) pr->addChild(children[0]); return pr; }
        NodeType nt = (lhs == "FuncDef") ? NT_FuncDef : (lhs == "Block") ? NT_Block : (lhs == "StmtList") ? NT_StmtList : (lhs == "IfStmt") ? NT_IfStmt : NT_Expr;
        auto* n = new TreeNode(nt, lhs, "int", line); for (auto* c : children) n->addChild(c); return n;
    }

    void printTree() const {
        if (!syntaxTreeRoot) { cout << "\n[ДЕРЕВО] Не построено.\n"; return; }
        cout << "\n=== СИНТАКСИЧЕСКОЕ ДЕРЕВО ===\n"; syntaxTreeRoot->print(); cout << "=============================\n";
    }

    bool parse() {
        vector<string> stack; stack.push_back("#"); nodeStack.clear(); nodeStack.push_back(nullptr);
        size_t ip = 0; string cur = symbolOf(ip); int step = 0;
        while (!(stack.size() == 2 && stack[0] == "#" && stack[1] == startSymbol && cur == "#")) {
            string trm = topTerminal(stack); auto rels = precTable[{trm, cur}];
            cout << "[ШАГ " << ++step << "] Стек: "; for (auto& s : stack) cout << s << " ";
            cout << "| Верх.терм: " << trm << " | Вход: " << cur << " | Отнош: "; for (char r : rels) cout << r; cout << endl;
            if (rels.empty()) { cout << "ОШИБКА: нет отношения предшествования\n"; return false; }
            char rel = *rels.begin();
            if (rels.size() > 1) {
                if (rels.count('=') && rels.count('<')) { string nxt = symbolOf(ip + 1); rel = LT.count({ trm,cur,nxt }) ? '<' : '='; }
                else if (rels.count('=') && rels.count('>')) { string prev = getRightTerminal(stack, stack.size() - 2); rel = RT.count({ prev,trm,cur }) ? '>' : '='; }
            }
            cout << "   -> выбрано: " << rel << endl;
            if (rel == '<' || rel == '=') {
                stack.push_back(cur);
                if (cur == "id" || cur == "num") { string val = (ip < tokens.size()) ? tokens[ip].lex : cur; nodeStack.push_back(new TreeNode(cur == "id" ? NT_Id : NT_Num, val, "int", tokens[ip].pos)); }
                else nodeStack.push_back(nullptr);
                ip++; cur = symbolOf(ip);
                if (cur == "#" && stack.size() == 3 && stack[0] == "#" && stack[1] == "StmtList" && stack[2] == "#") {
                    TreeNode* sl = (nodeStack.size() >= 2) ? nodeStack[1] : nullptr;
                    TreeNode* pr = new TreeNode(NT_Program, "main", "void"); if (sl) pr->addChild(sl);
                    stack = { "#", startSymbol }; nodeStack = { nullptr, pr }; syntaxTreeRoot = pr; cur = symbolOf(ip);
                }
            }
            else if (rel == '>') {
                int hStart = findHandleStart(stack); vector<string> handle(stack.begin() + hStart, stack.end());
                cout << "   -> Handle: "; for (auto& s : handle) cout << s << " "; cout << endl;
                string lhs = findRule(handle); if (lhs.empty()) { cout << "ОШИБКА: не найдено правило для свёртки.\n"; return false; }
                TreeNode* newNode = createTreeNode(lhs, handle, hStart);
                stack.erase(stack.begin() + hStart, stack.end()); stack.push_back(lhs);
                nodeStack.erase(nodeStack.begin() + hStart, nodeStack.end()); nodeStack.push_back(newNode);
                cout << "   -> Свёртка в: " << lhs << "\n";
                if (lhs == startSymbol && nodeStack.size() >= 2) syntaxTreeRoot = nodeStack.back();
            }
            else { cout << "Внутренняя ошибка парсера\n"; return false; }
        }
        cout << "\nРАЗБОР ЗАВЕРШЁН УСПЕШНО!\n"; printTree();
        cout << "\n[КОД] Генерация объектного кода...\n";
        codeGen->printCode(syntaxTreeRoot);
        codeGen->saveToFile("output.asm", syntaxTreeRoot);
        return true;
    }
};

int main() {
    setlocale(LC_ALL, "ru");
    Lexer lexer;
    auto tokens = lexer.tokenize("test.txt");
    if (tokens.empty()) return 1;
    lexer.printTokens(tokens);
    ExtendedPrecedenceParser parser(tokens);
    cout << "\nСИНТАКСИЧЕСКИЙ АНАЛИЗ (расширенное предшествование):\n";
    bool success = parser.parse();
    if (success && parser.getCodeGenerator()->hasSemanticError()) {
        cerr << "\n[ОШИБКА] Обнаружены семантические ошибки. Компиляция прервана.\n";
        return 2;
    }
    return success ? 0 : 1;
}
#pragma once
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>
#include <algorithm>
#include <sstream>

// ---- Tokens ----
enum class TT {
    KEYWORD, IDENT, NUMBER, STRING, STAR, COMMA, SEMI,
    LPAREN, RPAREN, EQ, NEQ, LT, GT, LEQ, GEQ,
    DOT, EOF_T
};

struct Token {
    TT type;
    std::string value;
};

// ---- Condition node ----
enum class CondOp { EQ, NEQ, LT, GT, LEQ, GEQ };
enum class LogicOp { AND, OR };

struct SimpleCondition {
    std::string column;
    CondOp op;
    std::string value; // raw string
    bool valueIsColumn = false;
};

struct WhereCondition {
    std::vector<SimpleCondition> conditions;
    std::vector<LogicOp> ops; // between conditions
};

// ---- AST nodes ----
struct CreateDbStmt  { std::string dbName; bool ifNotExists = false; };
struct DropDbStmt    { std::string dbName; };
struct UseDatabaseStmt { std::string dbName; };
struct ShowTablesStmt  {};
struct ShowDbsStmt     {};

struct ColumnSpec {
    std::string name;
    std::string typeName;
    int typeSize = 255;
    bool primaryKey = false;
    bool notNull    = false;
    bool unique     = false;
    std::string fkTable, fkColumn;
};

struct CreateTableStmt {
    std::string tableName;
    bool ifNotExists = false;
    std::vector<ColumnSpec> columns;
};

struct DropTableStmt { std::string tableName; bool ifExists = false; };

struct SelectStmt {
    std::vector<std::string> projection; // "*" or col names
    std::string tableName;
    bool hasWhere = false;
    WhereCondition where;
};

struct InsertStmt {
    std::string tableName;
    std::vector<std::string> columns; // optional
    std::vector<std::vector<std::string>> valueSets;
};

struct Assignment { std::string column; std::string value; };

struct UpdateStmt {
    std::string tableName;
    std::vector<Assignment> assignments;
    bool hasWhere = false;
    WhereCondition where;
};

struct DeleteStmt {
    std::string tableName;
    bool hasWhere = false;
    WhereCondition where;
};

using Statement = std::variant<
    CreateDbStmt, DropDbStmt, UseDatabaseStmt,
    ShowTablesStmt, ShowDbsStmt,
    CreateTableStmt, DropTableStmt,
    SelectStmt, InsertStmt, UpdateStmt, DeleteStmt
>;

// ---- Lexer ----
class Lexer {
    std::string src;
    size_t pos = 0;

    static const std::vector<std::string> KEYWORDS;

    void skipWS() {
        while (pos < src.size() && isspace((unsigned char)src[pos])) pos++;
    }

    bool isKeyword(const std::string& s) {
        std::string u = s;
        for (auto& c : u) c = (char)toupper(c);
        return std::find(KEYWORDS.begin(), KEYWORDS.end(), u) != KEYWORDS.end();
    }

public:
    explicit Lexer(const std::string& input) : src(input) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (true) {
            skipWS();
            if (pos >= src.size()) { tokens.push_back({TT::EOF_T, ""}); break; }

            char c = src[pos];

            if (c == '-' && pos+1 < src.size() && src[pos+1] == '-') {
                // comment
                while (pos < src.size() && src[pos] != '\n') pos++;
                continue;
            }

            if (c == ';')  { tokens.push_back({TT::SEMI, ";"}); pos++; continue; }
            if (c == ',')  { tokens.push_back({TT::COMMA, ","}); pos++; continue; }
            if (c == '*')  { tokens.push_back({TT::STAR, "*"}); pos++; continue; }
            if (c == '(')  { tokens.push_back({TT::LPAREN, "("}); pos++; continue; }
            if (c == ')')  { tokens.push_back({TT::RPAREN, ")"}); pos++; continue; }
            if (c == '.')  { tokens.push_back({TT::DOT, "."}); pos++; continue; }
            if (c == '=' ) { tokens.push_back({TT::EQ, "="}); pos++; continue; }
            if (c == '!' && pos+1 < src.size() && src[pos+1] == '=') {
                tokens.push_back({TT::NEQ, "!="}); pos += 2; continue;
            }
            if (c == '<' && pos+1 < src.size() && src[pos+1] == '>') {
                tokens.push_back({TT::NEQ, "<>"}); pos += 2; continue;
            }
            if (c == '<' && pos+1 < src.size() && src[pos+1] == '=') {
                tokens.push_back({TT::LEQ, "<="}); pos += 2; continue;
            }
            if (c == '>' && pos+1 < src.size() && src[pos+1] == '=') {
                tokens.push_back({TT::GEQ, ">="}); pos += 2; continue;
            }
            if (c == '<') { tokens.push_back({TT::LT, "<"}); pos++; continue; }
            if (c == '>') { tokens.push_back({TT::GT, ">"}); pos++; continue; }

            // String literal
            if (c == '\'' || c == '"') {
                char quote = c; pos++;
                std::string s;
                while (pos < src.size() && src[pos] != quote) {
                    if (src[pos] == '\\' && pos+1 < src.size()) {
                        pos++;
                        char esc = src[pos++];
                        if (esc == 'n') s += '\n';
                        else if (esc == 't') s += '\t';
                        else s += esc;
                    } else s += src[pos++];
                }
                if (pos < src.size()) pos++; // closing quote
                tokens.push_back({TT::STRING, s}); continue;
            }

            // Number
            if (isdigit(c) || (c == '-' && pos+1 < src.size() && isdigit(src[pos+1]))) {
                std::string n;
                if (c == '-') { n += c; pos++; }
                while (pos < src.size() && (isdigit(src[pos]) || src[pos] == '.')) n += src[pos++];
                tokens.push_back({TT::NUMBER, n}); continue;
            }

            // Identifier or keyword
            if (isalpha(c) || c == '_') {
                std::string id;
                while (pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')) id += src[pos++];
                TT tt = isKeyword(id) ? TT::KEYWORD : TT::IDENT;
                tokens.push_back({tt, id}); continue;
            }

            // Skip unknown
            pos++;
        }
        return tokens;
    }
};

inline const std::vector<std::string> Lexer::KEYWORDS = {
    "SELECT","FROM","WHERE","INSERT","INTO","VALUES","UPDATE","SET",
    "DELETE","CREATE","DROP","TABLE","DATABASE","IF","NOT","EXISTS",
    "PRIMARY","KEY","FOREIGN","REFERENCES","NULL","AND","OR","IN",
    "USE","SHOW","TABLES","DATABASES","VARCHAR","INTEGER","INT","FLOAT",
    "DOUBLE","REAL","BOOLEAN","BOOL","TEXT","STRING","UNIQUE","CHECK",
    "DEFAULT","ALTER","ADD","COLUMN","TRUE","FALSE","IS"
};

// ---- Parser ----
class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;

    Token& cur() { return tokens[pos]; }

    std::string up(const std::string& s) {
        std::string r = s; for (auto& c : r) c = (char)toupper(c); return r;
    }

    bool isKW(const std::string& kw) {
        return (cur().type == TT::KEYWORD || cur().type == TT::IDENT) && up(cur().value) == up(kw);
    }

    Token consume() { return tokens[pos++]; }

    Token expect(TT t, const std::string& msg = "") {
        if (cur().type != t) throw std::runtime_error(
            msg.empty() ? "Unexpected token: " + cur().value : msg);
        return consume();
    }

    Token expectKW(const std::string& kw) {
        if (!isKW(kw)) throw std::runtime_error("Expected keyword " + kw + ", got: " + cur().value);
        return consume();
    }

    bool tryConsume(TT t) {
        if (cur().type == t) { consume(); return true; } return false;
    }

    bool tryConsumeKW(const std::string& kw) {
        if (isKW(kw)) { consume(); return true; } return false;
    }

    std::string parseIdent() {
        if (cur().type == TT::IDENT || cur().type == TT::KEYWORD)
            return consume().value;
        throw std::runtime_error("Expected identifier, got: " + cur().value);
    }

    std::string parseValue() {
        if (cur().type == TT::STRING) return consume().value;
        if (cur().type == TT::NUMBER) return consume().value;
        if (isKW("NULL"))  { consume(); return "NULL"; }
        if (isKW("TRUE"))  { consume(); return "true"; }
        if (isKW("FALSE")) { consume(); return "false"; }
        // Could be identifier (column ref)
        return parseIdent();
    }

    CondOp parseOp() {
        switch (cur().type) {
            case TT::EQ:  consume(); return CondOp::EQ;
            case TT::NEQ: consume(); return CondOp::NEQ;
            case TT::LT:  consume(); return CondOp::LT;
            case TT::GT:  consume(); return CondOp::GT;
            case TT::LEQ: consume(); return CondOp::LEQ;
            case TT::GEQ: consume(); return CondOp::GEQ;
            default: throw std::runtime_error("Expected comparison operator");
        }
    }

    SimpleCondition parseSimpleCondition() {
        SimpleCondition c;
        c.column = parseIdent();
        if (isKW("IS")) {
            consume();
            bool neg = tryConsumeKW("NOT");
            expectKW("NULL");
            c.op = neg ? CondOp::NEQ : CondOp::EQ;
            c.value = "NULL";
            return c;
        }
        c.op = parseOp();
        // Could be literal or column
        if (cur().type == TT::STRING) {
            c.value = consume().value;
        } else if (cur().type == TT::NUMBER) {
            c.value = consume().value;
        } else if (isKW("NULL"))  { consume(); c.value = "NULL"; }
        else if (isKW("TRUE"))    { consume(); c.value = "true"; }
        else if (isKW("FALSE"))   { consume(); c.value = "false"; }
        else { c.value = parseIdent(); c.valueIsColumn = true; }
        return c;
    }

    WhereCondition parseCondition() {
        WhereCondition cond;
        cond.conditions.push_back(parseSimpleCondition());
        while (isKW("AND") || isKW("OR")) {
            cond.ops.push_back(isKW("AND") ? LogicOp::AND : LogicOp::OR);
            consume();
            cond.conditions.push_back(parseSimpleCondition());
        }
        return cond;
    }

    ColumnSpec parseColumnSpec() {
        ColumnSpec cs;
        cs.name = parseIdent();
        // Type
        cs.typeName = up(parseIdent());
        if (cs.typeName == "VARCHAR" || cs.typeName == "CHAR") {
            if (tryConsume(TT::LPAREN)) {
                cs.typeSize = std::stoi(consume().value);
                expect(TT::RPAREN);
            }
        }
        // Constraints
        while (cur().type != TT::COMMA && cur().type != TT::RPAREN &&
               cur().type != TT::EOF_T && cur().type != TT::SEMI) {
            if (isKW("PRIMARY")) {
                consume(); expectKW("KEY"); cs.primaryKey = true; cs.notNull = true;
            } else if (isKW("NOT")) {
                consume(); expectKW("NULL"); cs.notNull = true;
            } else if (isKW("UNIQUE")) {
                consume(); cs.unique = true;
            } else if (isKW("DEFAULT")) {
                consume(); parseValue(); // ignore default for MVP
            } else if (isKW("CHECK")) {
                consume(); expect(TT::LPAREN);
                int depth = 1;
                while (depth > 0 && cur().type != TT::EOF_T) {
                    if (cur().type == TT::LPAREN) depth++;
                    else if (cur().type == TT::RPAREN) depth--;
                    if (depth > 0) consume(); else consume();
                }
            } else if (isKW("REFERENCES")) {
                consume();
                cs.fkTable = parseIdent();
                if (tryConsume(TT::LPAREN)) {
                    cs.fkColumn = parseIdent();
                    expect(TT::RPAREN);
                }
            } else break;
        }
        return cs;
    }

public:
    explicit Parser(const std::string& input) {
        Lexer lex(input);
        tokens = lex.tokenize();
    }

    Statement parse() {
        if (cur().type == TT::EOF_T) throw std::runtime_error("Empty statement");

        if (isKW("CREATE")) {
            consume();
            if (isKW("DATABASE")) {
                consume();
                CreateDbStmt s;
                if (isKW("IF")) { consume(); expectKW("NOT"); expectKW("EXISTS"); s.ifNotExists = true; }
                s.dbName = parseIdent();
                return s;
            }
            if (isKW("TABLE")) {
                consume();
                CreateTableStmt s;
                if (isKW("IF")) { consume(); expectKW("NOT"); expectKW("EXISTS"); s.ifNotExists = true; }
                s.tableName = parseIdent();
                expect(TT::LPAREN);
                while (cur().type != TT::RPAREN && cur().type != TT::EOF_T) {
                    // FOREIGN KEY constraint at table level
                    if (isKW("FOREIGN")) {
                        consume(); expectKW("KEY");
                        expect(TT::LPAREN); std::string col = parseIdent(); expect(TT::RPAREN);
                        expectKW("REFERENCES");
                        std::string fkT = parseIdent();
                        std::string fkC;
                        if (tryConsume(TT::LPAREN)) { fkC = parseIdent(); expect(TT::RPAREN); }
                        for (auto& c : s.columns)
                            if (c.name == col) { c.fkTable = fkT; c.fkColumn = fkC; break; }
                    } else if (isKW("PRIMARY")) {
                        consume(); expectKW("KEY");
                        expect(TT::LPAREN); std::string col = parseIdent(); expect(TT::RPAREN);
                        for (auto& c : s.columns) if (c.name == col) { c.primaryKey = true; c.notNull = true; break; }
                    } else if (isKW("UNIQUE")) {
                        consume(); expect(TT::LPAREN); std::string col = parseIdent(); expect(TT::RPAREN);
                        for (auto& c : s.columns) if (c.name == col) { c.unique = true; break; }
                    } else {
                        s.columns.push_back(parseColumnSpec());
                    }
                    if (!tryConsume(TT::COMMA)) break;
                }
                expect(TT::RPAREN);
                return s;
            }
            throw std::runtime_error("Expected DATABASE or TABLE after CREATE");
        }

        if (isKW("DROP")) {
            consume();
            if (isKW("DATABASE")) {
                consume(); DropDbStmt s; s.dbName = parseIdent(); return s;
            }
            if (isKW("TABLE")) {
                consume(); DropTableStmt s;
                if (isKW("IF")) { consume(); expectKW("EXISTS"); s.ifExists = true; }
                s.tableName = parseIdent(); return s;
            }
            throw std::runtime_error("Expected DATABASE or TABLE after DROP");
        }

        if (isKW("USE")) {
            consume(); UseDatabaseStmt s; s.dbName = parseIdent(); return s;
        }

        if (isKW("SHOW")) {
            consume();
            if (isKW("TABLES"))    { consume(); return ShowTablesStmt{}; }
            if (isKW("DATABASES")) { consume(); return ShowDbsStmt{}; }
            throw std::runtime_error("Expected TABLES or DATABASES after SHOW");
        }

        if (isKW("SELECT")) {
            consume();
            SelectStmt s;
            if (cur().type == TT::STAR) { consume(); s.projection.push_back("*"); }
            else {
                s.projection.push_back(parseIdent());
                while (tryConsume(TT::COMMA)) s.projection.push_back(parseIdent());
            }
            expectKW("FROM"); s.tableName = parseIdent();
            if (isKW("WHERE")) { consume(); s.hasWhere = true; s.where = parseCondition(); }
            return s;
        }

        if (isKW("INSERT")) {
            consume(); expectKW("INTO");
            InsertStmt s; s.tableName = parseIdent();
            if (tryConsume(TT::LPAREN)) {
                s.columns.push_back(parseIdent());
                while (tryConsume(TT::COMMA)) s.columns.push_back(parseIdent());
                expect(TT::RPAREN);
            }
            expectKW("VALUES");
            do {
                expect(TT::LPAREN);
                std::vector<std::string> vals;
                vals.push_back(parseValue());
                while (tryConsume(TT::COMMA)) vals.push_back(parseValue());
                expect(TT::RPAREN);
                s.valueSets.push_back(vals);
            } while (tryConsume(TT::COMMA));
            return s;
        }

        if (isKW("UPDATE")) {
            consume(); UpdateStmt s; s.tableName = parseIdent();
            expectKW("SET");
            do {
                Assignment a; a.column = parseIdent(); expect(TT::EQ); a.value = parseValue();
                s.assignments.push_back(a);
            } while (tryConsume(TT::COMMA));
            if (isKW("WHERE")) { consume(); s.hasWhere = true; s.where = parseCondition(); }
            return s;
        }

        if (isKW("DELETE")) {
            consume(); expectKW("FROM"); DeleteStmt s; s.tableName = parseIdent();
            if (isKW("WHERE")) { consume(); s.hasWhere = true; s.where = parseCondition(); }
            return s;
        }

        throw std::runtime_error("Unknown SQL statement starting with: " + cur().value);
    }
};

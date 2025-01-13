#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <stack>
#include <cmath>
#include <math.h>

using VariableName = std::string;
using FunctionName = std::string;
using Line = std::string;

namespace Clot {

    std::map<VariableName, double> DOUBLE;
    std::map<VariableName, std::string> STRING;

    struct Function {
        std::vector<VariableName> parameters;
        std::vector<Line> body;
    };

    std::map<FunctionName, Function> functions;

    enum class TokenType {
        Identifier,
        Number,
        String,
        Assignment,
        Plus,
        Minus,
        Multiply,
        Divide,
        Modulo,
        And,
        Or,
        Not,
        Equal,
        NotEqual,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        Comma,
        LeftParen,
        RightParen,
        Colon,
        Func,
        EndFunc,
        Print,
        Comment,
        Unknown
    };

    struct Token {
        TokenType type;
        std::string value;
    };

    using Tokens = std::vector<Token>;

    class Tokenizer {
    public:
        static Tokens tokenize(const Line& line) {
            Tokens tokens;
            std::string currentToken;
            bool insideQuotes = false;

            for (char currentChar : line) {
                if (currentChar == '"') {
                    insideQuotes = !insideQuotes;
                    currentToken += currentChar;
                }
                else if (insideQuotes) {
                    currentToken += currentChar;
                }
                else if (isSpecialChar(currentChar)) {
                    if (!currentToken.empty()) {
                        tokens.push_back(createToken(currentToken));
                        currentToken.clear();
                    }
                    tokens.push_back(createToken(std::string(1, currentChar)));
                }
                else if (std::isspace(currentChar)) {
                    if (!currentToken.empty()) {
                        tokens.push_back(createToken(currentToken));
                        currentToken.clear();
                    }
                }
                else {
                    currentToken += currentChar;
                }
            }

            if (!currentToken.empty()) {
                tokens.push_back(createToken(currentToken));
            }

            return tokens;
        }

    private:
        static bool isSpecialChar(char c) {
            return c == '(' || c == ')' || c == ',' || c == '+' || c == '=' || c == ':';
        }

        static Token createToken(const std::string& str) {
            if (str == "=") return { TokenType::Assignment, str };
            if (str == "+") return { TokenType::Plus, str };
			if (str == "-") return { TokenType::Minus, str };
            if (str == "*") return { TokenType::Multiply, str };
			if (str == "/") return { TokenType::Divide, str };
			if (str == "%") return { TokenType::Modulo, str };
			if (str == "&&") return { TokenType::And, str };
			if (str == "||") return { TokenType::Or, str };
			if (str == "!") return { TokenType::Not, str };
			if (str == "==") return { TokenType::Equal, str };
			if (str == "!=") return { TokenType::NotEqual, str };
			if (str == "<") return { TokenType::Less, str };
			if (str == "<=") return { TokenType::LessEqual, str };
			if (str == ">") return { TokenType::Greater, str };
			if (str == ">=") return { TokenType::GreaterEqual, str };
            if (str == ",") return { TokenType::Comma, str };
            if (str == "(") return { TokenType::LeftParen, str };
            if (str == ")") return { TokenType::RightParen, str };
            if (str == ":") return { TokenType::Colon, str };
            if (str == "func") return { TokenType::Func, str };
            if (str == "endfunc") return { TokenType::EndFunc, str };
            if (str == "print") return { TokenType::Print, str };
			if (str == "//") return { TokenType::Comment, str };
            if (isNumber(str)) return { TokenType::Number, str };
            if (isString(str)) return { TokenType::String, str };
            return { TokenType::Identifier, str };
        }

        static bool isNumber(const std::string& s) {
            return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c) && c != '.'; }) == s.end();
        }

        static bool isString(const std::string& s) {
            return s.size() >= 2 && s.front() == '"' && s.back() == '"';
        }
    };

    class ExpressionEvaluator {
    public:
        static double evaluate(const Tokens& tokens) {
            std::stack<double> values;
            std::stack<TokenType> ops;

            for (const auto& token : tokens) {
                if (token.type == TokenType::Number) {
                    values.push(std::stod(token.value));
                }
                else if (token.type == TokenType::Identifier) {
                    if (DOUBLE.count(token.value)) {
                        values.push(DOUBLE[token.value]);
                    }
                    else {
                        throw std::runtime_error("Variable no definida: " + token.value);
                    }
                }
                else if (isOperator(token.type)) {
                    while (!ops.empty() && precedence(ops.top()) >= precedence(token.type)) {
                        double val2 = values.top(); values.pop();
                        double val1 = values.top(); values.pop();
                        TokenType op = ops.top(); ops.pop();
                        values.push(applyOperator(val1, val2, op));
                    }
                    ops.push(token.type);
                }
            }

            while (!ops.empty()) {
                double val2 = values.top(); values.pop();
                double val1 = values.top(); values.pop();
                TokenType op = ops.top(); ops.pop();
                values.push(applyOperator(val1, val2, op));
            }

            return values.top();
        }

    private:
        static bool isOperator(TokenType type) {
            return type == TokenType::Plus || type == TokenType::Minus || type == TokenType::Multiply ||
                type == TokenType::Divide || type == TokenType::Modulo || type == TokenType::And ||
                type == TokenType::Or || type == TokenType::Not || type == TokenType::Equal ||
                type == TokenType::NotEqual || type == TokenType::Less || type == TokenType::LessEqual ||
                type == TokenType::Greater || type == TokenType::GreaterEqual;
        }

        static int precedence(TokenType type) {
            if (type == TokenType::Multiply || type == TokenType::Divide || type == TokenType::Modulo) return 2;
            if (type == TokenType::Plus || type == TokenType::Minus) return 1;
            if (type == TokenType::And || type == TokenType::Or) return 0;
            return -1;
        }

        static double applyOperator(double val1, double val2, TokenType op) {
            switch (op) {
            case TokenType::Plus: return val1 + val2;
            case TokenType::Minus: return val1 - val2;
            case TokenType::Multiply: return val1 * val2;
            case TokenType::Divide: return val1 / val2;
            case TokenType::Modulo: return std::fmod(val1, val2);
            case TokenType::And: return val1 && val2;
            case TokenType::Or: return val1 || val2;
            case TokenType::Equal: return val1 == val2;
            case TokenType::NotEqual: return val1 != val2;
            case TokenType::Less: return val1 < val2;
            case TokenType::LessEqual: return val1 <= val2;
            case TokenType::Greater: return val1 > val2;
            case TokenType::GreaterEqual: return val1 >= val2;
            default: throw std::runtime_error("Operador no soportado.");
            }
        }
    };



    class VariableAssignment {
    public:
        static void assign(const Tokens& tokens) {
            if (tokens.size() < 3 || tokens[1].type != TokenType::Assignment) {
                throw std::runtime_error("Error en asignación: formato inválido.");
            }

            const VariableName& variableName = tokens[0].value;
            Tokens expressionTokens(tokens.begin() + 2, tokens.end());

            try {
                double value = ExpressionEvaluator::evaluate(expressionTokens);
                DOUBLE[variableName] = value;
                std::cout << "Variable '" << variableName << "' asignada con valor: " << value << std::endl;
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Error al evaluar expresión en asignación: " + std::string(e.what()));
            }
        }
    };


    class PrintStatement {
    public:
        static void print(const Tokens& tokens) {
            if (tokens.size() < 4 || tokens[1].type != TokenType::LeftParen || tokens.back().type != TokenType::RightParen) {
                throw std::runtime_error("Error en print: formato inválido.");
            }

            std::string message;
            for (size_t i = 2; i < tokens.size() - 1; ++i) {
                if (tokens[i].type == TokenType::Plus) {
                    continue;
                }
                if (tokens[i].type == TokenType::Identifier && DOUBLE.count(tokens[i].value)) {
                    message += std::to_string(DOUBLE[tokens[i].value]);
                }
                else {
                    message += tokens[i].value;
                }
            }

            message.erase(std::remove(message.begin(), message.end(), '\"'), message.end());
            std::cout << message << std::endl;
        }
    };

    class FunctionDeclaration {
    public:
        static void declare(const Tokens& tokens, std::ifstream& file) {
            if (tokens.size() < 4 || tokens[0].type != TokenType::Func || tokens[2].type != TokenType::LeftParen) {
                throw std::runtime_error("Error en la declaración de función: formato inválido.");
            }

            FunctionName functionName = tokens[1].value;
            std::vector<VariableName> parameters;
            size_t i = 3;
            for (; i < tokens.size(); ++i) {
                if (tokens[i].type == TokenType::RightParen) {
                    ++i;
                    break;
                }
                if (tokens[i].type != TokenType::Comma) {
                    parameters.push_back(tokens[i].value);
                }
            }

            if (i >= tokens.size() || tokens[i].type != TokenType::Colon) {
                throw std::runtime_error("Error en la declaración de función: falta el delimitador ':'");
            }

            std::vector<Line> body;
            Line line;
            bool endfuncFound = false;

            while (std::getline(file, line)) {
                if (line == "endfunc") {
                    endfuncFound = true;
                    break;
                }
                if (!line.empty() && line[0] != '\t') {
                    throw std::runtime_error("Error en la declaración de función: 'endfunc'");
                }
                body.push_back(line);
            }

            if (!endfuncFound) {
                throw std::runtime_error("Error en la declaración de función: falta 'endfunc'");
            }

            functions[functionName] = { parameters, body };
            std::cout << "Función '" << functionName << "' declarada con " << parameters.size() << " parámetro(s)." << std::endl;
        }
    };

    class FunctionExecution {
    public:
        static void execute(const Tokens& tokens) {
            if (tokens.empty()) {
                throw std::runtime_error("Error: nombre de función vacío.");
            }

            const FunctionName& functionName = tokens[0].value;
            if (!functions.count(functionName)) {
                throw std::runtime_error("Función no encontrada: " + functionName);
            }

            Function& function = functions[functionName];
            std::vector<double> arguments;

            for (size_t i = 2; i < tokens.size(); ++i) {
                if (tokens[i].type == TokenType::Comma || tokens[i].type == TokenType::LeftParen || tokens[i].type == TokenType::RightParen) continue;

                try {
                    if (tokens[i].type == TokenType::Identifier && DOUBLE.count(tokens[i].value)) {
                        arguments.push_back(DOUBLE[tokens[i].value]);
                    }
                    else {
                        arguments.push_back(std::stod(tokens[i].value));
                    }
                }
                catch (const std::exception& e) {
                    throw std::runtime_error("Error al convertir argumento '" + tokens[i].value + "': " + e.what());
                }
            }

            if (arguments.size() != function.parameters.size()) {
                throw std::runtime_error("Número incorrecto de argumentos para la función " + functionName +
                    ". Esperados: " + std::to_string(function.parameters.size()) +
                    ", Recibidos: " + std::to_string(arguments.size()));
            }

            std::map<VariableName, double> previousContext = DOUBLE;

            for (size_t i = 0; i < function.parameters.size(); ++i) {
                DOUBLE[function.parameters[i]] = arguments[i];
            }

            for (const Line& line : function.body) {
                Tokens lineTokens = Tokenizer::tokenize(line);
                if (lineTokens.empty()) continue;

                if (lineTokens.size() > 1 && lineTokens[1].type == TokenType::Assignment) {
                    VariableAssignment::assign(lineTokens);
                }
                else if (lineTokens[0].type == TokenType::Print) {
                    PrintStatement::print(lineTokens);
                }
                else if (functions.count(lineTokens[0].value)) {
                    execute(lineTokens);
                }
                else {
                    double result = ExpressionEvaluator::evaluate(lineTokens);
                    std::cout << "Resultado de la expresión: " << result << std::endl;
                }
            }

            DOUBLE = previousContext;
        }
    };


    static void Interpret(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("No se pudo abrir el archivo: " + filename);
        }

        Line line;
        while (std::getline(file, line)) {
            Tokens tokens = Tokenizer::tokenize(line);
            if (tokens.empty()) continue;

            try {
                if (tokens.size() > 1 && tokens[1].type == TokenType::Assignment) {
                    VariableAssignment::assign(tokens);
                }
                else if (tokens[0].type == TokenType::Print) {
                    PrintStatement::print(tokens);
                }
                else if (tokens[0].type == TokenType::Func) {
                    FunctionDeclaration::declare(tokens, file);
                }
                else if (tokens[0].type == TokenType::EndFunc) {
                    // Ignorar endfunc en el nivel superior
                    continue;
                }
                else if (functions.count(tokens[0].value)) {
                    FunctionExecution::execute(tokens);
                }
                else {
                    throw std::runtime_error("Instrucción no reconocida: " + line);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }

} // namespace Clot

int main() {
    try {
        Clot::Interpret("test.clot");
    }
    catch (const std::exception& e) {
        std::cerr << "Error fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

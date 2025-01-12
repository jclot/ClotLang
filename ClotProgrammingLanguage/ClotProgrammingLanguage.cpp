#include <iostream>c
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

using VariableName = std::string;
using FunctionName = std::string;
using Line = std::string;
using Token = std::string;
using Tokens = std::vector<Token>;

namespace Clot {

	std::map<VariableName, double> DOUBLE;
	std::map<Token, std::string> STRING;

	struct Function {
		std::vector<VariableName> parameters;
		std::vector<Line> body;
	};

	std::map<FunctionName, Function> functions;

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
						tokens.push_back(currentToken);
						currentToken.clear();
					}
					tokens.push_back(std::string(1, currentChar));
				}
				else if (std::isspace(currentChar)) {
					if (!currentToken.empty()) {
						tokens.push_back(currentToken);
						currentToken.clear();
					}
				}
				else {
					currentToken += currentChar;
				}
			}

			if (!currentToken.empty()) {
				tokens.push_back(currentToken);
			}

			return tokens;
		}

	private:
		static bool isSpecialChar(char c) {
			return c == '(' || c == ')' || c == ',' || c == '+' || c == '=';
		}
	};

	class VariableAssignment {
	public:
		static void assign(const Tokens& tokens) {
			if (tokens.size() != 3 || tokens[1] != "=") {
				throw std::runtime_error("Error en asignación: formato inválido.");
			}

			const VariableName& variableName = tokens[0];
			const Token& valueToken = tokens[2];

			try {
				if (isNumber(valueToken)) {
					double valueDou = std::stod(valueToken);
					DOUBLE[variableName] = valueDou;
					std::cout << "Variable '" << variableName << "' asignada con valor: " << valueDou << std::endl;
				}
				else {
					std::string valueStr = valueToken;
					if (valueStr.size() < 2 || valueStr.front() != '"' || valueStr.back() != '"') {
						throw std::runtime_error("Tipo STRING: se esperaba un string entre comillas.");
					}

					if (std::count(valueStr.begin(), valueStr.end(), '"') > 2) {
						throw std::runtime_error("Tipo STRING: se esperaba un string entre comillas.");
					}

					valueStr = valueStr.substr(1, valueStr.size() - 2);
					STRING[variableName] = valueStr;
					std::cout << "Variable '" << variableName << "' asignada con valor: " << valueStr << std::endl;
				}
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Error al convertir valor en asignacion: " + std::string(e.what()));
			}
		}

	private:
		static bool isNumber(const std::string& s) {
			return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c) && c != '.'; }) == s.end();
		}
	};

	class PrintStatement {
	public:
		static void print(const Tokens& tokens) {
			if (tokens.size() < 4 || tokens[1] != "(" || tokens.back() != ")") {
				throw std::runtime_error("Error en print: formato inválido.");
			}

			std::string message;
			for (size_t i = 2; i < tokens.size() - 1; ++i) {
				if (tokens[i] == "+") {
					continue;
				}
				if (DOUBLE.count(tokens[i])) {
					message += std::to_string(DOUBLE[tokens[i]]);
				}
				else {
					message += tokens[i];
				}
			}

			message.erase(std::remove(message.begin(), message.end(), '\"'), message.end());
			std::cout << message << std::endl;
		}
	};

	class FunctionDeclaration {
	public:
		static void declare(const Tokens& tokens, std::ifstream& file) {
			if (tokens.size() < 4 || tokens[0] != "func" || tokens[2] != "(") {
				throw std::runtime_error("Error en la declaración de función: formato inválido.");
			}

			FunctionName functionName = tokens[1];
			std::vector<VariableName> parameters;
			size_t i = 3;
			for (; i < tokens.size(); ++i) {
				if (tokens[i] == ")") {
					++i;
					break;
				}
				if (tokens[i] != ",") {
					parameters.push_back(tokens[i]);
				}
			}

			if (i >= tokens.size() || tokens[i] != ":") {
				throw std::runtime_error("Error en la declaración de función: falta el delimitador ':'");
			}

			std::vector<Line> body;
			Line line;
			while (std::getline(file, line) && line != "endfunc") {
				body.push_back(line);
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

			const FunctionName& functionName = tokens[0];
			if (!functions.count(functionName)) {
				throw std::runtime_error("Función no encontrada: " + functionName);
			}

			Function& function = functions[functionName];
			std::vector<double> arguments;

			for (size_t i = 2; i < tokens.size(); ++i) {
				if (tokens[i] == "," || tokens[i] == "(" || tokens[i] == ")") continue;

				try {
					if (DOUBLE.count(tokens[i])) {
						arguments.push_back(DOUBLE[tokens[i]]);
					}
					else {
						arguments.push_back(std::stod(tokens[i]));
					}
				}
				catch (const std::exception& e) {
					throw std::runtime_error("Error al convertir argumento '" + tokens[i] + "': " + e.what());
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

				if (lineTokens.size() > 1 && lineTokens[1] == "=") {
					VariableAssignment::assign(lineTokens);
				}
				else if (lineTokens[0] == "print") {
					PrintStatement::print(lineTokens);
				}
				else if (functions.count(lineTokens[0])) {
					execute(lineTokens);
				}
				else {
					throw std::runtime_error("Instrucción no reconocida en la función: " + line);
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
				if (tokens.size() > 1 && tokens[1] == "=") {
					VariableAssignment::assign(tokens);
				}
				else if (tokens[0] == "print") {
					PrintStatement::print(tokens);
				}
				else if (tokens[0] == "func") {
					FunctionDeclaration::declare(tokens, file);
				}
				else if (functions.count(tokens[0])) {
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

#include "clot/runtime/i18n.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <string>
#include <utility>

namespace clot::runtime {

namespace {

std::atomic<Language> g_language(Language::Spanish);

bool StartsWith(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

void ReplaceAll(std::string* text, const std::string& from, const std::string& to) {
    if (text == nullptr || from.empty()) {
        return;
    }

    std::size_t index = 0;
    while ((index = text->find(from, index)) != std::string::npos) {
        text->replace(index, from.size(), to);
        index += to.size();
    }
}

std::string TranslateSpanishToEnglish(const std::string& text) {
    static constexpr std::pair<const char*, const char*> kPrefixRules[] = {
        {"Falta valor para --mode.", "Missing value for --mode."},
        {"Falta valor para --emit.", "Missing value for --emit."},
        {"Falta valor para --output.", "Missing value for --output."},
        {"Falta valor para --target.", "Missing value for --target."},
        {"Falta valor para --lang.", "Missing value for --lang."},
        {"Falta valor para --runtime-bridge.", "Missing value for --runtime-bridge."},
        {"Modo invalido: ", "Invalid mode: "},
        {"Emit invalido: ", "Invalid emit kind: "},
        {"Runtime bridge invalido. Use static o external.", "Invalid runtime bridge. Use static or external."},
        {"Idioma invalido. Use es o en.", "Invalid language. Use es or en."},
        {"Opcion desconocida: ", "Unknown option: "},
        {"Se recibieron multiples archivos de entrada.", "Multiple input files were provided."},
        {"No se encontro archivo .clot de entrada.", "No input .clot file was found."},
        {"Falta ')' al cerrar llamada de funcion.", "Missing ')' to close function call."},
        {"Falta ']' al cerrar indice de lista.", "Missing ']' to close list index."},
        {"Numero invalido: ", "Invalid number: "},
        {"Falta ')' en expresion.", "Missing ')' in expression."},
        {"Falta ']' al cerrar literal de lista.", "Missing ']' to close list literal."},
        {"Clave invalida en objeto: ", "Invalid object key: "},
        {"Falta ':' despues de clave de objeto.", "Missing ':' after object key."},
        {"Falta '}' al cerrar literal de objeto.", "Missing '}' to close object literal."},
        {"Token no soportado en expresion: ", "Unsupported token in expression: "},
        {"Falta operador de asignacion.", "Missing assignment operator."},
        {"Falta ';' al final de la asignacion.", "Missing ';' at end of assignment."},
        {"Falta expresion en la asignacion.", "Missing expression in assignment."},
        {"Se esperaba un identificador.", "Expected an identifier."},
        {"Token de control fuera de bloque: ", "Control token outside block: "},
        {"Literal de objeto incompleto.", "Incomplete object literal."},
        {"Falta ';' al final de print.", "Missing ';' at end of print."},
        {"Se esperaba '(' despues de print.", "Expected '(' after print."},
        {"Se esperaba 'endif'.", "Expected 'endif'."},
        {"Se esperaba 'else:' o 'endif'.", "Expected 'else:' or 'endif'."},
        {"Se esperaba 'endif' despues de else.", "Expected 'endif' after else."},
        {"print requiere una expresion interna.", "print requires an inner expression."},
        {"Falta ':' al final del if.", "Missing ':' at end of if."},
        {"Falta 'endif' para cerrar el bloque if.", "Missing 'endif' to close if block."},
        {"Falta ':' al final de else.", "Missing ':' at end of else."},
        {"Falta 'endif' para cerrar el bloque else.", "Missing 'endif' to close else block."},
        {"Falta nombre de funcion valido.", "Missing valid function name."},
        {"Se esperaba '(' en la declaracion de funcion.", "Expected '(' in function declaration."},
        {"Se esperaba ',' o ')' en parametros de funcion.", "Expected ',' or ')' in function parameters."},
        {"Tokens extra despues de declaracion de funcion.", "Extra tokens after function declaration."},
        {"Falta ':' al final de la declaracion de funcion.", "Missing ':' at end of function declaration."},
        {"Parametro invalido en declaracion de funcion.", "Invalid parameter in function declaration."},
        {"Declaracion de funcion invalida: falta ':' final.", "Invalid function declaration: missing final ':'."},
        {"Falta 'endfunc' para cerrar la funcion '", "Missing 'endfunc' to close function '"},
        {"Formato invalido en import. Use: import modulo;", "Invalid import format. Use: import module;"},
        {"Formato invalido en try. Use: try:", "Invalid try format. Use: try:"},
        {"Falta 'catch:' para cerrar bloque try.", "Missing 'catch:' to close try block."},
        {"Se esperaba 'catch:' despues de try.", "Expected 'catch:' after try."},
        {"Falta ':' al final de catch.", "Missing ':' at end of catch."},
        {"Formato invalido en catch. Use: catch: o catch(error):", "Invalid catch format. Use: catch: or catch(error):"},
        {"Solo se permite un catch por bloque try.", "Only one catch is allowed per try block."},
        {"Falta 'endtry' para cerrar bloque try/catch.", "Missing 'endtry' to close try/catch block."},
        {"Se esperaba 'endtry'.", "Expected 'endtry'."},
        {"'endtry' no acepta tokens adicionales.", "'endtry' does not accept extra tokens."},
        {"Falta ';' al final de la mutacion.", "Missing ';' at end of mutation."},
        {"No se encontro operador de asignacion para mutacion.", "No assignment operator found for mutation."},
        {"Mutacion invalida: falta expresion en un lado de la asignacion.", "Invalid mutation: missing expression on one side of assignment."},
        {"Asignacion de mutacion incompleta.", "Incomplete mutation assignment."},
        {"Formato invalido en return. Use: return; o return expr;", "Invalid return format. Use: return; or return expr;"},
        {"print requiere cerrar ')' antes de ';'.", "print requires closing ')' before ';'."},
        {"Error de parseo en linea ", "Parse error at line "},
        {"Error de ejecucion: ", "Runtime error: "},
        {"Error de compilacion LLVM: ", "LLVM compilation error: "},
        {"Salida generada: ", "Generated output: "},
        {"Error: este binario no tiene soporte LLVM habilitado.", "Error: this binary does not have LLVM support enabled."},
        {"project_root vacio: no se puede enlazar runtime bridge LLVM.", "project_root is empty: cannot link LLVM runtime bridge."},
        {"No se encontraron archivos fuente para runtime bridge LLVM en: ", "No source files found for LLVM runtime bridge in: "},
        {"Tipo de sentencia no soportado por el interprete.", "Unsupported statement type in interpreter."},
        {"return solo se permite dentro de una funcion.", "return is only allowed inside a function."},
        {"Expresion no soportada por el interprete.", "Unsupported expression in interpreter."},
        {"Operacion binaria no soportada.", "Unsupported binary operation."},
        {"sum(a, b) requiere 2 argumentos.", "sum(a, b) requires 2 arguments."},
        {"sum(a, b) requiere exactamente 2 argumentos.", "sum(a, b) requires exactly 2 arguments."},
        {"sum(a, b) no acepta argumentos por referencia.", "sum(a, b) does not accept by-reference arguments."},
        {"input() acepta 0 o 1 argumento.", "input() accepts 0 or 1 argument."},
        {"read_file(path) requiere 1 argumento.", "read_file(path) requires 1 argument."},
        {"write_file(path, content) requiere 2 argumentos.", "write_file(path, content) requires 2 arguments."},
        {"append_file(path, content) requiere 2 argumentos.", "append_file(path, content) requires 2 arguments."},
        {"file_exists(path) requiere 1 argumento.", "file_exists(path) requires 1 argument."},
        {"now_ms() no acepta argumentos.", "now_ms() does not accept arguments."},
        {"sleep_ms(ms) requiere 1 argumento.", "sleep_ms(ms) requires 1 argument."},
        {"sleep_ms(ms) requiere entero >= 0.", "sleep_ms(ms) requires integer >= 0."},
        {"async_read_file(path) requiere 1 argumento.", "async_read_file(path) requires 1 argument."},
        {"task_ready(task_id) requiere 1 argumento.", "task_ready(task_id) requires 1 argument."},
        {"await(task_id) requiere 1 argumento.", "await(task_id) requires 1 argument."},
        {"El id de tarea debe ser un entero positivo.", "Task id must be a positive integer."},
        {"Id de tarea no encontrado: ", "Task id not found: "},
        {"Numero incorrecto de argumentos para funcion '", "Incorrect number of arguments for function '"},
        {"Parametro por referencia '", "Reference parameter '"},
        {"Referencia no soporta acceso con propiedad: ", "Reference does not support property access: "},
        {"Variable no definida para referencia: ", "Undefined variable for reference: "},
        {"No se puede pasar '&' a un parametro por valor: ", "Cannot pass '&' to value parameter: "},
        {"La funcion '", "Function '"},
        {"Variable no definida: ", "Undefined variable: "},
        {"Acceso de propiedad invalido: ", "Invalid property access: "},
        {"Propiedad no encontrada: ", "Property not found: "},
        {"No se puede acceder propiedad en un valor no objeto: ", "Cannot access property on non-object value: "},
        {"Solo se puede indexar una lista con [].", "Only lists can be indexed with []."},
        {"El indice de lista debe ser un entero finito.", "List index must be a finite integer."},
        {"Indice fuera de rango en lista.", "List index out of bounds."},
        {"Solo se puede mutar una lista con [].", "Only lists can be mutated with []."},
        {"El lado izquierdo de una mutacion debe ser variable o indexacion.", "Left side of a mutation must be variable or index expression."},
        {"Valor fuera de rango para long.", "Value out of range for long."},
        {"Valor fuera de rango para byte (0-255).", "Value out of range for byte (0-255)."},
        {"No se puede declarar tipo long/byte sobre una propiedad de objeto.", "Cannot declare long/byte type on object property."},
        {"Import circular detectado en modulo: ", "Circular import detected in module: "},
        {"Error importando modulo '", "Error importing module '"},
        {"Error de parseo importando modulo '", "Parse error importing module '"},
        {"Error interno: pila de retorno inconsistente.", "Internal error: inconsistent return stack."},
        {"Error interno: out_value nulo en ResolveMutableVariable.", "Internal error: null out_value in ResolveMutableVariable."},
        {"Error interno: out_value nulo en NormalizeValueForKind.", "Internal error: null out_value in NormalizeValueForKind."},
        {"Error de runtime bridge: source_text nulo.", "Runtime bridge error: source_text is null."},
        {"No se pudo abrir el archivo: ", "Could not open file: "},
        {"Error leyendo el archivo: ", "Error reading file: "},
        {"Error escribiendo el archivo: ", "Error writing file: "},
        {"No se pudo crear la funcion main.", "Failed to create main function."},
        {"Se requiere output_path para compilar con LLVM.", "output_path is required to compile with LLVM."},
        {"El backend LLVM no puede emitir archivo objeto para ese target.", "LLVM backend cannot emit object file for this target."},
        {"No se pudo escribir IR en '", "Could not write IR to '"},
        {"No se encontro target LLVM '", "LLVM target not found '"},
        {"No se pudo crear TargetMachine para '", "Could not create TargetMachine for '"},
        {"No se pudo abrir el archivo objeto '", "Could not open object file '"},
        {"Fallo el enlazado con clang++. Comando: ", "Linking with clang++ failed. Command: "},
        {"No se encontro runtime bridge externo LLVM en: ", "LLVM external runtime bridge was not found at: "},
        {"LLVM genero una funcion main invalida.", "LLVM generated an invalid main function."},
        {"LLVM genero una funcion main invalida en runtime bridge.", "LLVM generated an invalid main function in runtime bridge."},
        {"LLVM genero un modulo invalido.", "LLVM generated an invalid module."},
        {"LLVM genero un modulo invalido en runtime bridge.", "LLVM generated an invalid module in runtime bridge."},
        {"No hay codigo fuente para runtime bridge LLVM.", "No source code for LLVM runtime bridge."},
        {"No se pudo crear la funcion LLVM para '", "Could not create LLVM function for '"},
        {"Funcion LLVM invalida durante emision.", "Invalid LLVM function during emission."},
        {"LLVM genero una funcion invalida para '", "LLVM generated an invalid function for '"},
        {"Funcion interna no encontrada durante emision LLVM: ", "Internal function not found during LLVM emission: "},
        {"Funcion duplicada no soportada en AOT LLVM: ", "Duplicated function is not supported in LLVM AOT: "},
        {"No se soportan funciones anidadas en modo compile LLVM AOT.", "Nested functions are not supported in LLVM AOT compile mode."},
        {"Sentencia no soportada por el compilador LLVM.", "Unsupported statement in LLVM compiler."},
        {"Sentencia de expresion vacia en emision LLVM.", "Empty expression statement during LLVM emission."},
        {"Variable no definida para asignacion compuesta: ", "Undefined variable for compound assignment: "},
        {"Estado interno invalido: no hay funcion activa para asignacion.", "Invalid internal state: no active function for assignment."},
        {"sum(a, b) requiere 'import math;' en modo compile LLVM AOT.", "sum(a, b) requires 'import math;' in LLVM AOT compile mode."},
        {"Funcion no definida en modo compile LLVM AOT: ", "Undefined function in LLVM AOT compile mode: "},
        {"Referencia por propiedad no soportada en AOT LLVM: ", "Property reference is not supported in LLVM AOT: "},
        {"Acceso por propiedad no soportado en AOT LLVM: ", "Property access is not supported in LLVM AOT: "},
        {"Las listas aun no se soportan en modo compile LLVM AOT.", "Lists are not yet supported in LLVM AOT compile mode."},
        {"Los objetos aun no se soportan en modo compile LLVM AOT.", "Objects are not yet supported in LLVM AOT compile mode."},
        {"La indexacion de listas aun no se soporta en modo compile LLVM AOT.", "List indexing is not yet supported in LLVM AOT compile mode."},
        {"No se puede pasar '&' a parametro por valor en llamada '", "Cannot pass '&' to value parameter in call '"},
        {"Parametro por referencia en '", "Reference parameter in '"},
        {"Las declaraciones tipadas solo aceptan '='.", "Typed declarations only accept '='."},
        {"La expresion requiere un valor numerico.", "Expression requires a numeric value."},
        {"Las expresiones string solo se soportan como literal directo en print dentro del modo compilado.", "String expressions are only supported as direct literals in print in compiled mode."},
        {"Import no soportado en LLVM: ", "Unsupported import in LLVM: "},
        {"try/catch aun no se soporta en modo compile LLVM AOT.", "try/catch is not yet supported in LLVM AOT compile mode."},
        {"Funcion no soportada en modo compile LLVM AOT: ", "Unsupported function in LLVM AOT compile mode: "},
        {"Llamada no soportada en modo compile LLVM AOT: ", "Unsupported call in LLVM AOT compile mode: "},
        {"Expresion no soportada en backend LLVM.", "Unsupported expression in LLVM backend."},
        {"Este binario se compilo sin soporte LLVM. Reconfigura con LLVM instalado.", "This binary was built without LLVM support. Reconfigure with LLVM installed."},
        {"Analisis estatico (sentencia ", "Static analysis (statement "},
        {"Analisis estatico sin errores criticos.", "Static analysis completed with no critical errors."},
    };

    std::string translated = text;
    for (const auto& rule : kPrefixRules) {
        if (StartsWith(translated, rule.first)) {
            translated = std::string(rule.second) + translated.substr(std::string(rule.first).size());
            break;
        }
    }

    ReplaceAll(&translated, ", columna ", ", column ");
    ReplaceAll(&translated, ", linea ", ", line ");
    ReplaceAll(&translated, " en linea ", " at line ");
    ReplaceAll(&translated, "No se pudo abrir el archivo: ", "Could not open file: ");
    ReplaceAll(&translated, "Error leyendo el archivo: ", "Error reading file: ");
    ReplaceAll(&translated, "Error escribiendo el archivo: ", "Error writing file: ");
    ReplaceAll(&translated, "Variable no definida: ", "Undefined variable: ");
    ReplaceAll(&translated, "Funcion no definida: ", "Undefined function: ");
    ReplaceAll(&translated, "Propiedad no encontrada: ", "Property not found: ");
    ReplaceAll(&translated, "Indice fuera de rango en lista.", "List index out of bounds.");
    ReplaceAll(&translated, " requiere una variable.", " requires a variable.");
    ReplaceAll(&translated, " no retorno ningun valor.", " did not return any value.");
    ReplaceAll(&translated, " en modo compile LLVM AOT.", " in LLVM AOT compile mode.");
    return translated;
}

}  // namespace

bool ParseLanguage(const std::string& text, Language* out_language) {
    if (out_language == nullptr) {
        return false;
    }

    std::string normalized;
    normalized.reserve(text.size());
    for (char ch : text) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (normalized == "es" || normalized == "spa" || normalized == "spanish") {
        *out_language = Language::Spanish;
        return true;
    }
    if (normalized == "en" || normalized == "eng" || normalized == "english") {
        *out_language = Language::English;
        return true;
    }

    return false;
}

void SetLanguage(Language language) {
    g_language.store(language);
}

Language GetLanguage() {
    return g_language.load();
}

std::string LanguageCode(Language language) {
    return language == Language::English ? "en" : "es";
}

std::string Tr(const std::string& spanish, const std::string& english) {
    return GetLanguage() == Language::English ? english : spanish;
}

std::string TranslateDiagnostic(const std::string& text) {
    if (GetLanguage() == Language::Spanish) {
        return text;
    }
    return TranslateSpanishToEnglish(text);
}

}  // namespace clot::runtime

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
        {"Token no reconocido: '", "Unrecognized token: '"},
        {"Literal char invalido.", "Invalid char literal."},
        {"Token no soportado en expresion: ", "Unsupported token in expression: "},
        {"Declaracion const incompleta.", "Incomplete const declaration."},
        {"Error interno: salida nula al parsear type hint.", "Internal error: null output while parsing type hint."},
        {"Falta tipo.", "Missing type."},
        {"Tipo no reconocido: '", "Unrecognized type: '"},
        {"Falta '>' al cerrar type hint generico.", "Missing '>' to close generic type hint."},
        {"Se esperaba ',' o '>' en type hint generico.", "Expected ',' or '>' in generic type hint."},
        {"El type hint '", "Type hint '"},
        {"Cantidad de argumentos genericos invalida para type hint '",
         "Invalid number of generic arguments for type hint '"},
        {"Falta operador de asignacion.", "Missing assignment operator."},
        {"Operador de asignacion no valido.", "Invalid assignment operator."},
        {"Falta ';' al final de la asignacion.", "Missing ';' at end of assignment."},
        {"Falta expresion en la asignacion.", "Missing expression in assignment."},
        {"Se esperaba un identificador.", "Expected an identifier."},
        {"Token de control fuera de bloque: ", "Control token outside block: "},
        {"Literal de objeto incompleto.", "Incomplete object literal."},
        {"Instruccion de salida incompleta.", "Incomplete output statement."},
        {"Se esperaba '(' despues de la instruccion de salida.", "Expected '(' after output statement."},
        {"Falta ';' al final de la instruccion de salida.", "Missing ';' at end of output statement."},
        {"La instruccion de salida requiere cerrar ')' antes de ';'.", "Output statement requires closing ')' before ';'."},
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
        {"Falta tipo de parametro despues de ':'.", "Missing parameter type after ':'."},
        {"Tipo de parametro no reconocido: '", "Unrecognized parameter type: '"},
        {"Tipo de parametro no reconocido: ", "Unrecognized parameter type: "},
        {"Falta expresion de valor por defecto despues de '='.", "Missing default value expression after '='."},
        {"Los parametros sin valor por defecto no pueden ir despues de parametros con default.",
         "Parameters without default values cannot follow parameters with defaults."},
        {"Los parametros por referencia no aceptan valor por defecto.",
         "By-reference parameters do not accept default values."},
        {"Falta 'endfunc' para cerrar la funcion '", "Missing 'endfunc' to close function '"},
        {"Formato invalido en import. Use: import modulo;, import modulo as alias;, from modulo import simbolo; o from modulo import simbolo as alias;",
         "Invalid import format. Use: import module;, import module as alias;, from module import symbol;, or from module import symbol as alias;"},
        {"Formato invalido en enum. Use: enum Nombre { A, B };", "Invalid enum format. Use: enum Name { A, B };"},
        {"Miembro invalido en enum.", "Invalid enum member."},
        {"Se esperaba ',' o '}' en enum.", "Expected ',' or '}' in enum."},
        {"Tokens extra despues del cierre de enum.", "Extra tokens after enum closing brace."},
        {"Falta '}' para cerrar enum.", "Missing '}' to close enum."},
        {"No se permite coma final en enum.", "Trailing comma is not allowed in enum."},
        {"Enum no puede estar vacio.", "Enum cannot be empty."},
        {"Miembro enum duplicado: ", "Duplicated enum member: "},
        {"Formato invalido en try. Use: try:", "Invalid try format. Use: try:"},
        {"Falta 'catch:' para cerrar bloque try.", "Missing 'catch:' to close try block."},
        {"Se esperaba 'catch:' despues de try.", "Expected 'catch:' after try."},
        {"Falta ':' al final de catch.", "Missing ':' at end of catch."},
        {"Formato invalido en catch. Use: catch:, catch(error):, catch(Tipo): o catch(Tipo error):",
         "Invalid catch format. Use: catch:, catch(error):, catch(Type):, or catch(Type error):"},
        {"Solo se permite un catch por bloque try.", "Only one catch is allowed per try block."},
        {"Falta catch/finally para cerrar bloque try.", "Missing catch/finally to close try block."},
        {"Formato invalido en finally. Use: finally:", "Invalid finally format. Use: finally:"},
        {"Bloque try no permite catch/finally despues de finally.",
         "try block does not allow catch/finally after finally."},
        {"El bloque try requiere catch, finally o ambos.", "try block requires catch, finally, or both."},
        {"Falta 'endtry' para cerrar bloque try/catch/finally.",
         "Missing 'endtry' to close try/catch/finally block."},
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
        {"Excepcion no capturada: ", "Unhandled Exception: "},
        {"Error de compilacion LLVM: ", "LLVM compilation error: "},
        {"Salida generada: ", "Generated output: "},
        {"Error: este binario no tiene soporte LLVM habilitado.", "Error: this binary does not have LLVM support enabled."},
        {"project_root vacio: no se puede enlazar runtime bridge LLVM.", "project_root is empty: cannot link LLVM runtime bridge."},
        {"No se encontraron archivos fuente para runtime bridge LLVM en: ", "No source files found for LLVM runtime bridge in: "},
        {"Tipo de sentencia no soportado por el interprete.", "Unsupported statement type in interpreter."},
        {"Instruccion while incompleta.", "Incomplete while statement."},
        {"Falta 'endwhile' para cerrar el bloque while.", "Missing 'endwhile' to close while block."},
        {"Se esperaba 'endwhile' para cerrar el bucle.", "Expected 'endwhile' to close while loop."},
        {"Tokens extra despues de 'endwhile'.", "Extra tokens after 'endwhile'."},
        {"Formato invalido en for. Use: for (init; cond; update): o for (item in coleccion):",
         "Invalid for format. Use: for (init; cond; update): or for (item in collection):"},
        {"Falta 'endfor' para cerrar el bloque for.", "Missing 'endfor' to close for block."},
        {"Se esperaba 'endfor' para cerrar el for.", "Expected 'endfor' to close for loop."},
        {"Tokens extra despues de 'endfor'.", "Extra tokens after 'endfor'."},
        {"for init/update solo permite declaraciones, mutaciones o expresiones simples.",
         "for init/update only allows declarations, mutations, or simple expressions."},
        {"for init/update invalido.", "Invalid for init/update."},
        {"Cabecera for invalida.", "Invalid for header."},
        {"Formato invalido en for-each.", "Invalid for-each format."},
        {"Formato invalido en for-each. Use: for (item in coleccion):",
         "Invalid for-each format. Use: for (item in collection):"},
        {"Formato invalido en do-while. Use: do:", "Invalid do-while format. Use: do:"},
        {"Falta while(condicion); para cerrar do-while.", "Missing while(condition); to close do-while."},
        {"Formato invalido en do-while. Use: while(condicion);",
         "Invalid do-while format. Use: while(condition);"},
        {"Formato invalido en switch. Use: switch(expr):", "Invalid switch format. Use: switch(expr):"},
        {"Formato invalido en case. Use: case valor:", "Invalid case format. Use: case value:"},
        {"Formato invalido en default. Use: default:", "Invalid default format. Use: default:"},
        {"switch solo permite un default.", "switch only allows one default."},
        {"switch requiere labels case/default.", "switch requires case/default labels."},
        {"Falta 'endswitch' para cerrar switch.", "Missing 'endswitch' to close switch."},
        {"Se esperaba 'endswitch'.", "Expected 'endswitch'."},
        {"Tokens extra despues de 'endswitch'.", "Extra tokens after 'endswitch'."},
        {"switch requiere al menos un case o default.", "switch requires at least one case or default."},
        {"Formato invalido en break. Use: break;", "Invalid break format. Use: break;"},
        {"Formato invalido en continue. Use: continue;", "Invalid continue format. Use: continue;"},
        {"Formato invalido en pass. Use: pass;", "Invalid pass format. Use: pass;"},
        {"Formato invalido en defer. Use: defer sentencia;", "Invalid defer format. Use: defer statement;"},
        {"defer requiere una sentencia.", "defer requires a statement."},
        {"defer solo acepta sentencias simples.", "defer only accepts simple statements."},
        {"defer requiere exactamente una sentencia.", "defer requires exactly one statement."},
        {"return solo se permite dentro de una funcion.", "return is only allowed inside a function."},
        {"break/continue fuera de contexto de bucle.", "break/continue outside of loop context."},
        {"break solo se permite dentro de bucles o switch.", "break is only allowed inside loops or switch."},
        {"continue solo se permite dentro de bucles.", "continue is only allowed inside loops."},
        {"defer invalido: sentencia vacia.", "Invalid defer: empty statement."},
        {"defer fuera de bloque.", "defer outside block."},
        {"Expresion no soportada por el interprete.", "Unsupported expression in interpreter."},
        {"Operacion binaria no soportada.", "Unsupported binary operation."},
        {"Division por cero.", "Division by zero."},
        {"Modulo por cero.", "Modulo by zero."},
        {"La expresion requiere un entero.", "Expression requires an integer."},
        {"No se puede convertir entero grande a double para division.", "Cannot convert large integer to double for division."},
        {"No se puede convertir entero grande a double para potencia.", "Cannot convert large integer to double for power."},
        {"sum(a, b) requiere 2 argumentos.", "sum(a, b) requires 2 arguments."},
        {"sum(a, b) requiere exactamente 2 argumentos.", "sum(a, b) requires exactly 2 arguments."},
        {"sum(a, b) no acepta argumentos por referencia.", "sum(a, b) does not accept by-reference arguments."},
        {"factorial() argumento demasiado grande.", "factorial() argument is too large."},
        {"sqrt(x) requiere x >= 0.", "sqrt(x) requires x >= 0."},
        {"log(x) o log(x, base) requiere 1 o 2 argumentos.", "log(x) or log(x, base) requires 1 or 2 arguments."},
        {"log(x) requiere x > 0.", "log(x) requires x > 0."},
        {"log(x, base) requiere base > 0 y base != 1.", "log(x, base) requires base > 0 and base != 1."},
        {"ln(x) requiere x > 0.", "ln(x) requires x > 0."},
        {"asin(x) requiere -1 <= x <= 1.", "asin(x) requires -1 <= x <= 1."},
        {"acos(x) requiere -1 <= x <= 1.", "acos(x) requires -1 <= x <= 1."},
        {"gcd(a, b) requiere 2 argumentos.", "gcd(a, b) requires 2 arguments."},
        {"lcm(a, b) requiere 2 argumentos.", "lcm(a, b) requires 2 arguments."},
        {"type(value) requiere 1 argumento.", "type(value) requires 1 argument."},
        {"cast(value, type_name) requiere 2 argumentos.", "cast(value, type_name) requires 2 arguments."},
        {"cast(): tipo destino no soportado: ", "cast(): unsupported target type: "},
        {"assert(cond) o assert(cond, mensaje) requiere 1 o 2 argumentos.", "assert(cond) or assert(cond, message) requires 1 or 2 arguments."},
        {"assert() fallo: ", "assert() failed: "},
        {"assert() fallo.", "assert() failed."},
        {"throw(value) requiere 1 argumento.", "throw(value) requires 1 argument."},
        {"Exception lanzada.", "Thrown exception."},
        {"input() acepta 0 o 1 argumento.", "input() accepts 0 or 1 argument."},
        {"println() acepta 0 o 1 argumento.", "println() accepts 0 or 1 argument."},
        {"printf(format, ...args) requiere al menos 1 argumento.", "printf(format, ...args) requires at least 1 argument."},
        {"printf: formato invalido, '%' sin especificador.", "printf: invalid format, '%' without specifier."},
        {"printf: faltan argumentos para el formato.", "printf: missing arguments for format string."},
        {"printf: sobran argumentos para el formato.", "printf: extra arguments not used by format string."},
        {"printf: %d/%i requiere entero.", "printf: %d/%i requires an integer."},
        {"printf: %u requiere entero sin signo (>= 0).", "printf: %u requires unsigned integer (>= 0)."},
        {"printf: %f requiere valor numerico.", "printf: %f requires a numeric value."},
        {"printf: %c requiere char (string de longitud 1 o entero ASCII 0-255).", "printf: %c requires a char (single-character string or ASCII integer 0-255)."},
        {"printf: %x/%X requiere entero sin signo (>= 0).", "printf: %x/%X requires unsigned integer (>= 0)."},
        {"printf: especificador no soportado '%", "printf: unsupported format specifier '%"},
        {"map(key, value, ...) requiere cantidad par de argumentos.", "map(key, value, ...) requires an even number of arguments."},
        {"len(value) requiere 1 argumento.", "len(value) requires 1 argument."},
        {"len() requiere un string, list, tuple, set, map, object o char.",
         "len() requires a string, list, tuple, set, map, object, or char."},
        {"range() requiere 1, 2 o 3 argumentos.", "range() requires 1, 2, or 3 arguments."},
        {"range() requiere step != 0.", "range() requires step != 0."},
        {"range() excede el maximo de 1000000 elementos.", "range() exceeds the maximum of 1000000 elements."},
        {"enumerate(iterable, start=0) requiere 1 o 2 argumentos.",
         "enumerate(iterable, start=0) requires 1 or 2 arguments."},
        {"enumerate() requiere un iterable (list, tuple, set, map, object o string).",
         "enumerate() requires an iterable (list, tuple, set, map, object, or string)."},
        {"zip() requiere iterables validos en todos los argumentos.",
         "zip() requires valid iterables in all arguments."},
        {"all(iterable) requiere 1 argumento.", "all(iterable) requires 1 argument."},
        {"any(iterable) requiere 1 argumento.", "any(iterable) requires 1 argument."},
        {"all() requiere un iterable (list, tuple, set, map, object o string).",
         "all() requires an iterable (list, tuple, set, map, object, or string)."},
        {"any() requiere un iterable (list, tuple, set, map, object o string).",
         "any() requires an iterable (list, tuple, set, map, object, or string)."},
        {"isinstance(value, type_name) requiere 2 argumentos.", "isinstance(value, type_name) requires 2 arguments."},
        {"isinstance(): type_name debe ser string, char, list, tuple o set.",
         "isinstance(): type_name must be string, char, list, tuple, or set."},
        {"chr(code) requiere 1 argumento.", "chr(code) requires 1 argument."},
        {"chr(code) requiere 0 <= code <= 255.", "chr(code) requires 0 <= code <= 255."},
        {"ord(char) requiere 1 argumento.", "ord(char) requires 1 argument."},
        {"ord() requiere char o string de longitud 1.", "ord() requires char or a string of length 1."},
        {"hex(value) requiere 1 argumento.", "hex(value) requires 1 argument."},
        {"bin(value) requiere 1 argumento.", "bin(value) requires 1 argument."},
        {"hash(value) requiere 1 argumento.", "hash(value) requires 1 argument."},
        {"id(value) requiere 1 argumento.", "id(value) requires 1 argument."},
        {"id() excedio el limite de identidades en runtime.", "id() exceeded the runtime identity limit."},
        {"enum_name(enum_obj, value) requiere 2 argumentos.", "enum_name(enum_obj, value) requires 2 arguments."},
        {"enum_name(): primer argumento debe ser enum.", "enum_name(): first argument must be an enum."},
        {"enum_name(): valor no encontrado en enum.", "enum_name(): value not found in enum."},
        {"enum_value(enum_obj, name) requiere 2 argumentos.", "enum_value(enum_obj, name) requires 2 arguments."},
        {"enum_value(): primer argumento debe ser enum.", "enum_value(): first argument must be an enum."},
        {"enum_value(): nombre invalido.", "enum_value(): invalid member name."},
        {"enum_value(): miembro no encontrado en enum.", "enum_value(): member not found in enum."},
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
        {"Elemento list[", "List element["},
        {"Elemento tuple[", "Tuple element["},
        {"Elemento set[", "Set element["},
        {"Clave map[", "Map key["},
        {"Valor map[", "Map value["},
        {"Propiedad object.", "Object property."},
        {"Error interno: salida nula en for-each.", "Internal error: null output in for-each."},
        {"for-each requiere list, tuple, set, map, object o string.",
         "for-each requires list, tuple, set, map, object, or string."},
        {"Operador 'in' requiere list, tuple, set, map, object o string a la derecha.",
         "Operator 'in' requires list, tuple, set, map, object, or string on the right-hand side."},
        {"Solo se puede indexar una lista con [].", "Only lists can be indexed with []."},
        {"Solo se puede indexar list, tuple, map u object con [].", "Only list, tuple, map, or object can be indexed with []."},
        {"El indice de lista debe ser un entero finito.", "List index must be a finite integer."},
        {"Indice fuera de rango en lista.", "List index out of bounds."},
        {"Indice fuera de rango en tuple.", "Tuple index out of bounds."},
        {"Clave no encontrada en map.", "Key not found in map."},
        {"Solo se puede mutar una lista con [].", "Only lists can be mutated with []."},
        {"No se puede mutar un tuple con [].", "Cannot mutate a tuple with []."},
        {"Solo se puede mutar list, map u object con [].", "Only list, map, or object can be mutated with []."},
        {"El lado izquierdo de una mutacion debe ser variable o indexacion.", "Left side of a mutation must be variable or index expression."},
        {"Valor fuera de rango para long.", "Value out of range for long."},
        {"Valor fuera de rango para float.", "Value out of range for float."},
        {"Valor fuera de rango para byte (0-255).", "Value out of range for byte (0-255)."},
        {"Valor no finito para double.", "Non-finite value for double."},
        {"La expresion requiere un decimal.", "Expression requires a decimal."},
        {"Valor invalido para char (requiere longitud 1).", "Invalid value for char (requires length 1)."},
        {"Valor invalido para char.", "Invalid value for char."},
        {"Valor invalido para tuple.", "Invalid value for tuple."},
        {"Valor invalido para map.", "Invalid value for map."},
        {"Valor invalido para function.", "Invalid value for function."},
        {"Las constantes solo aceptan '='.", "Constants only accept '='."},
        {"No se puede declarar const sobre una propiedad de objeto.", "Cannot declare const on an object property."},
        {"No se puede modificar constante: ", "Cannot modify constant: "},
        {"Constante ya definida: ", "Constant already defined: "},
        {"No se puede declarar tipo long/byte sobre una propiedad de objeto.", "Cannot declare long/byte type on object property."},
        {"No se puede declarar tipo int/double/long/byte sobre una propiedad de objeto.", "Cannot declare int/double/long/byte type on object property."},
        {"No se puede declarar tipo explicito sobre una propiedad de objeto.", "Cannot declare explicit type on object property."},
        {"No se pudo convertir el valor numerico a entero.", "Could not convert numeric value to integer."},
        {"Import circular detectado en modulo: ", "Circular import detected in module: "},
        {"Error importando modulo '", "Error importing module '"},
        {"Error de parseo importando modulo '", "Parse error importing module '"},
        {"Import directo/alias para 'math' no soportado; use 'import math;'.",
         "Direct/aliased import for 'math' is not supported; use 'import math;'."},
        {"Error interno: cache de exportaciones no encontrada para modulo '",
         "Internal error: exports cache not found for module '"},
        {"Alias de modulo invalido en import.", "Invalid module alias in import."},
        {"Import directo invalido: simbolo vacio.", "Invalid direct import: empty symbol."},
        {"Simbolo '", "Symbol '"},
        {"' no exportado por el modulo '", "' is not exported by module '"},
        {"Miembro no invocable: ", "Non-callable member: "},
        {"Error interno: pila de retorno inconsistente.", "Internal error: inconsistent return stack."},
        {"Error interno: out_value nulo en ResolveMutableVariable.", "Internal error: null out_value in ResolveMutableVariable."},
        {"Error interno: out_value nulo en NormalizeValueForKind.", "Internal error: null out_value in NormalizeValueForKind."},
        {"Error interno: out_value nulo en NormalizeValueForTypeAnnotation.",
         "Internal error: null out_value in NormalizeValueForTypeAnnotation."},
        {"Error interno: salida nula en printf.", "Internal error: null output buffer in printf."},
        {"Error interno: salida nula en potencia entera.", "Internal error: null output in integer power."},
        {"Error interno: salida nula en division decimal.", "Internal error: null output in decimal division."},
        {"pow() no acepta exponente entero negativo para resultado entero.", "pow() does not accept a negative integer exponent for integer result."},
        {"pow() exponente entero demasiado grande para computar.", "pow() integer exponent is too large to compute."},
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
        {"Las variables int arbitrarias no se soportan en AOT LLVM; usa runtime bridge.", "Arbitrary-precision int variables are not supported in LLVM AOT; use runtime bridge."},
        {"Declaracion tipada no soportada en AOT LLVM; usa runtime bridge.", "Typed declaration is not supported in LLVM AOT; use runtime bridge."},
        {"Error interno LLVM: indice de argumento invalido en builtin math.", "LLVM internal error: invalid argument index in math builtin."},
        {"factorial(x) requiere 1 argumento.", "factorial(x) requires 1 argument."},
        {"log(x) o log(x, base) requiere 1 o 2 argumentos.", "log(x) or log(x, base) requires 1 or 2 arguments."},
        {"Builtin math no soportado en LLVM AOT: ", "Unsupported math builtin in LLVM AOT: "},
        {"Numero entero no convertible a double en AOT LLVM: ", "Integer literal cannot be converted to double in LLVM AOT: "},
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
        {"Operador 'in' no soportado en modo compile LLVM AOT; usa runtime bridge.",
         "Operator 'in' is not supported in LLVM AOT compile mode; use runtime bridge."},
        {"Este binario se compilo sin soporte LLVM. Reconfigura con LLVM instalado.", "This binary was built without LLVM support. Reconfigure with LLVM installed."},
        {"Asignacion potencialmente invalida para char en '", "Potentially invalid assignment for char in '"},
        {"Asignacion potencialmente invalida para tuple en '", "Potentially invalid assignment for tuple in '"},
        {"Asignacion potencialmente invalida para set en '", "Potentially invalid assignment for set in '"},
        {"Asignacion potencialmente invalida para map en '", "Potentially invalid assignment for map in '"},
        {"Asignacion potencialmente invalida para function en '", "Potentially invalid assignment for function in '"},
        {"Formato invalido en interface. Use: interface Nombre:", "Invalid interface format. Use: interface Name:"},
        {"Tokens extra en declaracion de interface.", "Extra tokens in interface declaration."},
        {"'endinterface' no acepta tokens adicionales.", "'endinterface' does not accept extra tokens."},
        {"Solo se permiten firmas 'func ...;' dentro de interface.", "Only 'func ...;' signatures are allowed inside interface."},
        {"Las firmas en interface deben terminar en ';'.", "Interface signatures must end with ';'."},
        {"Firma invalida en interface: falta nombre de metodo.", "Invalid interface signature: missing method name."},
        {"Firma invalida en interface: se esperaba '('.", "Invalid interface signature: expected '('."},
        {"Parametro invalido en firma de interface.", "Invalid parameter in interface signature."},
        {"Tipo de parametro no reconocido en interface.", "Unrecognized parameter type in interface."},
        {"Las firmas de interface no aceptan valores por defecto.", "Interface signatures do not accept default values."},
        {"Se esperaba ',' o ')' en firma de interface.", "Expected ',' or ')' in interface signature."},
        {"Falta ';' al final de firma de interface.", "Missing ';' at end of interface signature."},
        {"Falta 'endinterface' para cerrar la interface '", "Missing 'endinterface' to close interface '"},
        {"Formato invalido en class. Use: class Nombre:", "Invalid class format. Use: class Name:"},
        {"Se esperaba clase base valida despues de 'extends'.", "Expected a valid base class after 'extends'."},
        {"Se esperaba interface valida despues de 'implements'.", "Expected a valid interface after 'implements'."},
        {"Nombre de interface invalido en 'implements'.", "Invalid interface name in 'implements'."},
        {"Token no esperado en cabecera de class: ", "Unexpected token in class header: "},
        {"'endclass' no acepta tokens adicionales.", "'endclass' does not accept extra tokens."},
        {"Declaracion de miembro vacia.", "Empty member declaration."},
        {"constructor no acepta modificadores static/readonly/override.", "constructor does not accept static/readonly/override modifiers."},
        {"Solo se permite un constructor por class.", "Only one constructor is allowed per class."},
        {"Se esperaba '(' en constructor.", "Expected '(' in constructor."},
        {"get no acepta modificadores static/readonly/override.", "get does not accept static/readonly/override modifiers."},
        {"Formato invalido en get. Use: get nombre():", "Invalid get format. Use: get name():"},
        {"set no acepta modificadores static/readonly/override.", "set does not accept static/readonly/override modifiers."},
        {"set requiere un parametro.", "set requires a parameter."},
        {"Falta tipo en parametro de set.", "Missing type in set parameter."},
        {"Tipo invalido en parametro de set.", "Invalid type in set parameter."},
        {"set requiere cerrar ')' en su parametro.", "set requires closing ')' in its parameter."},
        {"Falta ':' al final de set.", "Missing ':' at end of set."},
        {"Tokens extra despues de set.", "Extra tokens after set."},
        {"'endconstructor' no acepta tokens adicionales.", "'endconstructor' does not accept extra tokens."},
        {"'endget' no acepta tokens adicionales.", "'endget' does not accept extra tokens."},
        {"'endset' no acepta tokens adicionales.", "'endset' does not accept extra tokens."},
        {"Falta 'endconstructor' para cerrar bloque.", "Missing 'endconstructor' to close block."},
        {"Falta 'endget' para cerrar bloque.", "Missing 'endget' to close block."},
        {"Falta 'endset' para cerrar bloque.", "Missing 'endset' to close block."},
        {"readonly no aplica a metodos.", "readonly does not apply to methods."},
        {"Error interno parseando metodo de class.", "Internal error parsing class method."},
        {"override solo aplica a metodos.", "override only applies to methods."},
        {"Declaracion invalida de campo en class.", "Invalid class field declaration."},
        {"Falta expresion de inicializacion en campo.", "Missing initialization expression in field."},
        {"Falta ';' al final de campo.", "Missing ';' at end of field."},
        {"Falta 'endclass' para cerrar la class '", "Missing 'endclass' to close class '"},
        {"Clase base no definida: ", "Undefined base class: "},
        {"Metodo override sin base: ", "Method marked override without base method: "},
        {"Metodo sobrescribe base y requiere 'override': ", "Method overrides base and requires 'override': "},
        {"Override invalido por aridad en metodo: ", "Invalid override due to arity in method: "},
        {"Override invalido por static/instancia en metodo: ", "Invalid override due to static/instance mismatch in method: "},
        {"Override invalido por retorno en metodo: ", "Invalid override due to return type in method: "},
        {"Override invalido por firma en metodo: ", "Invalid override due to signature in method: "},
        {"Interface no definida: ", "Undefined interface: "},
        {"Metodo de interface no puede implementarse como static: ", "Interface method cannot be implemented as static: "},
        {"Metodo de interface debe ser public: ", "Interface method must be public: "},
        {"Firma incompatible en metodo de interface: ", "Incompatible signature in interface method: "},
        {"Retorno incompatible en metodo de interface: ", "Incompatible return type in interface method: "},
        {"Error interno: out_value nulo en ResolveClassStaticField.", "Internal error: null out_value in ResolveClassStaticField."},
        {"Clase '", "Class '"},
        {"Clase no definida: ", "Undefined class: "},
        {"Campo no definido: ", "Undefined field: "},
        {"Campo de instancia requiere objeto: ", "Instance field requires object: "},
        {"Campo no accesible por visibilidad: ", "Field not accessible due to visibility: "},
        {"Campo static no inicializado: ", "Static field not initialized: "},
        {"Interface duplicada: ", "Duplicated interface: "},
        {"Clase duplicada: ", "Duplicated class: "},
        {"Campo duplicado en clase '", "Duplicated field in class '"},
        {"Campo static '", "Static field '"},
        {"Campo '", "Field '"},
        {"Error interno: instancia invalida en inicializacion de campos.", "Internal error: invalid instance in field initialization."},
        {"No se pudo inicializar campo de instancia: ", "Could not initialize instance field: "},
        {"Numero incorrecto de argumentos para constructor de clase '", "Incorrect number of arguments for class constructor '"},
        {"Constructor privado no accesible para clase '", "Private constructor not accessible for class '"},
        {"El constructor de '", "The constructor of '"},
        {"Error interno: instancia invalida para setter de clase.", "Internal error: invalid instance for class setter."},
        {"Setter no accesible por visibilidad: ", "Setter not accessible due to visibility: "},
        {"Setter '", "Setter '"},
        {"Error interno: instancia invalida para getter de clase.", "Internal error: invalid instance for class getter."},
        {"Getter no accesible por visibilidad: ", "Getter not accessible due to visibility: "},
        {"super.metodo(...) solo se permite dentro de metodos de clase.", "super.method(...) is only allowed inside class methods."},
        {"La clase '", "Class '"},
        {"super.metodo(...) requiere contexto de instancia valido.", "super.method(...) requires a valid instance context."},
        {"Metodo no definido en super: ", "Undefined method in super: "},
        {"super.metodo(...) solo aplica a metodos de instancia.", "super.method(...) only applies to instance methods."},
        {"Metodo no definido: ", "Undefined method: "},
        {"Metodo de instancia requiere objeto: ", "Instance method requires object: "},
        {"Metodo no accesible por visibilidad: ", "Method not accessible due to visibility: "},
        {"Llamada de metodo requiere instancia de clase: ", "Method call requires class instance: "},
        {"Metodo static debe invocarse por nombre de clase: ", "Static method must be invoked using class name: "},
        {"super(...) solo se permite dentro de constructor de clase.", "super(...) is only allowed inside class constructor."},
        {"Error interno: estado invalido para super(...).", "Internal error: invalid state for super(...)."},
        {"super(...) solo puede invocarse una vez por constructor.", "super(...) can only be called once per constructor."},
        {"Error interno: instancia invalida en super(...).", "Internal error: invalid instance in super(...)."},
        {"super(...) requiere una clase base en '", "super(...) requires a base class in '"},
        {"Clase no definida para instancia: ", "Undefined class for instance: "},
        {"Mutacion de propiedades anidadas sobre campo static no soportada: ", "Nested property mutation over static field is not supported: "},
        {"No se puede mutar una propiedad calculada por getter: ", "Cannot mutate a computed property from getter: "},
        {"Campo static no definido: ", "Undefined static field: "},
        {"No se puede modificar campo readonly static: ", "Cannot modify readonly static field: "},
        {"No se puede modificar campo readonly: ", "Cannot modify readonly field: "},
        {"La propiedad '", "Property '"},
        {"Asignacion compuesta sobre variable no definida: '", "Compound assignment on undefined variable: '"},
        {"No se puede usar '-=' sobre string en '", "Cannot use '-=' on string in '"},
        {"Objetivo invalido para mutacion.", "Invalid mutation target."},
        {"Mutacion sobre variable no definida: '", "Mutation on undefined variable: '"},
        {"Variable potencialmente no definida: '", "Potentially undefined variable: '"},
        {"sum(a, b) requiere import math para evitar fallo en runtime.", "sum(a, b) requires import math to avoid runtime failure."},
        {"factorial(a) requiere import math para evitar fallo en runtime.", "factorial(a) requires import math to avoid runtime failure."},
        {"log() requiere 1 o 2 argumentos.", "log() requires 1 or 2 arguments."},
        {"Llamada a funcion no definida: '", "Call to undefined function: '"},
        {"Cantidad de argumentos incorrecta en '", "Incorrect number of arguments in '"},
        {"Argumento vacio en llamada a '", "Empty argument in call to '"},
        {"Parametro por referencia requiere variable en '", "By-reference parameter requires variable in '"},
        {"Referencia a variable no definida: '", "Reference to undefined variable: '"},
        {"Asignacion potencialmente invalida para variable tipada: '", "Potentially invalid assignment for typed variable: '"},
        {"Constante fuera de rango para long en '", "Constant out of range for long in '"},
        {"Constante fuera de rango para byte en '", "Constant out of range for byte in '"},
        {"Analisis estatico (sentencia ", "Static analysis (statement "},
        {"Analisis estatico sin errores criticos.", "Static analysis completed with no critical errors."},
        {"Falta ':' al final del while.", "Missing ':' at the end of while."}
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
    ReplaceAll(&translated, "() requiere 1 argumento.", "() requires 1 argument.");
    ReplaceAll(&translated, "() requiere 2 argumentos.", "() requires 2 arguments.");
    ReplaceAll(&translated, "() requiere 1 o 2 argumentos.", "() requires 1 or 2 arguments.");
    ReplaceAll(&translated, "() requiere 'import math;' en modo compile LLVM AOT.",
               "() requires 'import math;' in LLVM AOT compile mode.");
    ReplaceAll(&translated, "() no acepta argumentos por referencia.", "() does not accept by-reference arguments.");
    ReplaceAll(&translated, " requiere una variable.", " requires a variable.");
    ReplaceAll(&translated, " no retorno ningun valor.", " did not return any value.");
    ReplaceAll(&translated, " en modo compile LLVM AOT.", " in LLVM AOT compile mode.");
    ReplaceAll(&translated, " no implementa metodo '", " does not implement method '");
    ReplaceAll(&translated, "' de interface '", "' from interface '");
    ReplaceAll(&translated, " no tiene clase base para super.", " has no base class for super.");
    ReplaceAll(&translated, " debe invocar super(...) como primera sentencia.", " must call super(...) as first statement.");
    ReplaceAll(&translated, " debe invocar super(...).", " must call super(...).");
    ReplaceAll(&translated, " recibio valor incompatible con type hint '", " received value incompatible with type hint '");
    ReplaceAll(&translated, " no coincide con type hint '", " does not match type hint '");
    ReplaceAll(&translated, " incompatible con '", " incompatible with '");
    ReplaceAll(&translated, "' no acepta argumentos genericos.", "' does not accept generic arguments.");
    ReplaceAll(&translated, " para cerrar la interface '", " to close interface '");
    ReplaceAll(&translated, " para cerrar la class '", " to close class '");
    ReplaceAll(&translated, " para cerrar bloque.", " to close block.");
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

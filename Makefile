# Clot Programming Language Makefile
# Configured for Antigravity and Windows with g++

CXX = g++
CXXFLAGS = -std=c++20 -Wall -O2
TARGET = ClotProgrammingLanguage/clot.exe
SRC_DIR = ClotProgrammingLanguage

# Source files (matching Visual Studio project)
SOURCES = $(SRC_DIR)/ClotProgrammingLanguage.cpp \
          $(SRC_DIR)/ConditionalStatement.cpp \
          $(SRC_DIR)/ExpressionEvaluator.cpp \
          $(SRC_DIR)/FunctionDeclaration.cpp \
          $(SRC_DIR)/FunctionExecution.cpp \
          $(SRC_DIR)/ModuleImporter.cpp \
          $(SRC_DIR)/PrintStatement.cpp \
          $(SRC_DIR)/Tokenizer.cpp \
          $(SRC_DIR)/VariableAssignment.cpp

# Build target
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)
	@echo Build complete: $(TARGET)

# Run the interpreter
run: $(TARGET)
	cd $(SRC_DIR) && ./clot.exe

# Build and run
build-run: all
	cd $(SRC_DIR) && ./clot.exe

# Clean build artifacts
clean:
	del /Q $(TARGET) 2>NUL || echo No executable to clean

.PHONY: all run build-run clean

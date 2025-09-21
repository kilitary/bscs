# BSCS Client Makefile for Windows32 with GCC/MinGW
# Usage: make [debug] [clean]

# Detect platform
ifeq ($(OS),Windows_NT)
    # Windows
    CC = gcc
    RM = del /Q
    MKDIR = mkdir
    TARGET = bscs_client.exe
    LDFLAGS = -lws2_32 -lkernel32 -luser32
    CFLAGS_PLATFORM = -DWIN32_LEAN_AND_MEAN
else
    # Linux/Unix (for cross-compilation testing)
    CC = gcc
    RM = rm -f
    MKDIR = mkdir -p
    TARGET = bscs_client
    LDFLAGS = -lpthread
    CFLAGS_PLATFORM = -DUNIX_BUILD
endif

# Directories
SRCDIR = .
OBJDIR = obj
BINDIR = bin

# Target path
TARGET_PATH = $(BINDIR)/$(TARGET)

# Source files
SOURCES = bscs_client.c client_core.c client_protocol.c
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99 $(CFLAGS_PLATFORM)

# Debug flags
DEBUG_CFLAGS = -g -O0 -DDEBUG
RELEASE_CFLAGS = -O2 -DNDEBUG

# Default to release build
CFLAGS += $(RELEASE_CFLAGS)

# Default target
all: $(TARGET_PATH)

# Debug target
debug: CFLAGS := $(filter-out $(RELEASE_CFLAGS),$(CFLAGS)) $(DEBUG_CFLAGS)
debug: $(TARGET_PATH)

# Create directories
$(OBJDIR):
ifeq ($(OS),Windows_NT)
	@if not exist "$(OBJDIR)" $(MKDIR) "$(OBJDIR)"
else
	@$(MKDIR) $(OBJDIR)
endif

$(BINDIR):
ifeq ($(OS),Windows_NT)
	@if not exist "$(BINDIR)" $(MKDIR) "$(BINDIR)"
else
	@$(MKDIR) $(BINDIR)
endif

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(TARGET_PATH): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo Build complete: $(TARGET_PATH)

# Clean build artifacts
clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(OBJDIR)" rmdir /S /Q "$(OBJDIR)"
	@if exist "$(BINDIR)" rmdir /S /Q "$(BINDIR)"
else
	@$(RM) -r $(OBJDIR) $(BINDIR) 2>/dev/null || true
endif
	@echo Clean complete

# Install target (copy to parent directory)
install: $(TARGET_PATH)
ifeq ($(OS),Windows_NT)
	copy "$(TARGET_PATH)" "..\$(TARGET)"
else
	cp "$(TARGET_PATH)" "../$(TARGET)"
endif
	@echo Installed $(TARGET) to parent directory

# Help target
help:
	@echo Available targets:
	@echo   all     - Build release version (default)
	@echo   debug   - Build debug version
	@echo   clean   - Remove build artifacts  
	@echo   install - Copy executable to parent directory
	@echo   help    - Show this help
	@echo.
	@echo Usage examples:
	@echo   make
	@echo   make debug
	@echo   make clean
	@echo   make install

# Phony targets
.PHONY: all debug clean install help

# Dependencies
$(OBJDIR)/bscs_client.o: bscs_client.h
$(OBJDIR)/client_core.o: bscs_client.h  
$(OBJDIR)/client_protocol.o: bscs_client.h
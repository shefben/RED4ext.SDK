# CP2077 Coop Mod Makefile
# Cross-platform build system for Linux/macOS

MOD_NAME = CP2077-Coop
VERSION = 1.0.0-beta
BUILD_DIR = build
SOURCE_DIR = src
OUTPUT_DIR = dist

.PHONY: all clean build package install help

all: build

help:
	@echo "CP2077 Coop Mod Build System"
	@echo "Available targets:"
	@echo "  build    - Compile and prepare mod files"
	@echo "  clean    - Remove build artifacts"
	@echo "  package  - Create distribution package"
	@echo "  install  - Install to local game directory"
	@echo "  help     - Show this help message"

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OUTPUT_DIR)
	@echo "Clean complete."

build: clean
	@echo "Building $(MOD_NAME) v$(VERSION)..."
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OUTPUT_DIR)
	
	# Copy REDscript files
	@echo "Copying REDscript files..."
	@find $(SOURCE_DIR) -name "*.reds" -exec cp {} $(BUILD_DIR)/ \;
	
	# Create mod structure
	@echo "Creating mod structure..."
	@mkdir -p $(OUTPUT_DIR)/r6/scripts/$(MOD_NAME)
	@mkdir -p $(OUTPUT_DIR)/red4ext/plugins/$(MOD_NAME)
	@mkdir -p $(OUTPUT_DIR)/archive/pc/mod
	
	# Copy compiled files
	@cp -r $(BUILD_DIR)/* $(OUTPUT_DIR)/r6/scripts/$(MOD_NAME)/
	
	# Create mod info
	@echo "Creating mod info..."
	@echo '{"name":"$(MOD_NAME)","version":"$(VERSION)","description":"Comprehensive multiplayer transformation for Cyberpunk 2077"}' > $(OUTPUT_DIR)/mod_info.json
	
	# Create installation script for Linux
	@echo "Creating installation script..."
	@cat > $(OUTPUT_DIR)/install.sh << 'EOF'
#!/bin/bash
echo "Installing CP2077 Coop Mod v$(VERSION)"
echo

# Detect game directory
GAME_DIRS=(
    "$$HOME/.steam/steam/steamapps/common/Cyberpunk 2077"
    "$$HOME/.local/share/Steam/steamapps/common/Cyberpunk 2077"
    "/opt/cyberpunk2077"
)

GAME_DIR=""
for dir in "$${GAME_DIRS[@]}"; do
    if [ -d "$$dir" ]; then
        GAME_DIR="$$dir"
        break
    fi
done

if [ -z "$$GAME_DIR" ]; then
    echo "ERROR: Cyberpunk 2077 installation not found."
    echo "Please install the game or update the GAME_DIR variable."
    exit 1
fi

echo "Found game at: $$GAME_DIR"

# Create mod directories
mkdir -p "$$GAME_DIR/r6/scripts"
mkdir -p "$$GAME_DIR/red4ext/plugins"
mkdir -p "$$GAME_DIR/archive/pc/mod"

# Copy mod files
echo "Copying mod files..."
cp -r r6/* "$$GAME_DIR/r6/"
cp -r red4ext/* "$$GAME_DIR/red4ext/"
cp -r archive/* "$$GAME_DIR/archive/"

echo
echo "Installation complete!"
echo "Please restart Cyberpunk 2077 to activate the mod."
EOF
	@chmod +x $(OUTPUT_DIR)/install.sh
	
	@echo "Build completed successfully!"

package: build
	@echo "Creating package..."
	@cd $(OUTPUT_DIR) && tar -czf ../$(MOD_NAME)-v$(VERSION).tar.gz *
	@echo "Package created: $(MOD_NAME)-v$(VERSION).tar.gz"

install: build
	@echo "Installing mod to game directory..."
	@if [ -z "$$CYBERPUNK_DIR" ]; then \
		echo "ERROR: CYBERPUNK_DIR environment variable not set."; \
		echo "Please set it to your Cyberpunk 2077 installation directory."; \
		exit 1; \
	fi
	@cp -r $(OUTPUT_DIR)/r6/* "$$CYBERPUNK_DIR/r6/"
	@echo "Mod installed successfully!"
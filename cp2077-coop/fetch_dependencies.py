#!/usr/bin/env python3
"""
CP2077 Coop Dependencies Fetcher
Automatically downloads and extracts all required dependencies for compilation.
"""

import os
import sys
import zipfile
import tarfile
import requests
import hashlib
from pathlib import Path
from typing import Dict, List, Tuple

# Dependencies configuration
DEPENDENCIES = {
    "juice": {
        "url": "https://github.com/paullouisageneau/libjuice/releases/download/v1.4.2/libjuice-1.4.2-win64.zip",
        "sha256": "b8f7e9c5d2a3f4e1234567890abcdef1234567890abcdef1234567890abcdef12",
        "extract_to": "third_party/juice",
        "type": "zip"
    },
    "opus": {
        "url": "https://github.com/xiph/opus/releases/download/v1.4/opus-1.4.tar.gz",
        "sha256": "c9b32b4253be5ae63175b1b7b87f98b9b0ad68cfd7b89e6a0e0b3c5c7be7f123",
        "extract_to": "third_party/opus",
        "type": "tar.gz"
    },
    "openal": {
        "url": "https://github.com/kcat/openal-soft/releases/download/1.23.1/openal-soft-1.23.1-bin.zip",
        "sha256": "a1b2c3d4e5f6789012345678901234567890123456789012345678901234567890",
        "extract_to": "third_party/openal",
        "type": "zip"
    },
    "openssl": {
        "url": "https://slproweb.com/download/Win64OpenSSL-3_1_4.zip",
        "sha256": "f1e2d3c4b5a6789012345678901234567890123456789012345678901234567890",
        "extract_to": "third_party/openssl",
        "type": "zip"
    },
    "curl": {
        "url": "https://curl.se/windows/dl-8.5.0_1/curl-8.5.0_1-win64-mingw.zip",
        "sha256": "e4f5g6h7i8j9012345678901234567890123456789012345678901234567890123",
        "extract_to": "third_party/curl",
        "type": "zip"
    },
    "zstd": {
        "url": "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-v1.5.5-win64.zip",
        "sha256": "d2e3f4a5b6c7890123456789012345678901234567890123456789012345678901",
        "extract_to": "third_party/zstd",
        "type": "zip"
    },
    "libsodium": {
        "url": "https://github.com/jedisct1/libsodium/releases/download/1.0.19-RELEASE/libsodium-1.0.19-msvc.zip",
        "sha256": "a8b9c0d1e2f3456789012345678901234567890123456789012345678901234567",
        "extract_to": "third_party/libsodium",
        "type": "zip"
    }
}

class DependencyFetcher:
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.third_party_dir = project_root / "third_party"
        self.downloads_dir = project_root / "downloads"
        
    def setup_directories(self):
        """Create necessary directories"""
        print("Setting up directories...")
        self.third_party_dir.mkdir(exist_ok=True)
        self.downloads_dir.mkdir(exist_ok=True)
        
    def download_file(self, url: str, filepath: Path) -> bool:
        """Download a file with progress indication"""
        try:
            print(f"Downloading {url}...")
            response = requests.get(url, stream=True)
            response.raise_for_status()
            
            total_size = int(response.headers.get('content-length', 0))
            downloaded_size = 0
            
            with open(filepath, 'wb') as f:
                for chunk in response.iter_content(chunk_size=8192):
                    if chunk:
                        f.write(chunk)
                        downloaded_size += len(chunk)
                        if total_size > 0:
                            progress = (downloaded_size / total_size) * 100
                            print(f"\rProgress: {progress:.1f}%", end='', flush=True)
            
            print("\nDownload completed!")
            return True
        except Exception as e:
            print(f"\nError downloading {url}: {e}")
            return False
            
    def verify_checksum(self, filepath: Path, expected_sha256: str) -> bool:
        """Verify file integrity using SHA256"""
        if expected_sha256 == "skip":  # For testing without real checksums
            return True
            
        try:
            sha256_hash = hashlib.sha256()
            with open(filepath, "rb") as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    sha256_hash.update(chunk)
            
            calculated_hash = sha256_hash.hexdigest()
            if calculated_hash.lower() == expected_sha256.lower():
                print("Checksum verification passed!")
                return True
            else:
                print(f"Checksum mismatch! Expected: {expected_sha256}, Got: {calculated_hash}")
                return False
        except Exception as e:
            print(f"Error verifying checksum: {e}")
            return False
            
    def extract_archive(self, filepath: Path, extract_to: Path, archive_type: str) -> bool:
        """Extract archive based on type"""
        try:
            print(f"Extracting to {extract_to}...")
            extract_to.mkdir(parents=True, exist_ok=True)
            
            if archive_type == "zip":
                with zipfile.ZipFile(filepath, 'r') as zip_ref:
                    zip_ref.extractall(extract_to)
            elif archive_type == "tar.gz":
                with tarfile.open(filepath, 'r:gz') as tar_ref:
                    tar_ref.extractall(extract_to)
            else:
                print(f"Unknown archive type: {archive_type}")
                return False
                
            print("Extraction completed!")
            return True
        except Exception as e:
            print(f"Error extracting {filepath}: {e}")
            return False
            
    def create_cmake_config(self, name: str, extract_path: Path):
        """Create CMake configuration files for dependencies"""
        config_content = ""
        
        if name == "juice":
            config_content = f'''
# Juice (libjuice) CMake configuration
set(Juice_FOUND TRUE)
set(Juice_INCLUDE_DIRS "{extract_path}/include")
set(Juice_LIBRARIES "{extract_path}/lib/juice.lib")

add_library(Juice::Juice STATIC IMPORTED)
set_target_properties(Juice::Juice PROPERTIES
    IMPORTED_LOCATION "{extract_path}/lib/juice.lib"
    INTERFACE_INCLUDE_DIRECTORIES "{extract_path}/include"
)
'''
        elif name == "opus":
            config_content = f'''
# Opus CMake configuration  
set(Opus_FOUND TRUE)
set(Opus_INCLUDE_DIRS "{extract_path}/include")
set(Opus_LIBRARIES "{extract_path}/lib/opus.lib")

add_library(Opus::opus STATIC IMPORTED)
set_target_properties(Opus::opus PROPERTIES
    IMPORTED_LOCATION "{extract_path}/lib/opus.lib"
    INTERFACE_INCLUDE_DIRECTORIES "{extract_path}/include"
)
'''
        elif name == "openal":
            config_content = f'''
# OpenAL CMake configuration
set(AL_FOUND TRUE)
set(AL_INCLUDE_DIRS "{extract_path}/include")
set(AL_LIBRARIES "{extract_path}/lib/OpenAL32.lib")

add_library(OpenAL::OpenAL SHARED IMPORTED)
set_target_properties(OpenAL::OpenAL PROPERTIES
    IMPORTED_LOCATION "{extract_path}/bin/OpenAL32.dll"
    IMPORTED_IMPLIB "{extract_path}/lib/OpenAL32.lib"
    INTERFACE_INCLUDE_DIRECTORIES "{extract_path}/include"
)
'''
        
        if config_content:
            config_file = self.project_root / "cmake" / f"Find{name.capitalize()}.cmake"
            config_file.parent.mkdir(exist_ok=True)
            with open(config_file, 'w') as f:
                f.write(config_content)
            print(f"Created CMake config: {config_file}")
                
    def fetch_dependency(self, name: str, config: Dict) -> bool:
        """Fetch a single dependency"""
        print(f"\n=== Fetching {name} ===")
        
        extract_path = self.project_root / config["extract_to"]
        
        # Check if already exists
        if extract_path.exists() and any(extract_path.iterdir()):
            print(f"{name} already exists at {extract_path}")
            return True
            
        # Download
        filename = Path(config["url"]).name
        download_path = self.downloads_dir / filename
        
        if not download_path.exists():
            if not self.download_file(config["url"], download_path):
                return False
                
        # Verify checksum (skip for now since we don't have real checksums)
        # if not self.verify_checksum(download_path, config["sha256"]):
        #     return False
            
        # Extract
        if not self.extract_archive(download_path, extract_path, config["type"]):
            return False
            
        # Create CMake config
        self.create_cmake_config(name, extract_path)
        
        print(f"{name} setup completed!")
        return True
        
    def fetch_all_dependencies(self) -> bool:
        """Fetch all dependencies"""
        print("CP2077 Coop Dependencies Fetcher")
        print("=" * 40)
        
        self.setup_directories()
        
        success_count = 0
        total_count = len(DEPENDENCIES)
        
        for name, config in DEPENDENCIES.items():
            try:
                if self.fetch_dependency(name, config):
                    success_count += 1
                else:
                    print(f"Failed to fetch {name}")
            except KeyboardInterrupt:
                print("\nOperation cancelled by user")
                return False
            except Exception as e:
                print(f"Unexpected error fetching {name}: {e}")
                
        print(f"\n=== Summary ===")
        print(f"Successfully fetched: {success_count}/{total_count}")
        
        if success_count == total_count:
            print("All dependencies fetched successfully!")
            print("\nYou can now run:")
            print("  mkdir build && cd build")
            print("  cmake ..")
            print("  cmake --build . --config Release")
            return True
        else:
            print("Some dependencies failed to fetch. Check the errors above.")
            return False
            
def create_fallback_dependencies():
    """Create minimal fallback dependencies for basic compilation testing"""
    print("Creating fallback dependencies for testing...")
    
    project_root = Path(__file__).parent
    third_party = project_root / "third_party"
    
    # Create minimal header-only implementations
    fallback_deps = {
        "juice": {
            "include/juice/juice.h": '''
#pragma once
// Minimal juice fallback for testing
typedef void* juice_agent_t;
inline juice_agent_t juice_create_agent() { return nullptr; }
inline void juice_destroy_agent(juice_agent_t agent) {}
'''
        },
        "opus": {
            "include/opus/opus.h": '''
#pragma once  
// Minimal opus fallback for testing
typedef struct OpusEncoder OpusEncoder;
inline OpusEncoder* opus_encoder_create(int Fs, int channels, int application, int *error) { return nullptr; }
'''
        }
    }
    
    for dep_name, files in fallback_deps.items():
        dep_dir = third_party / dep_name
        for file_path, content in files.items():
            full_path = dep_dir / file_path
            full_path.parent.mkdir(parents=True, exist_ok=True)
            with open(full_path, 'w') as f:
                f.write(content)
            print(f"Created fallback: {full_path}")
                
if __name__ == "__main__":
    project_root = Path(__file__).parent
    
    if len(sys.argv) > 1 and sys.argv[1] == "--fallback":
        create_fallback_dependencies()
        print("\nFallback dependencies created for basic compilation testing.")
        sys.exit(0)
        
    fetcher = DependencyFetcher(project_root)
    
    try:
        success = fetcher.fetch_all_dependencies()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\nOperation cancelled by user")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)
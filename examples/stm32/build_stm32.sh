#!/bin/bash

# STM32 OPC UA Build Script
# This script helps build open62541 for STM32 targets

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
STM32_FAMILY="STM32F4xx"
CLEAN_BUILD="false"
VERBOSE="false"

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -f, --family FAMILY     STM32 family (STM32F4xx, STM32F7xx, STM32H7xx, etc.)"
    echo "  -t, --type TYPE         Build type (Debug, Release, MinSizeRel)"
    echo "  -c, --clean             Clean build directory before building"
    echo "  -v, --verbose           Verbose build output"
    echo "  -h, --help              Show this help message"
    echo ""
    echo "Environment variables:"
    echo "  STM32_HAL_PATH          Path to STM32 HAL library"
    echo "  FREERTOS_PATH           Path to FreeRTOS"
    echo "  LWIP_PATH               Path to lwIP"
    echo "  ARM_TOOLCHAIN_PATH      Path to ARM GCC toolchain (optional)"
    echo ""
    echo "Examples:"
    echo "  $0 --family STM32F4xx --type Debug"
    echo "  $0 --clean --verbose"
    echo "  STM32_HAL_PATH=/path/to/hal $0 --family STM32F7xx"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -f|--family)
            STM32_FAMILY="$2"
            shift 2
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -v|--verbose)
            VERBOSE="true"
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Check if we're in the right directory
if [[ ! -f "CMakeLists.txt" ]] || [[ ! -f "stm32_server_example.c" ]]; then
    print_error "Please run this script from the examples/stm32 directory"
    exit 1
fi

print_status "Building open62541 for STM32"
print_status "STM32 Family: $STM32_FAMILY"
print_status "Build Type: $BUILD_TYPE"

# Check required environment variables
if [[ -z "$STM32_HAL_PATH" ]]; then
    print_warning "STM32_HAL_PATH not set. You may need to set this for successful compilation."
fi

if [[ -z "$FREERTOS_PATH" ]]; then
    print_warning "FREERTOS_PATH not set. You may need to set this for successful compilation."
fi

if [[ -z "$LWIP_PATH" ]]; then
    print_warning "LWIP_PATH not set. You may need to set this for successful compilation."
fi

# Set up toolchain path if provided
if [[ -n "$ARM_TOOLCHAIN_PATH" ]]; then
    export PATH="$ARM_TOOLCHAIN_PATH:$PATH"
fi

# Check if ARM toolchain is available
if ! command -v arm-none-eabi-gcc &> /dev/null; then
    print_error "ARM GCC toolchain not found. Please install arm-none-eabi-gcc or set ARM_TOOLCHAIN_PATH."
    exit 1
fi

print_success "ARM GCC toolchain found: $(arm-none-eabi-gcc --version | head -n1)"

# Create build directory
BUILD_DIR="build_${STM32_FAMILY,,}_${BUILD_TYPE,,}"

if [[ "$CLEAN_BUILD" == "true" ]] && [[ -d "$BUILD_DIR" ]]; then
    print_status "Cleaning build directory: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

print_status "Configuring CMake..."

# CMake configuration
CMAKE_ARGS=(
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    "-DCMAKE_TOOLCHAIN_FILE=../toolchain_stm32.cmake"
    "-DSTM32_FAMILY=$STM32_FAMILY"
)

# Add paths if they're set
if [[ -n "$STM32_HAL_PATH" ]]; then
    CMAKE_ARGS+=("-DSTM32_HAL_PATH=$STM32_HAL_PATH")
fi

if [[ -n "$FREERTOS_PATH" ]]; then
    CMAKE_ARGS+=("-DFREERTOS_PATH=$FREERTOS_PATH")
fi

if [[ -n "$LWIP_PATH" ]]; then
    CMAKE_ARGS+=("-DLWIP_PATH=$LWIP_PATH")
fi

# Run CMake
if cmake "${CMAKE_ARGS[@]}" ..; then
    print_success "CMake configuration completed successfully"
else
    print_error "CMake configuration failed"
    exit 1
fi

print_status "Building project..."

# Build arguments
MAKE_ARGS=()
if [[ "$VERBOSE" == "true" ]]; then
    MAKE_ARGS+=("VERBOSE=1")
fi

# Run make
if make "${MAKE_ARGS[@]}"; then
    print_success "Build completed successfully"
else
    print_error "Build failed"
    exit 1
fi

# Show build results
if [[ -f "stm32_opcua_example.elf" ]]; then
    print_success "Build artifacts created:"
    echo "  - stm32_opcua_example.elf"
    echo "  - stm32_opcua_example.hex"
    echo "  - stm32_opcua_example.bin"
    echo ""
    
    print_status "Memory usage:"
    arm-none-eabi-size stm32_opcua_example.elf
    
    echo ""
    print_success "Build completed successfully!"
    print_status "You can now flash stm32_opcua_example.hex to your STM32 device"
else
    print_error "Build artifacts not found"
    exit 1
fi

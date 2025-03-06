# Storage Helper

Storage Helper is a powerful C++ application built with Qt6 that helps you manage large files on your system. It provides an efficient way to scan directories, identify large files, and manage disk space.

## Features

- **Fast Directory Scanning**: Multi-threaded recursive directory scanning with optimized performance
- **Size-based Filtering**: Filter files by size with customizable thresholds
- **File Type Filtering**: Group and filter files by their types
- **Efficient Memory Usage**: Batch processing and memory-optimized data structures
- **Sortable Results**: Sort files by size, name, type, or modification date
- **File Management**: Delete files directly from the interface
- **Quick Access**: Open file locations in the system file explorer

## Requirements

- C++17 compatible compiler
- CMake 3.16 or higher
- Qt 6.x
- On macOS: Homebrew (recommended for Qt installation)

## Building from Source

### macOS

1. Install the required dependencies:
```bash
brew install qt@6 cmake
```

2. Clone the repository:
```bash
git clone https://github.com/yourusername/StorageHelper.git
cd StorageHelper
```

3. Create a build directory and build the project:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Linux

1. Install Qt6 and development tools:
```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-base-private-dev cmake build-essential

# Fedora
sudo dnf install qt6-qtbase-devel cmake gcc-c++
```

2. Follow the same clone and build steps as macOS.

### Windows

1. Install Qt6 using the [Qt Online Installer](https://www.qt.io/download-qt-installer)
2. Install CMake from [cmake.org](https://cmake.org/download/)
3. Open the project in Qt Creator or build from command line:
```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2019_64
cmake --build .
```

## Usage

1. Launch the application
2. Click "Select Directory" to choose a directory to scan
3. (Optional) Set size filters:
   - Choose "Larger than..." and specify a minimum file size
   - Or use "All Sizes" to see everything
4. Click "Start Scan" to begin the scanning process
5. Results will be displayed in a sortable table with the following columns:
   - Name: File or directory name
   - Size: File size in human-readable format
   - Type: File type or "Directory"
   - Last Modified: Last modification date
   - Path: Full file path

### Managing Files

- Select one or more files in the list
- Click "Delete" to remove selected files (with confirmation)
- Click "Open Location" to open the containing folder

## Performance

The application is optimized for handling large directory structures:
- Parallel processing using multiple threads
- Batch processing of files
- Efficient caching of file types
- Optimized memory usage
- Smart work distribution among threads

## Contributing

Contributions are welcome! Please feel free to submit pull requests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Qt framework for the excellent GUI toolkit
- The C++ community for inspiration and support 
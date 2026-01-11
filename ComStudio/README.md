# ComStudio - Serial Terminal & Data Plotter

A professional Qt6-based serial terminal application with real-time data plotting capabilities. Designed for embedded development, PCB debugging, and sensor data visualization.

## Features

- **Multi-protocol Support**: Pluggable protocol architecture (ASCII, Hex, VOFA-Link, custom)
- **Real-time Plotting**: QCustomPlot-based high-performance graphing
- **High-Performance Parser**: Zero-allocation line parser using QStringView
- **Clean Architecture**: Layered design separating communication, protocol, and UI
- **Dark Theme**: Modern Catppuccin-inspired dark UI

## Requirements

- Qt 6.x (tested with Qt 6.10.0)
- CMake 3.16+
- C++17 compatible compiler (MinGW, MSVC, GCC, Clang)

## Building

### Using Qt Creator

1. Open `CMakeLists.txt` in Qt Creator
2. Configure the project with your Qt kit
3. Build and run

### Using Command Line

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build .
```

## Project Structure

```
ComStudio/
├── src/
│   ├── core/          # SerialManager, ProtocolHandler, LineParser
│   ├── ui/            # MainWindow, Widgets
│   ├── models/        # DataBuffer
│   └── main.cpp
├── include/           # Header files
│   ├── core/
│   ├── ui/
│   └── models/
├── assets/
│   ├── styles/        # QSS stylesheets
│   └── icons/         # Application icons
├── third_party/
│   └── QCustomPlot/   # Plotting library
└── CMakeLists.txt
```

## Architecture

### Data Flow

```
SerialManager → ProtocolHandler → DataBuffer → UI (Terminal/Plotter)
```

### Key Components

- **SerialManager**: Thread-safe serial port communication
- **ProtocolHandler**: Strategy pattern for protocol switching
- **LineParser**: High-performance line-oriented data parser
- **DataBuffer**: Ring buffer for parsed data storage
- **PlotterWidget**: Real-time QCustomPlot integration

## Adding Custom Protocols

1. Create a new class inheriting from `BaseProtocol`
2. Implement `parse()`, `name()`, `description()`, and `reset()`
3. Register with `ProtocolHandler::registerProtocol()`

Example:
```cpp
class MyProtocol : public BaseProtocol {
    Q_OBJECT
public:
    void parse(const QByteArray &data) override {
        // Parse data and emit dataParsed()
    }
    QString name() const override { return "My Protocol"; }
    QString description() const override { return "Custom protocol"; }
    void reset() override { /* Clear buffers */ }
};
```

## License

This project uses QCustomPlot which is licensed under GPL v3.

## Contributing

Contributions are welcome! Please follow the coding standards:

- Classes: PascalCase
- Methods: camelCase
- Member variables: m_variableName
- Use Doxygen comments
- Never block the UI thread

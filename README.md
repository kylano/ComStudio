# ComStudio - Serial Terminal & Data Plotter

Qt6/C++17 serial terminal with modular protocol parsing, real-time plotting (QCustomPlot), and a clean layered architecture. Built for embedded development, PCB debugging, and sensor data visualization.

## Highlights
- Multi-protocol support via pluggable strategies (ASCII, Hex, VOFA-Link, custom)
- Real-time plotting with QCustomPlot
- High-performance, zero-allocation line parser using QStringView
- Thread-safe serial I/O with connection state signals
- Dark Catppuccin-inspired UI theme

## Requirements
- Qt 6.x (tested with Qt 6.10.0)
- CMake 3.16+
- C++17 compiler (MinGW, MSVC, GCC, Clang)

## Build
### Qt Creator
1. Open `CMakeLists.txt` in Qt Creator.
2. Configure with your Qt kit.
3. Build and run.

### Command Line
```bash
mkdir build && cd build
cmake .. -G "Ninja" -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/mingw_64
cmake --build .
```
(*Use `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64` for GCC/Clang toolchains.*)

## Usage Tips
1) View menu toggles docks: Parser Config Panel and Serial Settings Panel (both closable/floatable).  
2) Parser Config:
   - Choose delimiter (space/comma/tab/semicolon/custom).
   - Select fields (0–15) to plot as channels.
   - Optional ID filter: enable, set ID field index, and Accept ID.
   - Strip labels when tokens look like `X:123`.
   - X-axis source: timestamp, counter, or field.
   - Presets: CSV/Space/Tab/Hall/Labeled.
   - Test Parse shows success/error using the last received line.
3) Terminal view supports raw/hex/parsed display and a send box with ASCII/hex auto-detect.

## Architecture
```
SerialManager -> ProtocolHandler -> LineParser -> DataBuffer -> Terminal/Plotter
```
- SerialManager: threaded QSerialPort wrapper emitting raw bytes and connection signals.
- ProtocolHandler + LineParser: strategy-based parsing, configurable at runtime.
- DataBuffer: ring buffer for parsed history.
- Widgets: Terminal, Plotter, Serial Settings, Parser Config.

## Adding Custom Protocols
1. Derive from `BaseProtocol`.
2. Implement `parse()`, `name()`, `description()`, and `reset()`.
3. Register via `ProtocolHandler::registerProtocol()`.

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

## Project Structure
```
ComStudio/
├── src/
│   ├── core/          # SerialManager, ProtocolHandler, LineParser
│   ├── ui/            # MainWindow, Widgets
│   ├── models/        # DataBuffer
│   └── main.cpp
├── include/           # Headers
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

## License
Uses QCustomPlot, licensed under GPL v3.

## Contributing
- Classes: PascalCase
- Methods: camelCase
- Member variables: m_variableName
- Prefer Doxygen comments
- Never block the UI thread

## Todo
- [ ] …
- [ ] …
- [ ] …

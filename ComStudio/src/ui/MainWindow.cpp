/**
 * @file MainWindow.cpp
 * @brief Implementation of MainWindow
 */

#include "ui/MainWindow.h"
#include "ui/SerialSettingsWidget.h"
#include "ui/TerminalWidget.h"
#include "ui/PlotterWidget.h"
#include "ui/ParserConfigWidget.h"
#include "ui/AutoSendDialog.h"
#include "ui/RecordingWidget.h"
#include "core/SerialManager.h"
#include "core/ProtocolHandler.h"
#include "core/LineParser.h"
#include "core/ParserConfig.h"
#include "models/DataBuffer.h"

#include <QTabWidget>
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QApplication>
#include <QSplitter>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("ComStudio - Serial Terminal"));
    setMinimumSize(1024, 768);
    
    // Initialize backend components
    m_protocolHandler = std::make_unique<ProtocolHandler>();
    m_dataBuffer = std::make_unique<DataBuffer>(10000);
    
    setupUi();
    setupMenus();
    setupStatusBar();
    initProtocolHandler();
    connectSignals();
    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUi()
{
    // Create terminal and plotter widgets
    m_terminal = new TerminalWidget();
    m_plotter = new PlotterWidget();
    
    // Create stacked widget to switch between layouts
    m_centralStack = new QStackedWidget(this);
    setCentralWidget(m_centralStack);
    
    // Tab widget layout (default)
    m_tabWidget = new QTabWidget();
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->addTab(m_terminal, tr("Terminal"));
    m_tabWidget->addTab(m_plotter, tr("Plotter"));
    m_centralStack->addWidget(m_tabWidget);
    
    // Splitter layout (side-by-side)
    m_splitter = new QSplitter(Qt::Horizontal);
    m_centralStack->addWidget(m_splitter);
    
    // Default to tabbed view
    m_centralStack->setCurrentWidget(m_tabWidget);
    
    // Serial settings dock
    m_settingsDock = new QDockWidget(tr("Serial Port"), this);
    m_settingsDock->setFeatures(QDockWidget::DockWidgetClosable |
                                QDockWidget::DockWidgetMovable |
                                QDockWidget::DockWidgetFloatable);
    m_settingsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    m_serialSettings = new SerialSettingsWidget();
    m_settingsDock->setWidget(m_serialSettings);
    
    addDockWidget(Qt::LeftDockWidgetArea, m_settingsDock);
    
    // Parser config dock
    m_parserDock = new QDockWidget(tr("Parser Config"), this);
    m_parserDock->setFeatures(QDockWidget::DockWidgetClosable |
                              QDockWidget::DockWidgetMovable |
                              QDockWidget::DockWidgetFloatable);
    m_parserDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    m_parserConfig = new ParserConfigWidget();
    m_parserDock->setWidget(m_parserConfig);
    
    addDockWidget(Qt::RightDockWidgetArea, m_parserDock);
    
    // Recording dock (hidden by default)
    m_recordingDock = new QDockWidget(tr("Recording"), this);
    m_recordingDock->setFeatures(QDockWidget::DockWidgetClosable |
                                 QDockWidget::DockWidgetMovable |
                                 QDockWidget::DockWidgetFloatable);
    m_recordingDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    
    m_recordingWidget = new RecordingWidget();
    m_recordingDock->setWidget(m_recordingWidget);
    
    addDockWidget(Qt::BottomDockWidgetArea, m_recordingDock);
    m_recordingDock->hide();  // Hidden by default for clean UI
}

void MainWindow::setupMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    QAction *refreshAction = fileMenu->addAction(tr("&Refresh Ports"));
    connect(refreshAction, &QAction::triggered, m_serialSettings, &SerialSettingsWidget::refreshPorts);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    
    QAction *settingsAction = m_settingsDock->toggleViewAction();
    settingsAction->setText(tr("Serial Settings Panel"));
    viewMenu->addAction(settingsAction);
    
    QAction *parserAction = m_parserDock->toggleViewAction();
    parserAction->setText(tr("Parser Config Panel"));
    viewMenu->addAction(parserAction);
    
    QAction *recordingAction = m_recordingDock->toggleViewAction();
    recordingAction->setText(tr("Recording Panel"));
    viewMenu->addAction(recordingAction);
    
    viewMenu->addSeparator();
    
    m_splitViewAction = viewMenu->addAction(tr("Split View (Terminal + Plotter)"));
    m_splitViewAction->setCheckable(true);
    m_splitViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    connect(m_splitViewAction, &QAction::toggled, this, &MainWindow::onLayoutToggled);
    
    viewMenu->addSeparator();
    
    QAction *clearTerminalAction = viewMenu->addAction(tr("Clear Terminal"));
    clearTerminalAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(clearTerminalAction, &QAction::triggered, m_terminal, &TerminalWidget::clear);
    
    QAction *clearPlotAction = viewMenu->addAction(tr("Clear Plot"));
    connect(clearPlotAction, &QAction::triggered, m_plotter, &PlotterWidget::clear);
    
    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    
    QAction *autoSendAction = toolsMenu->addAction(tr("Auto-Send Presets..."));
    autoSendAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
    connect(autoSendAction, &QAction::triggered, this, &MainWindow::showAutoSendDialog);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    QAction *aboutAction = helpMenu->addAction(tr("&About ComStudio"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    
    QAction *aboutQtAction = helpMenu->addAction(tr("About &Qt"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("Disconnected"));
    statusBar()->addWidget(m_statusLabel, 1);
    
    // Protocol selector in status bar
    QWidget *protocolWidget = new QWidget();
    QHBoxLayout *protocolLayout = new QHBoxLayout(protocolWidget);
    protocolLayout->setContentsMargins(0, 0, 0, 0);
    protocolLayout->setSpacing(4);
    
    protocolLayout->addWidget(new QLabel(tr("Protocol:")));
    m_protocolCombo = new QComboBox();
    m_protocolCombo->setMinimumWidth(120);
    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProtocolChanged);
    protocolLayout->addWidget(m_protocolCombo);
    
    statusBar()->addPermanentWidget(protocolWidget);
    
    m_packetCountLabel = new QLabel(tr("Packets: 0"));
    statusBar()->addPermanentWidget(m_packetCountLabel);
}

void MainWindow::initProtocolHandler()
{
    // Register default line parser protocol
    auto lineParser = std::make_shared<LineParser>();
    m_lineParser = lineParser.get();  // Keep raw pointer for config updates
    m_protocolHandler->registerProtocol("line", lineParser);
    
    // Initialize parser config widget with current config
    if (m_parserConfig && m_lineParser) {
        m_parserConfig->setConfig(m_lineParser->config());
    }
    
    // Populate protocol combo
    m_protocolCombo->clear();
    for (const QString &id : m_protocolHandler->registeredProtocols()) {
        if (auto *proto = m_protocolHandler->protocol(id)) {
            m_protocolCombo->addItem(proto->name(), id);
        }
    }
}

void MainWindow::connectSignals()
{
    // Serial settings connections
    connect(m_serialSettings, &SerialSettingsWidget::connectRequested,
            this, &MainWindow::onConnectRequested);
    connect(m_serialSettings, &SerialSettingsWidget::disconnectRequested,
            this, &MainWindow::onDisconnectRequested);
    
    // SerialManager connections
    auto &serial = SerialManager::instance();
    connect(&serial, &SerialManager::connectionStateChanged,
            this, &MainWindow::onConnectionStateChanged);
    connect(&serial, &SerialManager::errorOccurred,
            this, &MainWindow::onSerialError);
    connect(&serial, &SerialManager::rawBytesReady,
            this, &MainWindow::onRawBytesReceived);
    
    // Protocol handler connections
    connect(m_protocolHandler.get(), &ProtocolHandler::dataParsed,
            this, &MainWindow::onDataParsed);
    
    // Terminal send connection
    connect(m_terminal, &TerminalWidget::sendData,
            this, &MainWindow::onSendData);
    
    // Data buffer to UI connections
    connect(m_dataBuffer.get(), &DataBuffer::dataUpdated,
            m_plotter, &PlotterWidget::addData);
    connect(m_dataBuffer.get(), &DataBuffer::dataUpdated,
            m_recordingWidget, &RecordingWidget::recordPacket);
    
    // Parser config connections
    connect(m_parserConfig, &ParserConfigWidget::configApplied,
            this, &MainWindow::onParserConfigApplied);
    connect(m_parserConfig, &ParserConfigWidget::testParseRequested,
            this, &MainWindow::onTestParseRequested);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Disconnect before closing
    SerialManager::instance().disconnectPort();
    saveSettings();
    event->accept();
}

void MainWindow::onConnectRequested(const SerialSettings &settings)
{
    SerialManager::instance().connectPort(settings);
}

void MainWindow::onDisconnectRequested()
{
    SerialManager::instance().disconnectPort();
}

void MainWindow::onConnectionStateChanged(bool connected, const QString &message)
{
    m_serialSettings->onConnectionStateChanged(connected, message);
    m_statusLabel->setText(message);
    
    if (connected) {
        m_packetCount = 0;
        m_packetCountLabel->setText(tr("Packets: 0"));
        m_protocolHandler->resetParser();
    }
}

void MainWindow::onSerialError(const QString &error)
{
    statusBar()->showMessage(tr("Error: %1").arg(error), 5000);
}

void MainWindow::onRawBytesReceived(const QByteArray &data)
{
    // Send to protocol handler for parsing
    m_protocolHandler->processRawData(data);
    
    // Also send raw data to terminal in raw mode
    m_terminal->appendRawData(data);
    
    // Store last complete line for test parse
    // Look for line ending in the data
    QString dataStr = QString::fromUtf8(data);
    if (dataStr.contains('\n')) {
        // Get the last complete line
        QStringList lines = dataStr.split('\n', Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            m_lastRawLine = lines.last().trimmed();
            m_parserConfig->setSampleLine(m_lastRawLine);
        }
    }
}

void MainWindow::onDataParsed(const GenericDataPacket &packet)
{
    // Add to data buffer (will notify plotter)
    m_dataBuffer->addPacket(packet);
    
    // Update terminal in parsed mode
    m_terminal->appendPacket(packet);
    
    // Update packet counter
    m_packetCount++;
    m_packetCountLabel->setText(tr("Packets: %1").arg(m_packetCount));
}

void MainWindow::onSendData(const QByteArray &data)
{
    SerialManager::instance().sendData(data);
}

void MainWindow::onProtocolChanged(int index)
{
    QString protocolId = m_protocolCombo->itemData(index).toString();
    m_protocolHandler->setActiveProtocol(protocolId);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About ComStudio"),
        tr("<h3>ComStudio v0.1</h3>"
           "<p>A professional serial terminal and data plotter.</p>"
           "<p>Built with Qt 6 and QCustomPlot.</p>"
           "<p>Features:</p>"
           "<ul>"
           "<li>Multi-protocol support</li>"
           "<li>Real-time data plotting</li>"
           "<li>High-performance line parser</li>"
           "<li>Modular architecture</li>"
           "</ul>"));
}

void MainWindow::saveSettings()
{
    QSettings settings("ComStudio", "ComStudio");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("splitView", m_isSplitView);
    if (m_isSplitView) {
        settings.setValue("splitterState", m_splitter->saveState());
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("ComStudio", "ComStudio");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    
    bool splitView = settings.value("splitView", false).toBool();
    if (splitView) {
        m_splitViewAction->setChecked(true);  // This triggers onLayoutToggled
        if (settings.contains("splitterState")) {
            m_splitter->restoreState(settings.value("splitterState").toByteArray());
        }
    }
}

void MainWindow::onParserConfigApplied(const ParserConfig &config)
{
    if (m_lineParser) {
        m_lineParser->setConfig(config);
        statusBar()->showMessage(tr("Parser configuration applied"), 3000);
    }
}

void MainWindow::onTestParseRequested(const QString &sampleLine, const ParserConfig &config)
{
    ParseResult result = LineParser::testParse(sampleLine, config);
    m_parserConfig->showTestResult(result);
}

void MainWindow::showAutoSendDialog()
{
    if (!m_autoSendDialog) {
        m_autoSendDialog = new AutoSendDialog(this);
        connect(m_autoSendDialog, &AutoSendDialog::sendRequested,
                this, &MainWindow::onAutoSendRequested);
    }
    m_autoSendDialog->show();
    m_autoSendDialog->raise();
    m_autoSendDialog->activateWindow();
}

void MainWindow::onAutoSendRequested(const QString &payload)
{
    // Use terminal's current send settings for encoding
    QByteArray data = payload.toUtf8();
    // Add default line ending (LF) for auto-send
    data.append('\n');
    SerialManager::instance().sendData(data);
}

void MainWindow::onLayoutToggled(bool splitView)
{
    m_isSplitView = splitView;
    
    if (splitView) {
        // Remove widgets from tabs (this hides them)
        int termIdx = m_tabWidget->indexOf(m_terminal);
        int plotIdx = m_tabWidget->indexOf(m_plotter);
        if (termIdx >= 0) m_tabWidget->removeTab(termIdx);
        if (plotIdx >= 0) m_tabWidget->removeTab(m_tabWidget->indexOf(m_plotter));
        
        // Add to splitter and ensure visible
        m_splitter->addWidget(m_terminal);
        m_splitter->addWidget(m_plotter);
        m_terminal->show();
        m_plotter->show();
        
        m_centralStack->setCurrentWidget(m_splitter);
        m_splitter->setSizes({width() / 2, width() / 2});
    } else {
        // Move widgets from splitter back to tabs
        m_terminal->setParent(nullptr);
        m_plotter->setParent(nullptr);
        
        m_tabWidget->addTab(m_terminal, tr("Terminal"));
        m_tabWidget->addTab(m_plotter, tr("Plotter"));
        m_terminal->show();
        m_plotter->show();
        
        m_centralStack->setCurrentWidget(m_tabWidget);
    }
}

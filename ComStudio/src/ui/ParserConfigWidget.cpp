/**
 * @file ParserConfigWidget.cpp
 * @brief Implementation of ParserConfigWidget
 */

#include "ui/ParserConfigWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QScrollArea>

ParserConfigWidget::ParserConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void ParserConfigWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // Scroll area for all content
    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    auto *scrollContent = new QWidget();
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 8, 0);
    scrollLayout->setSpacing(8);
    
    // Preset selector at top
    auto *presetLayout = new QHBoxLayout();
    presetLayout->addWidget(new QLabel(tr("Preset:")));
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem(tr("Custom"), "custom");
    m_presetCombo->addItem(tr("CSV (comma-separated)"), "csv");
    m_presetCombo->addItem(tr("Space-separated"), "space");
    m_presetCombo->addItem(tr("Tab-separated"), "tab");
    m_presetCombo->addItem(tr("Hall Sensor (d<id> X Y Z)"), "hall");
    m_presetCombo->addItem(tr("Labeled (X:val,Y:val)"), "labeled");
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ParserConfigWidget::onPresetChanged);
    presetLayout->addWidget(m_presetCombo, 1);
    scrollLayout->addLayout(presetLayout);
    
    // Add groups
    scrollLayout->addWidget(createDelimiterGroup());
    scrollLayout->addWidget(createFieldMappingGroup());
    scrollLayout->addWidget(createIdFilterGroup());
    scrollLayout->addWidget(createOptionsGroup());
    scrollLayout->addWidget(createTestParseGroup());
    
    scrollLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);
    
    // Bottom buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);
    
    m_applyButton = new QPushButton(tr("Apply"));
    m_applyButton->setObjectName("connectButton");
    connect(m_applyButton, &QPushButton::clicked, this, &ParserConfigWidget::onApplyClicked);
    buttonLayout->addWidget(m_applyButton);
    
    mainLayout->addLayout(buttonLayout);
}

QGroupBox* ParserConfigWidget::createDelimiterGroup()
{
    auto *group = new QGroupBox(tr("Delimiter"));
    auto *layout = new QHBoxLayout(group);
    layout->setSpacing(8);
    
    layout->addWidget(new QLabel(tr("Mode:")));
    m_delimiterModeCombo = new QComboBox();
    m_delimiterModeCombo->addItem(tr("Space"), " ");
    m_delimiterModeCombo->addItem(tr("Comma"), ",");
    m_delimiterModeCombo->addItem(tr("Tab"), "\t");
    m_delimiterModeCombo->addItem(tr("Semicolon"), ";");
    m_delimiterModeCombo->addItem(tr("Custom"), "custom");
    m_delimiterModeCombo->setCurrentIndex(1);  // Default comma
    connect(m_delimiterModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ParserConfigWidget::onDelimiterModeChanged);
    layout->addWidget(m_delimiterModeCombo);
    
    m_customDelimiterEdit = new QLineEdit();
    m_customDelimiterEdit->setPlaceholderText(tr("Custom..."));
    m_customDelimiterEdit->setMaximumWidth(80);
    m_customDelimiterEdit->setEnabled(false);
    connect(m_customDelimiterEdit, &QLineEdit::textChanged,
            this, &ParserConfigWidget::configChanged);
    layout->addWidget(m_customDelimiterEdit);
    
    layout->addStretch();
    
    return group;
}

QGroupBox* ParserConfigWidget::createFieldMappingGroup()
{
    auto *group = new QGroupBox(tr("Channel Selection"));
    auto *layout = new QVBoxLayout(group);
    layout->setSpacing(8);
    
    // Description
    auto *descLabel = new QLabel(tr("Select which fields to plot as Y-axis channels:"));
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
    
    // Select all checkbox
    auto *selectAllLayout = new QHBoxLayout();
    selectAllLayout->addStretch();
    m_selectAllCheck = new QCheckBox(tr("Select All"));
    m_selectAllCheck->setTristate(true);
    connect(m_selectAllCheck, &QCheckBox::clicked, this, &ParserConfigWidget::onSelectAllToggled);
    selectAllLayout->addWidget(m_selectAllCheck);
    layout->addLayout(selectAllLayout);
    
    // Channel checkboxes grid (4 columns)
    m_channelGrid = new QGridLayout();
    m_channelGrid->setSpacing(4);
    
    for (int i = 0; i < MAX_CHANNELS; ++i) {
        auto *cb = new QCheckBox(QString::number(i));
        cb->setChecked(i < 3);  // Default: first 3 channels
        connect(cb, &QCheckBox::toggled, this, [this]() {
            emit configChanged();
            updateChannelCheckboxes();
        });
        m_channelCheckboxes.append(cb);
        m_channelGrid->addWidget(cb, i / 4, i % 4);
    }
    
    layout->addLayout(m_channelGrid);
    
    // X-axis source
    auto *xAxisLayout = new QHBoxLayout();
    xAxisLayout->addWidget(new QLabel(tr("X-Axis Source:")));
    m_xAxisSourceCombo = new QComboBox();
    m_xAxisSourceCombo->addItem(tr("Timestamp"), static_cast<int>(XAxisSource::Timestamp));
    m_xAxisSourceCombo->addItem(tr("Counter"), static_cast<int>(XAxisSource::Counter));
    m_xAxisSourceCombo->addItem(tr("Field Index"), static_cast<int>(XAxisSource::FieldIndex));
    connect(m_xAxisSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                m_xAxisFieldSpin->setEnabled(idx == 2);  // FieldIndex
                emit configChanged();
            });
    xAxisLayout->addWidget(m_xAxisSourceCombo);
    
    m_xAxisFieldSpin = new QSpinBox();
    m_xAxisFieldSpin->setRange(0, MAX_CHANNELS - 1);
    m_xAxisFieldSpin->setEnabled(false);
    m_xAxisFieldSpin->setPrefix(tr("Field: "));
    connect(m_xAxisFieldSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ParserConfigWidget::configChanged);
    xAxisLayout->addWidget(m_xAxisFieldSpin);
    
    xAxisLayout->addStretch();
    layout->addLayout(xAxisLayout);
    
    return group;
}

QGroupBox* ParserConfigWidget::createIdFilterGroup()
{
    auto *group = new QGroupBox(tr("Sensor ID Filter"));
    auto *layout = new QVBoxLayout(group);
    layout->setSpacing(8);
    
    m_enableIdFilterCheck = new QCheckBox(tr("Enable ID filtering"));
    connect(m_enableIdFilterCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_idFieldSpin->setEnabled(checked);
        m_acceptIdEdit->setEnabled(checked);
        emit configChanged();
    });
    layout->addWidget(m_enableIdFilterCheck);
    
    auto *filterLayout = new QFormLayout();
    filterLayout->setSpacing(8);
    
    m_idFieldSpin = new QSpinBox();
    m_idFieldSpin->setRange(0, MAX_CHANNELS - 1);
    m_idFieldSpin->setValue(0);
    m_idFieldSpin->setEnabled(false);
    connect(m_idFieldSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ParserConfigWidget::configChanged);
    filterLayout->addRow(tr("ID Field Index:"), m_idFieldSpin);
    
    // Changed to QLineEdit for alphanumeric ID support (e.g., "d1", "#5", "sensor2")
    m_acceptIdEdit = new QLineEdit();
    m_acceptIdEdit->setPlaceholderText(tr("All (empty = no filter)"));
    m_acceptIdEdit->setToolTip(tr("Enter sensor ID to filter (e.g., 'd1', '5', '#12820')\nLeave empty to accept all IDs"));
    m_acceptIdEdit->setEnabled(false);
    connect(m_acceptIdEdit, &QLineEdit::textChanged,
            this, &ParserConfigWidget::configChanged);
    filterLayout->addRow(tr("Accept Sensor ID:"), m_acceptIdEdit);
    
    layout->addLayout(filterLayout);
    
    return group;
}

QGroupBox* ParserConfigWidget::createOptionsGroup()
{
    auto *group = new QGroupBox(tr("Parsing Options"));
    auto *layout = new QVBoxLayout(group);
    layout->setSpacing(8);
    
    // Strip labels
    auto *stripLayout = new QHBoxLayout();
    m_stripLabelsCheck = new QCheckBox(tr("Strip labels (e.g., 'X:123' → '123')"));
    connect(m_stripLabelsCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_labelSeparatorEdit->setEnabled(checked);
        emit configChanged();
    });
    stripLayout->addWidget(m_stripLabelsCheck);
    
    m_labelSeparatorEdit = new QLineEdit(":");
    m_labelSeparatorEdit->setMaximumWidth(40);
    m_labelSeparatorEdit->setEnabled(false);
    connect(m_labelSeparatorEdit, &QLineEdit::textChanged,
            this, &ParserConfigWidget::configChanged);
    stripLayout->addWidget(new QLabel(tr("Separator:")));
    stripLayout->addWidget(m_labelSeparatorEdit);
    stripLayout->addStretch();
    layout->addLayout(stripLayout);
    
    // Trim whitespace
    m_trimWhitespaceCheck = new QCheckBox(tr("Trim whitespace from tokens"));
    m_trimWhitespaceCheck->setChecked(true);
    connect(m_trimWhitespaceCheck, &QCheckBox::toggled,
            this, &ParserConfigWidget::configChanged);
    layout->addWidget(m_trimWhitespaceCheck);
    
    // Performance section separator
    auto *perfSeparator = new QFrame();
    perfSeparator->setFrameShape(QFrame::HLine);
    perfSeparator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(perfSeparator);
    
    auto *perfLabel = new QLabel(tr("<b>Performance</b>"));
    layout->addWidget(perfLabel);
    
    // Rate limiting
    auto *rateLayout = new QHBoxLayout();
    m_rateLimitCheck = new QCheckBox(tr("Rate limit display"));
    m_rateLimitCheck->setChecked(true);
    m_rateLimitCheck->setToolTip(tr("Limit display updates for better performance at high data rates"));
    connect(m_rateLimitCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_displayRateSpin->setEnabled(checked);
        emit configChanged();
    });
    rateLayout->addWidget(m_rateLimitCheck);
    
    m_displayRateSpin = new QSpinBox();
    m_displayRateSpin->setRange(1, 1000);
    m_displayRateSpin->setValue(60);
    m_displayRateSpin->setSuffix(tr(" Hz"));
    m_displayRateSpin->setToolTip(tr("Maximum display update rate (data logging is not affected)"));
    connect(m_displayRateSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int hz) {
                emit displayRateChanged(hz);
            });
    rateLayout->addWidget(m_displayRateSpin);
    rateLayout->addStretch();
    layout->addLayout(rateLayout);
    
    return group;
}

QGroupBox* ParserConfigWidget::createTestParseGroup()
{
    auto *group = new QGroupBox(tr("Test Parse"));
    auto *layout = new QVBoxLayout(group);
    layout->setSpacing(8);
    
    layout->addWidget(new QLabel(tr("Sample Line (last received):")));
    
    m_sampleLineEdit = new QPlainTextEdit();
    m_sampleLineEdit->setMaximumHeight(60);
    m_sampleLineEdit->setPlaceholderText(tr("Waiting for data..."));
    layout->addWidget(m_sampleLineEdit);
    
    m_testParseButton = new QPushButton(tr("Test Parse"));
    connect(m_testParseButton, &QPushButton::clicked,
            this, &ParserConfigWidget::onTestParseClicked);
    layout->addWidget(m_testParseButton);
    
    m_testResultLabel = new QLabel();
    m_testResultLabel->setWordWrap(true);
    layout->addWidget(m_testResultLabel);
    
    m_parsedValuesEdit = new QPlainTextEdit();
    m_parsedValuesEdit->setMaximumHeight(80);
    m_parsedValuesEdit->setReadOnly(true);
    m_parsedValuesEdit->setPlaceholderText(tr("Parsed values will appear here..."));
    layout->addWidget(m_parsedValuesEdit);
    
    return group;
}

void ParserConfigWidget::updateChannelCheckboxes()
{
    int checkedCount = 0;
    for (auto *cb : m_channelCheckboxes) {
        if (cb->isChecked()) {
            ++checkedCount;
        }
    }
    
    if (checkedCount == 0) {
        m_selectAllCheck->setCheckState(Qt::Unchecked);
    } else if (checkedCount == MAX_CHANNELS) {
        m_selectAllCheck->setCheckState(Qt::Checked);
    } else {
        m_selectAllCheck->setCheckState(Qt::PartiallyChecked);
    }
}

ParserConfig ParserConfigWidget::currentConfig() const
{
    ParserConfig config;
    
    // Delimiter
    if (m_delimiterModeCombo->currentData().toString() == "custom") {
        config.delimiter = m_customDelimiterEdit->text();
    } else {
        config.delimiter = m_delimiterModeCombo->currentData().toString();
    }
    
    // Data fields (selected channels)
    config.dataFields.clear();
    config.channelNames.clear();
    for (int i = 0; i < m_channelCheckboxes.size(); ++i) {
        if (m_channelCheckboxes[i]->isChecked()) {
            config.dataFields.append(i);
            config.channelNames.append(QString("Ch%1").arg(i));
        }
    }
    
    // X-axis source
    config.xAxisSource = static_cast<XAxisSource>(m_xAxisSourceCombo->currentData().toInt());
    config.xAxisFieldIndex = m_xAxisFieldSpin->value();
    
    // ID filter
    if (m_enableIdFilterCheck->isChecked()) {
        config.idFieldIndex = m_idFieldSpin->value();
        config.acceptSensorId = m_acceptIdEdit->text().trimmed();
    } else {
        config.idFieldIndex = -1;
        config.acceptSensorId.clear();
    }
    
    // Options
    config.stripLabels = m_stripLabelsCheck->isChecked();
    if (!m_labelSeparatorEdit->text().isEmpty()) {
        config.labelSeparator = m_labelSeparatorEdit->text().at(0);
    }
    config.trimWhitespace = m_trimWhitespaceCheck->isChecked();
    
    return config;
}

void ParserConfigWidget::setConfig(const ParserConfig &config)
{
    // Block signals during update
    blockSignals(true);
    
    // Delimiter
    bool foundDelimiter = false;
    for (int i = 0; i < m_delimiterModeCombo->count() - 1; ++i) {  // -1 to skip Custom
        if (m_delimiterModeCombo->itemData(i).toString() == config.delimiter) {
            m_delimiterModeCombo->setCurrentIndex(i);
            foundDelimiter = true;
            break;
        }
    }
    if (!foundDelimiter) {
        m_delimiterModeCombo->setCurrentIndex(m_delimiterModeCombo->count() - 1);  // Custom
        m_customDelimiterEdit->setText(config.delimiter);
    }
    
    // Data fields
    for (int i = 0; i < m_channelCheckboxes.size(); ++i) {
        m_channelCheckboxes[i]->setChecked(config.dataFields.contains(i));
    }
    updateChannelCheckboxes();
    
    // X-axis source
    for (int i = 0; i < m_xAxisSourceCombo->count(); ++i) {
        if (m_xAxisSourceCombo->itemData(i).toInt() == static_cast<int>(config.xAxisSource)) {
            m_xAxisSourceCombo->setCurrentIndex(i);
            break;
        }
    }
    m_xAxisFieldSpin->setValue(config.xAxisFieldIndex);
    m_xAxisFieldSpin->setEnabled(config.xAxisSource == XAxisSource::FieldIndex);
    
    // ID filter
    bool hasIdFilter = config.idFieldIndex >= 0;
    m_enableIdFilterCheck->setChecked(hasIdFilter);
    m_idFieldSpin->setValue(hasIdFilter ? config.idFieldIndex : 0);
    m_acceptIdEdit->setText(config.acceptSensorId);
    m_idFieldSpin->setEnabled(hasIdFilter);
    m_acceptIdEdit->setEnabled(hasIdFilter);
    
    // Options
    m_stripLabelsCheck->setChecked(config.stripLabels);
    m_labelSeparatorEdit->setText(QString(config.labelSeparator));
    m_labelSeparatorEdit->setEnabled(config.stripLabels);
    m_trimWhitespaceCheck->setChecked(config.trimWhitespace);
    
    blockSignals(false);
}

void ParserConfigWidget::setSampleLine(const QString &line)
{
    m_sampleLineEdit->setPlainText(line.trimmed());
}

void ParserConfigWidget::showTestResult(const ParseResult &result)
{
    if (result.success) {
        m_testResultLabel->setText(tr("<span style='color: #a6e3a1;'>✓ Parse successful!</span>"));
        m_testResultLabel->setStyleSheet("color: #a6e3a1;");
        
        QStringList values;
        for (int i = 0; i < result.values.size(); ++i) {
            values.append(QString("Ch%1 = %2").arg(i).arg(result.values[i], 0, 'f', 4));
        }
        m_parsedValuesEdit->setPlainText(values.join("\n"));
    } else {
        m_testResultLabel->setText(tr("<span style='color: #f38ba8;'>✗ Parse failed: %1</span>")
            .arg(result.errorMessage));
        m_testResultLabel->setStyleSheet("color: #f38ba8;");
        
        if (result.failedFieldIndex >= 0) {
            m_parsedValuesEdit->setPlainText(
                tr("Failed at field index %1\nTokens: %2")
                    .arg(result.failedFieldIndex)
                    .arg(result.fieldTexts.join(" | ")));
        } else {
            m_parsedValuesEdit->setPlainText(tr("Tokens: %1").arg(result.fieldTexts.join(" | ")));
        }
    }
}

void ParserConfigWidget::applyPreset(const QString &presetName)
{
    ParserConfig config;
    
    if (presetName == "csv") {
        config = ParserConfig::csvDefault();
        config.dataFields = {0, 1, 2};
    } else if (presetName == "space") {
        config.delimiter = " ";
        config.dataFields = {0, 1, 2};
    } else if (presetName == "tab") {
        config = ParserConfig::tsvDefault();
        config.dataFields = {0, 1, 2};
    } else if (presetName == "hall") {
        // Hall sensor: d<id> X Y Z overflow
        config.delimiter = " ";
        config.idFieldIndex = 0;
        config.acceptSensorId.clear();  // Empty = accept all
        config.dataFields = {1, 2, 3};
        config.channelNames = {"X", "Y", "Z"};
        config.stripLabels = false;
    } else if (presetName == "labeled") {
        config = ParserConfig::labeledDefault();
        config.dataFields = {};  // Auto-detect
    } else {
        return;  // Custom - no change
    }
    
    setConfig(config);
    emit configChanged();
}

void ParserConfigWidget::onDelimiterModeChanged(int index)
{
    bool isCustom = (m_delimiterModeCombo->itemData(index).toString() == "custom");
    m_customDelimiterEdit->setEnabled(isCustom);
    
    // Set preset to Custom when delimiter changes
    m_presetCombo->blockSignals(true);
    m_presetCombo->setCurrentIndex(0);  // Custom
    m_presetCombo->blockSignals(false);
    
    emit configChanged();
}

void ParserConfigWidget::onPresetChanged(int index)
{
    QString presetId = m_presetCombo->itemData(index).toString();
    if (presetId != "custom") {
        applyPreset(presetId);
    }
}

void ParserConfigWidget::onApplyClicked()
{
    emit configApplied(currentConfig());
}

void ParserConfigWidget::onTestParseClicked()
{
    QString sampleLine = m_sampleLineEdit->toPlainText();
    emit testParseRequested(sampleLine, currentConfig());
}

void ParserConfigWidget::onSelectAllToggled(bool checked)
{
    for (auto *cb : m_channelCheckboxes) {
        cb->blockSignals(true);
        cb->setChecked(checked);
        cb->blockSignals(false);
    }
    m_selectAllCheck->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
    emit configChanged();
}

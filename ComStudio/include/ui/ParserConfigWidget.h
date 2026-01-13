/**
 * @file ParserConfigWidget.h
 * @brief Widget for configuring the line parser
 *
 * Provides UI controls for configuring delimiter, field mapping,
 * ID filtering, label stripping, and test parse functionality.
 */

#ifndef PARSERCONFIGWIDGET_H
#define PARSERCONFIGWIDGET_H

#include <QWidget>
#include <QVector>
#include <QCheckBox>

#include "core/ParserConfig.h"
#include "core/LineParser.h"

// Forward declarations
class QComboBox;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QPlainTextEdit;
class QGroupBox;
class QGridLayout;

/**
 * @class ParserConfigWidget
 * @brief UI widget for configuring the line parser
 *
 * Allows users to configure delimiter, field indices, ID filter,
 * strip labels, and test the configuration against sample data.
 */
class ParserConfigWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit ParserConfigWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~ParserConfigWidget() override = default;
    
    /**
     * @brief Get the current configuration from UI
     * @return ParserConfig built from current UI values
     */
    ParserConfig currentConfig() const;
    
    /**
     * @brief Set configuration to display in UI
     * @param config Configuration to display
     */
    void setConfig(const ParserConfig &config);
    
    /**
     * @brief Update the sample line for test parsing
     * @param line The latest raw line received
     */
    void setSampleLine(const QString &line);
    
    /**
     * @brief Display a test parse result
     * @param result The parse result to display
     */
    void showTestResult(const ParseResult &result);

public slots:
    /**
     * @brief Apply a preset configuration
     * @param presetName Name of the preset
     */
    void applyPreset(const QString &presetName);

signals:
    /**
     * @brief Emitted when user clicks Apply
     * @param config The new configuration
     */
    void configApplied(const ParserConfig &config);
    
    /**
     * @brief Emitted when user clicks Test Parse
     * @param sampleLine Line to test
     * @param config Configuration to use
     */
    void testParseRequested(const QString &sampleLine, const ParserConfig &config);
    
    /**
     * @brief Emitted when any configuration value changes
     */
    void configChanged();
    
    /**
     * @brief Emitted when display rate changes
     * @param hz New display rate in Hz
     */
    void displayRateChanged(int hz);

private slots:
    /**
     * @brief Handle delimiter mode change
     * @param index Combo box index
     */
    void onDelimiterModeChanged(int index);
    
    /**
     * @brief Handle preset selection
     * @param index Combo box index
     */
    void onPresetChanged(int index);
    
    /**
     * @brief Handle Apply button click
     */
    void onApplyClicked();
    
    /**
     * @brief Handle Test Parse button click
     */
    void onTestParseClicked();
    
    /**
     * @brief Handle Select All checkbox
     * @param checked Check state
     */
    void onSelectAllToggled(bool checked);

private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Create the delimiter group
     * @return Group box widget
     */
    QGroupBox* createDelimiterGroup();
    
    /**
     * @brief Create the field mapping group
     * @return Group box widget
     */
    QGroupBox* createFieldMappingGroup();
    
    /**
     * @brief Create the ID filter group
     * @return Group box widget
     */
    QGroupBox* createIdFilterGroup();
    
    /**
     * @brief Create the options group
     * @return Group box widget
     */
    QGroupBox* createOptionsGroup();
    
    /**
     * @brief Create the test parse group
     * @return Group box widget
     */
    QGroupBox* createTestParseGroup();
    
    /**
     * @brief Update channel checkboxes based on field count
     */
    void updateChannelCheckboxes();

    // Delimiter controls
    QComboBox *m_delimiterModeCombo = nullptr;
    QLineEdit *m_customDelimiterEdit = nullptr;
    
    // Preset controls
    QComboBox *m_presetCombo = nullptr;
    
    // Field mapping controls
    static constexpr int MAX_CHANNELS = 16;
    QVector<QCheckBox*> m_channelCheckboxes;
    QCheckBox *m_selectAllCheck = nullptr;
    QGridLayout *m_channelGrid = nullptr;
    
    // X-axis controls
    QComboBox *m_xAxisSourceCombo = nullptr;
    QSpinBox *m_xAxisFieldSpin = nullptr;
    
    // ID filter controls
    QSpinBox *m_idFieldSpin = nullptr;
    QLineEdit *m_acceptIdEdit = nullptr;  // Changed to QLineEdit for alphanumeric IDs
    QCheckBox *m_enableIdFilterCheck = nullptr;
    
    // Options controls
    QCheckBox *m_stripLabelsCheck = nullptr;
    QLineEdit *m_labelSeparatorEdit = nullptr;
    QCheckBox *m_trimWhitespaceCheck = nullptr;
    
    // Performance controls
    QSpinBox *m_displayRateSpin = nullptr;
    QCheckBox *m_rateLimitCheck = nullptr;
    
    // Test parse controls
    QPlainTextEdit *m_sampleLineEdit = nullptr;
    QPushButton *m_testParseButton = nullptr;
    QLabel *m_testResultLabel = nullptr;
    QPlainTextEdit *m_parsedValuesEdit = nullptr;
    
    // Action buttons
    QPushButton *m_applyButton = nullptr;
};

#endif // PARSERCONFIGWIDGET_H

/**
 * @file AutoSendDialog.h
 * @brief Dialog for managing auto-send presets
 */

#ifndef AUTOSENDDIALOG_H
#define AUTOSENDDIALOG_H

#include <QDialog>
#include <QVector>
#include <QString>
#include <QTimer>

class QLineEdit;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QListWidget;
class QListWidgetItem;

/**
 * @struct SendPreset
 * @brief A single auto-send preset configuration
 */
struct SendPreset {
    QString label;              ///< Display label for the preset
    QString payload;            ///< Data to send
    int intervalMs = 0;         ///< Auto-repeat interval (0 = no repeat)
    bool enabled = true;        ///< Whether preset is active
};

/**
 * @class AutoSendDialog
 * @brief Dialog for managing auto-send presets
 * 
 * Allows users to create, edit, delete, and execute send presets.
 * Presets can have optional auto-repeat intervals.
 */
class AutoSendDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit AutoSendDialog(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~AutoSendDialog() override;
    
    /**
     * @brief Get all configured presets
     * @return Vector of presets
     */
    QVector<SendPreset> presets() const { return m_presets; }
    
    /**
     * @brief Set presets (for loading saved presets)
     * @param presets Presets to load
     */
    void setPresets(const QVector<SendPreset> &presets);

signals:
    /**
     * @brief Emitted when user wants to send data
     * @param data Data to send (as-is, caller handles encoding)
     */
    void sendRequested(const QString &data);
    
    /**
     * @brief Emitted when presets change
     * @param presets Updated presets list
     */
    void presetsChanged(const QVector<SendPreset> &presets);

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onSendClicked();
    void onItemSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onIntervalTimeout();
    void onStartStopClicked();
    void updatePresetFromUi();

private:
    void setupUi();
    void refreshList();
    void selectPreset(int index);
    
    QListWidget *m_listWidget = nullptr;
    QLineEdit *m_labelEdit = nullptr;
    QLineEdit *m_payloadEdit = nullptr;
    QSpinBox *m_intervalSpin = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_startStopButton = nullptr;
    
    QVector<SendPreset> m_presets;
    int m_selectedIndex = -1;
    
    QTimer *m_repeatTimer = nullptr;
    bool m_isRepeating = false;
};

#endif // AUTOSENDDIALOG_H

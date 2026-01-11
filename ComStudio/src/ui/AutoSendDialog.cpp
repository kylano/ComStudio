/**
 * @file AutoSendDialog.cpp
 * @brief Implementation of AutoSendDialog
 */

#include "ui/AutoSendDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QMessageBox>

AutoSendDialog::AutoSendDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Auto-Send Presets"));
    setMinimumSize(450, 350);
    
    m_repeatTimer = new QTimer(this);
    connect(m_repeatTimer, &QTimer::timeout, this, &AutoSendDialog::onIntervalTimeout);
    
    setupUi();
}

AutoSendDialog::~AutoSendDialog()
{
    m_repeatTimer->stop();
}

void AutoSendDialog::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);
    
    // Left: preset list
    auto *leftLayout = new QVBoxLayout();
    
    m_listWidget = new QListWidget();
    m_listWidget->setMinimumWidth(150);
    connect(m_listWidget, &QListWidget::itemSelectionChanged,
            this, &AutoSendDialog::onItemSelectionChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &AutoSendDialog::onItemDoubleClicked);
    leftLayout->addWidget(m_listWidget, 1);
    
    auto *listButtonLayout = new QHBoxLayout();
    m_addButton = new QPushButton(tr("Add"));
    connect(m_addButton, &QPushButton::clicked, this, &AutoSendDialog::onAddClicked);
    listButtonLayout->addWidget(m_addButton);
    
    m_removeButton = new QPushButton(tr("Remove"));
    m_removeButton->setEnabled(false);
    connect(m_removeButton, &QPushButton::clicked, this, &AutoSendDialog::onRemoveClicked);
    listButtonLayout->addWidget(m_removeButton);
    
    leftLayout->addLayout(listButtonLayout);
    mainLayout->addLayout(leftLayout);
    
    // Right: preset editor
    auto *rightLayout = new QVBoxLayout();
    
    auto *editGroup = new QGroupBox(tr("Preset Settings"));
    auto *formLayout = new QFormLayout(editGroup);
    
    m_labelEdit = new QLineEdit();
    m_labelEdit->setPlaceholderText(tr("e.g., Start Motor"));
    connect(m_labelEdit, &QLineEdit::textChanged, this, &AutoSendDialog::updatePresetFromUi);
    formLayout->addRow(tr("Label:"), m_labelEdit);
    
    m_payloadEdit = new QLineEdit();
    m_payloadEdit->setPlaceholderText(tr("e.g., CMD:START or 0x01 0x02"));
    connect(m_payloadEdit, &QLineEdit::textChanged, this, &AutoSendDialog::updatePresetFromUi);
    formLayout->addRow(tr("Payload:"), m_payloadEdit);
    
    auto *intervalLayout = new QHBoxLayout();
    m_intervalSpin = new QSpinBox();
    m_intervalSpin->setRange(0, 60000);
    m_intervalSpin->setSuffix(" ms");
    m_intervalSpin->setSpecialValueText(tr("No repeat"));
    m_intervalSpin->setToolTip(tr("0 = manual send only"));
    connect(m_intervalSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AutoSendDialog::updatePresetFromUi);
    intervalLayout->addWidget(m_intervalSpin);
    
    m_startStopButton = new QPushButton(tr("Start"));
    m_startStopButton->setEnabled(false);
    connect(m_startStopButton, &QPushButton::clicked, this, &AutoSendDialog::onStartStopClicked);
    intervalLayout->addWidget(m_startStopButton);
    
    formLayout->addRow(tr("Interval:"), intervalLayout);
    
    rightLayout->addWidget(editGroup);
    
    // Send button
    m_sendButton = new QPushButton(tr("Send Now"));
    m_sendButton->setEnabled(false);
    connect(m_sendButton, &QPushButton::clicked, this, &AutoSendDialog::onSendClicked);
    rightLayout->addWidget(m_sendButton);
    
    rightLayout->addStretch();
    
    // Close button
    auto *closeButton = new QPushButton(tr("Close"));
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    rightLayout->addWidget(closeButton);
    
    mainLayout->addLayout(rightLayout, 1);
}

void AutoSendDialog::setPresets(const QVector<SendPreset> &presets)
{
    m_presets = presets;
    refreshList();
}

void AutoSendDialog::refreshList()
{
    m_listWidget->clear();
    for (const auto &preset : m_presets) {
        m_listWidget->addItem(preset.label.isEmpty() ? tr("(unnamed)") : preset.label);
    }
}

void AutoSendDialog::selectPreset(int index)
{
    m_selectedIndex = index;
    
    bool valid = (index >= 0 && index < m_presets.size());
    m_removeButton->setEnabled(valid);
    m_sendButton->setEnabled(valid);
    m_startStopButton->setEnabled(valid && m_presets[index].intervalMs > 0);
    
    if (valid) {
        const auto &preset = m_presets[index];
        m_labelEdit->blockSignals(true);
        m_payloadEdit->blockSignals(true);
        m_intervalSpin->blockSignals(true);
        
        m_labelEdit->setText(preset.label);
        m_payloadEdit->setText(preset.payload);
        m_intervalSpin->setValue(preset.intervalMs);
        
        m_labelEdit->blockSignals(false);
        m_payloadEdit->blockSignals(false);
        m_intervalSpin->blockSignals(false);
    } else {
        m_labelEdit->clear();
        m_payloadEdit->clear();
        m_intervalSpin->setValue(0);
    }
}

void AutoSendDialog::onAddClicked()
{
    SendPreset preset;
    preset.label = tr("New Preset %1").arg(m_presets.size() + 1);
    m_presets.append(preset);
    
    refreshList();
    m_listWidget->setCurrentRow(m_presets.size() - 1);
    emit presetsChanged(m_presets);
}

void AutoSendDialog::onRemoveClicked()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_presets.size()) {
        return;
    }
    
    m_presets.removeAt(m_selectedIndex);
    refreshList();
    
    if (m_presets.isEmpty()) {
        selectPreset(-1);
    } else {
        m_listWidget->setCurrentRow(qMin(m_selectedIndex, m_presets.size() - 1));
    }
    
    emit presetsChanged(m_presets);
}

void AutoSendDialog::onSendClicked()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_presets.size()) {
        return;
    }
    
    emit sendRequested(m_presets[m_selectedIndex].payload);
}

void AutoSendDialog::onItemSelectionChanged()
{
    auto items = m_listWidget->selectedItems();
    if (items.isEmpty()) {
        selectPreset(-1);
    } else {
        selectPreset(m_listWidget->row(items.first()));
    }
}

void AutoSendDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    onSendClicked();
}

void AutoSendDialog::onIntervalTimeout()
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_presets.size()) {
        emit sendRequested(m_presets[m_selectedIndex].payload);
    }
}

void AutoSendDialog::onStartStopClicked()
{
    if (m_isRepeating) {
        m_repeatTimer->stop();
        m_isRepeating = false;
        m_startStopButton->setText(tr("Start"));
    } else {
        if (m_selectedIndex >= 0 && m_selectedIndex < m_presets.size()) {
            int interval = m_presets[m_selectedIndex].intervalMs;
            if (interval > 0) {
                m_repeatTimer->start(interval);
                m_isRepeating = true;
                m_startStopButton->setText(tr("Stop"));
                // Send immediately on start
                emit sendRequested(m_presets[m_selectedIndex].payload);
            }
        }
    }
}

void AutoSendDialog::updatePresetFromUi()
{
    if (m_selectedIndex < 0 || m_selectedIndex >= m_presets.size()) {
        return;
    }
    
    auto &preset = m_presets[m_selectedIndex];
    preset.label = m_labelEdit->text();
    preset.payload = m_payloadEdit->text();
    preset.intervalMs = m_intervalSpin->value();
    
    // Update list item
    if (auto *item = m_listWidget->item(m_selectedIndex)) {
        item->setText(preset.label.isEmpty() ? tr("(unnamed)") : preset.label);
    }
    
    // Update start button state
    m_startStopButton->setEnabled(preset.intervalMs > 0);
    
    emit presetsChanged(m_presets);
}

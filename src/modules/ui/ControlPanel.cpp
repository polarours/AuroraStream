/********************************************************************************
 * @file   : ControlPanel.cpp
 * @brief  :
 *
 *
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/ui/ControlPanel.h"

namespace aurorastream {
namespace modules {
namespace ui {

ControlPanel::ControlPanel(QWidget* parent) : QWidget(parent) {
    m_playButton = new QPushButton("Play", this);
    m_pauseButton = new QPushButton("Pause", this);
    m_stopButton = new QPushButton("Stop", this);
    
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(m_playButton);
    layout->addWidget(m_pauseButton);
    layout->addWidget(m_stopButton);
    
    setLayout(layout);
    
    setupUI();
    connectSignals();
}

ControlPanel::~ControlPanel() {
    // Cleanup resources
}

void ControlPanel::setupUI() {
    // Additional UI setup if needed
}

void ControlPanel::connectSignals() {
    connect(m_playButton, &QPushButton::clicked, this, [this]() { emit playClicked(); });
    connect(m_pauseButton, &QPushButton::clicked, this, [this]() { emit pauseClicked(); });
    connect(m_stopButton, &QPushButton::clicked, this, [this]() { emit stopClicked(); });
}

void ControlPanel::updateState(PlayerState state) {
    switch (state) {
        case Playing:
            m_playButton->setEnabled(false);
            m_pauseButton->setEnabled(true);
            m_stopButton->setEnabled(true);
            break;
        case Paused:
            m_playButton->setEnabled(true);
            m_pauseButton->setEnabled(false);
            m_stopButton->setEnabled(true);
            break;
        case Stopped:
            m_playButton->setEnabled(true);
            m_pauseButton->setEnabled(false);
            m_stopButton->setEnabled(false);
            break;
    }
}

} // namespace ui
} // namespace modules
} // namespace aurorastream
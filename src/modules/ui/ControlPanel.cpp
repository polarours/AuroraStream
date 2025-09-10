/********************************************************************************
 * @file   : ControlPanel.cpp
 * @brief  : AuroraStream 控制面板模块的实现。
 *
 * 本文件实现了 ControlPanel 类，它提供了一个简单的控制面板界面。
 *
 * @author : polarours
 * @date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/ui/ControlPanel.h"

namespace aurorastream {
namespace modules {
namespace ui {
/**
 * @brief 构造函数
 * @param parent 父窗口
 */
ControlPanel::ControlPanel(QWidget* parent)
    : QWidget(parent)
{
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

/**
 * @brief 析构函数
 * @note 释放资源
 */
ControlPanel::~ControlPanel() {
    // TODO: 释放资源
}

/**
 * @brief 设置界面
 */
void ControlPanel::setupUI() {
    // TODO: 初始化界面
}

/**
 * @brief 连接信号槽
 */
void ControlPanel::connectSignals()
{
    connect(m_playButton, &QPushButton::clicked, this, [this]() { emit playClicked(); });
    connect(m_pauseButton, &QPushButton::clicked, this, [this]() { emit pauseClicked(); });
    connect(m_stopButton, &QPushButton::clicked, this, [this]() { emit stopClicked(); });
}

/**
 * @brief 更新状态
 * @param state 状态
 */
void ControlPanel::updateState(PlayerState state)
{
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
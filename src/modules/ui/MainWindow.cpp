/********************************************************************************
 * @file   : MainWindow.cpp
 * @brief  : 实现主窗口
 *
 * 本文件实现了主窗口的界面和逻辑
 * 包括视频渲染、控制面板等
 *
 * @Author : polarours
 * @Date   : 2025/08/25
 ********************************************************************************/

include "aurorastream/modules/ui/MainWindow.h"

namespace aurorastream {
namespace modules {
namespace ui {


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_videoWidget = new VideoWidget(this);
    m_controlPanel = new ControlPanel(this);
    
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->addWidget(m_videoWidget);
    layout->addWidget(m_controlPanel);
    
    setCentralWidget(centralWidget);
    
    setupUI();
    connectSignals();
}

MainWindow::~MainWindow() {
    // Cleanup resources
}

void MainWindow::setupUI() {
    setWindowTitle("AuroraStream");
    resize(800, 600);
    
    // Add menu bar and other UI components if needed
}

void MainWindow::connectSignals() {
    connect(m_controlPanel, &ControlPanel::playClicked, m_videoWidget, &VideoWidget::play);
    connect(m_controlPanel, &ControlPanel::pauseClicked, m_videoWidget, &VideoWidget::pause);
    connect(m_controlPanel, &ControlPanel::stopClicked, m_videoWidget, &VideoWidget::stop);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Handle close event
    QMainWindow::closeEvent(event);
}

} // namespace ui
} // namespace modules
} // namespace aurorastream
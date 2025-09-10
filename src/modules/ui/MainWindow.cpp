/********************************************************************************
 * @file   : MainWindow.cpp
 * @brief  : 实现了 aurorastream::modules::ui::MainWindow 主窗口类。
 *
 * 该类实现了 AuroraStream 的主窗口，包括视频播放器、控制面板等。
 *
 * @author : polarours
 * @date   : 2025/08/25
 ********************************************************************************/

include "aurorastream/modules/ui/MainWindow.h"

namespace aurorastream {
namespace modules {
namespace ui {
/**
 * @brief MainWindow 类的构造函数
 * @param parent 父窗口
 */
MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	// TODO: 初始化成员变量
{
    setupUI();

	qDebug() << "MainWindow initialized";
}

/**
 * @brief 析构函数
 * @note 释放资源
 */
MainWindow::~MainWindow()
{
	// TODO: 释放资源
}

/**
 * @brief 设置视频源
 * @param source 视频源路径
 */
void MainWindow::setupUI()
{
    setWindowTitle("AuroraStream");
    resize(800, 600);
}

/**
 * @brief 连接信号槽
 */
void MainWindow::connectSignals()
{
    connect(m_controlPanel, &ControlPanel::playClicked, m_videoWidget, &VideoWidget::play);
    connect(m_controlPanel, &ControlPanel::pauseClicked, m_videoWidget, &VideoWidget::pause);
    connect(m_controlPanel, &ControlPanel::stopClicked, m_videoWidget, &VideoWidget::stop);
}

/**
 * @brief 关闭事件
 * @param event 关闭事件
 */
void MainWindow::closeEvent(QCloseEvent* event)
{
    QMainWindow::closeEvent(event);
}

/**
 * @brief 显示事件
 */
void MainWindow::show()
{
    QMainWindow::show();
}

// TODO: 实现其他成员函数

} // namespace ui
} // namespace modules
} // namespace aurorastream
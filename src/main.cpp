/********************************************************************************
* @file   : main.cpp
 * @brief  : AuroraStream 主程序入口
 *
 * 本文件包含 AuroraStream 的主程序入口函数 main()
 *
 * @author : polarours
 * @date   : 2025/08/26
 ********************************************************************************/

#include <QApplication>
#include <QWidget>

#include "AuroraStream/core/MediaPlayer.h"
#include "AuroraStream/modules/ui/MainWindow.h"

/**
 * @brief 主程序入口
 * @param argc 命令行参数个数
 * @param argv 命令行参数
 * @return 应用程序执行结果
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 初始化媒体播放器
    aurorastream::core::MediaPlayer mediaPlayer;

    // 创建主窗口并设置媒体播放器
    aurorastream::modules::ui::MainWindow mainWindow;
    mainWindow.setMediaPlayer(&mediaPlayer);
    mainWindow.show();

    return app.exec();
}
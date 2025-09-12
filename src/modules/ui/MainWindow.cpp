/********************************************************************************
 * @file   : MainWindow.cpp
 * @brief  : 实现了 aurorastream::modules::ui::MainWindow 主窗口类。
 *
 * 该类实现了 AuroraStream 的主窗口，包含视频显示和控制面板功能。
 *
 * @author : polarours
 * @date   : 2025/08/25
 ********************************************************************************/

#include "aurorastream/modules/ui/MainWindow.h"
#include "aurorastream/core/MediaPlayer.h"
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>

namespace aurorastream {
namespace modules {
namespace ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_mediaPlayer(nullptr)
    , m_openButton(nullptr)
    , m_playButton(nullptr)
    , m_pauseButton(nullptr)
    , m_stopButton(nullptr)
    , m_seekSlider(nullptr)
    , m_volumeSlider(nullptr)
    , m_timeLabel(nullptr)
    , m_durationLabel(nullptr)
    , m_duration(0)
    , m_videoContainer(nullptr)
{
    setupUI();
    connectSignals();
}

MainWindow::~MainWindow()
{
    // 清理资源
    delete m_openButton;
    delete m_playButton;
    delete m_pauseButton;
    delete m_stopButton;
    delete m_seekSlider;
    delete m_volumeSlider;
    delete m_timeLabel;
    delete m_durationLabel;
    delete m_videoContainer;
}

void MainWindow::setMediaPlayer(aurorastream::core::MediaPlayer* player)
{
    m_mediaPlayer = player;
    if (m_mediaPlayer) {
        connect(m_mediaPlayer, &core::MediaPlayer::stateChanged, this, &MainWindow::onMediaStateChanged);
        connect(m_mediaPlayer, &core::MediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
        connect(m_mediaPlayer, &core::MediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);
        connect(m_mediaPlayer, &core::MediaPlayer::error, this, &MainWindow::onError);
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("AuroraStream");
    resize(800, 600);
    setAcceptDrops(true);

    // 创建主窗口部件
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 创建视频显示区域
    m_videoContainer = new QWidget(centralWidget);
    m_videoContainer->setMinimumSize(640, 480);
    m_videoContainer->setStyleSheet("background-color: black;");
    mainLayout->addWidget(m_videoContainer);

    // 创建控制面板
    QWidget* controlPanel = new QWidget(centralWidget);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);

    // 创建控制按钮
    m_openButton = new QPushButton("打开", controlPanel);
    m_playButton = new QPushButton("播放", controlPanel);
    m_pauseButton = new QPushButton("暂停", controlPanel);
    m_stopButton = new QPushButton("停止", controlPanel);
    m_connectButton = new QPushButton("连接RTMP", controlPanel);
    m_rtmpUrlEdit = new QLineEdit(controlPanel);
    m_rtmpUrlEdit->setPlaceholderText("输入RTMP地址");
    m_rtmpUrlEdit->setMinimumWidth(200);

    // 创建进度条和音量控制
    m_seekSlider = new QSlider(Qt::Horizontal, controlPanel);
    m_seekSlider->setRange(0, 100);
    m_volumeSlider = new QSlider(Qt::Horizontal, controlPanel);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);

    // 创建时间标签
    m_timeLabel = new QLabel("00:00", controlPanel);
    m_durationLabel = new QLabel("00:00", controlPanel);

    // 添加到控制面板布局
    controlLayout->addWidget(m_openButton);
    controlLayout->addWidget(m_playButton);
    controlLayout->addWidget(m_pauseButton);
    controlLayout->addWidget(m_stopButton);
    controlLayout->addWidget(m_rtmpUrlEdit);
    controlLayout->addWidget(m_connectButton);
    controlLayout->addWidget(m_timeLabel);
    controlLayout->addWidget(m_seekSlider);
    controlLayout->addWidget(m_durationLabel);
    controlLayout->addWidget(new QLabel("音量:", controlPanel));
    controlLayout->addWidget(m_volumeSlider);

    mainLayout->addWidget(controlPanel);
    setCentralWidget(centralWidget);

    // 初始化按钮状态
    updateButtons();
}

void MainWindow::connectSignals()
{
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::onPlayClicked);
    connect(m_pauseButton, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_seekSlider, &QSlider::sliderMoved, this, &MainWindow::onSeekSliderMoved);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
}

void MainWindow::onOpenFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "打开媒体文件", "", "媒体文件 (*.mp4 *.avi *.mkv *.mp3 *.wav)");
    if (!filePath.isEmpty()) {
        onMediaOpened(filePath);
    }
}

void MainWindow::onPlayClicked()
{
    if (m_mediaPlayer) {
        m_mediaPlayer->play();
    }
}

void MainWindow::onPauseClicked()
{
    if (m_mediaPlayer) {
        m_mediaPlayer->pause();
    }
}

void MainWindow::onStopClicked()
{
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
}

void MainWindow::onSeekSliderMoved(int value)
{
    if (m_mediaPlayer && m_duration > 0) {
        qint64 position = static_cast<qint64>(value) * m_duration / 100;
        m_mediaPlayer->seek(position);
    }
}

void MainWindow::onVolumeChanged(int value)
{
    if (m_mediaPlayer) {
        m_mediaPlayer->setVolume(value / 100.0f);
    }
}

void MainWindow::onConnectClicked()
{
    QString url = m_rtmpUrlEdit->text().trimmed();
    if (!url.isEmpty()) {
        onRtmpOpened(url);
    }
}

void MainWindow::onRtmpOpened(const QString& url)
{
    m_currentRtmpUrl = url;
    updateWindowTitle();
    if (m_mediaPlayer) {
        m_mediaPlayer->setSource(url);
    }
}

void MainWindow::onMediaOpened(const QString& filePath)
{
    m_currentFile = filePath;
    updateWindowTitle();
    if (m_mediaPlayer) {
        m_mediaPlayer->setSource(filePath);
    }
}

void MainWindow::onError(const QString& message)
{
    qWarning() << "Media error:" << message;
}

void MainWindow::onMediaStateChanged(aurorastream::MediaState state)
{
    updateButtons();
}

void MainWindow::onDurationChanged(qint64 duration)
{
    m_duration = duration;
    m_seekSlider->setRange(0, 100);
    m_durationLabel->setText(formatTime(duration));
}

void MainWindow::onPositionChanged(qint64 position)
{
    if (m_duration > 0) {
        int sliderValue = static_cast<int>(position * 100 / m_duration);
        m_seekSlider->setValue(sliderValue);
        m_timeLabel->setText(formatTime(position));
    }
}

void MainWindow::paintEvent(QPaintEvent* event)
{
    QMainWindow::paintEvent(event);
    // 视频渲染逻辑可以在这里实现
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    // 处理窗口大小变化
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QUrl url = mimeData->urls().first();
        if (url.isLocalFile()) {
            onMediaOpened(url.toLocalFile());
        }
    }
}

void MainWindow::updateButtons()
{
    if (!m_mediaPlayer) {
        m_playButton->setEnabled(false);
        m_pauseButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        return;
    }

    auto state = m_mediaPlayer->getState();
    m_playButton->setEnabled(state != MediaState::PLAYING);
    m_pauseButton->setEnabled(state == MediaState::PLAYING);
    m_stopButton->setEnabled(state != MediaState::STOPPED);
}

void MainWindow::updateWindowTitle()
{
    QString title = "AuroraStream";
    if (!m_currentFile.isEmpty()) {
        title += " - " + QFileInfo(m_currentFile).fileName();
    } else if (!m_currentRtmpUrl.isEmpty()) {
        title += " - " + m_currentRtmpUrl;
    }
    setWindowTitle(title);
}

QString MainWindow::formatTime(qint64 milliseconds)
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes, 2, 10, QLatin1Char('0')).arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace ui
} // namespace modules
} // namespace aurorastream
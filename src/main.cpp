#include <QApplication>
#include "aurorastream/modules/ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    aurorastream::modules::ui::MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}
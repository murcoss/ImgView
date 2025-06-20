#include <QMainWindow>
#include <QSettings>
#include "ImgView.h"
#include "IconEngine.h"

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QStringList filenames) {
		ImgView* view = new ImgView(this);
		setCentralWidget(view);
		connect(view, &ImgView::message, [this](QString msg) {
            setWindowTitle(QStringLiteral(u"'%1' - ImgView").arg(msg));
			});
		if(!filenames.isEmpty()){
			view->loadImage(filenames);
		}

		QIcon* ie = new QIcon(new IconEngine);
		setWindowIcon(*ie);

		QSettings const settings("ImgView", "ImgView");
		int const w = settings.value("WindowWidth", 800).toInt();
		int const h = settings.value("WindowHeight", 600).toInt();
		if (w > 100 && h > 100) {
			resize(w, h);
		}
	}

    void resizeEvent(QResizeEvent* event){
        QSettings settings("ImgView", "ImgView");
        QSize size = this->size();
        settings.setValue("WindowWidth", size.width());
        settings.setValue("WindowHeight", size.height());
        QMainWindow::resizeEvent(event);
    }
};
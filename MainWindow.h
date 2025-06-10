#include <QMainWindow>
#include <QStatusBar>
#include "ImgView.h"
#include "IconEngine.h"

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QStringList filenames) {
		ImgView* view = new ImgView(this);
		setCentralWidget(view);
		connect(view, &ImgView::message, [this](QString msg) {
			//statusBar()->showMessage(msg);
            setWindowTitle(QStringLiteral(u"ImgView - '%1'").arg(msg));
			});
		if(!filenames.isEmpty()){
			view->loadImage(filenames);
		}
		resize(800, 600);
		QIcon* ie = new QIcon(new IconEngine);
		setWindowIcon(*ie);
	}
};
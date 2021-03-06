#ifndef FILESTATUSWIDGET_H
#define FILESTATUSWIDGET_H

#include "main/statuswidget.h"
#include "basefile.h"

class FileStatusWidget : public StatusWidget
{
	Q_OBJECT

public:
	FileStatusWidget(BaseFile* file, QWidget* parent);
	~FileStatusWidget();

	void showEvent(QShowEvent*);

private slots:
	void openStatusChanged();

private:
	BaseFile* mFile;
};

#endif // FILESTATUSWIDGET_H

#include "dialogrethreader.h"
#include <QCoreApplication>

DialogRethreader* DialogRethreader::sInstance;
int DialogRethreader::sRunDialogEventId;

DialogRethreader::DialogRethreader() : QObject(0)
{
	sInstance = this;
	sRunDialogEventId = QEvent::registerEventType();
}

bool DialogRethreader::event(QEvent* event)
{
	if (event->type() == sRunDialogEventId)
	{
		event->accept();

		DialogEvent* e = (DialogEvent*)event;
		DialogRethreadRequest* rq = e->request;

		ThreadCrossingDialog* dialog = rq->factoryMethod();
		dialog->setOptions(rq->options);
		dialog->exec();
		rq->result = dialog->getResult();
		delete dialog;

		//	Unlock the provided mutex to tell the calling thread that we're done here.
		rq->lock->unlock();
		return true;
	}
	else
		return QObject::event(event);
}


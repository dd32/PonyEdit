#ifndef TESTPONY

#include <QtGui/QApplication>
#include <QString>
#include <QDebug>
#include <QMetaType>
#include <QFont>
#include <QNetworkProxyFactory>

#include "website/updatemanager.h"
#include "website/sitemanager.h"
#include "ponyedit.h"

int main(int argc, char *argv[])
{
	int result = 1;

	UpdateManager* updateManager = NULL;

	try
	{
		QCoreApplication::setOrganizationName("Pentalon");
		QCoreApplication::setApplicationName("PonyEdit");
		QCoreApplication::setApplicationVersion(PRETTY_VERSION);

		PonyEdit a(argc, argv);

		if(a.isRunning())
		{
			if(argc > 1)
			{
				for(int ii = 1; ii < argc; ii++)
				{
					QString name(argv[ii]);
					if(name.trimmed().isNull())
						continue;

					a.sendMessage(name);
				}
			}

			return 0;
		}

		if(argc > 1)
		{
			for(int ii = 1; ii < argc; ii++)
			{
				QString name(argv[ii]);
				if(name.trimmed().isNull())
					continue;

				gMainWindow->openSingleFile(Location(name));
			}
		}

		QNetworkProxyFactory::setUseSystemConfiguration(true);

		updateManager = new UpdateManager();

		gSiteManager->checkForUpdates();

		result = a.exec();
	}
	catch (QString err)
	{
		qDebug() << "FATAL ERROR: " << err;
	}

	delete updateManager;
	return result;
}

#endif

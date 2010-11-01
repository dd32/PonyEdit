#include "location.h"
#include "sshremotecontroller.h"
#include "sshrequest.h"
#include "sshhost.h"
#include <QFileIconProvider>
#include <QMetaMethod>
#include <QObject>
#include <QDebug>
#include <QDir>


QRegExp gSshServerRegExp("^(?:([^@:]+)@)?([^:]+\\.[^:]+):([^:]+)?");


///////////////////////////
//  Icon Provider stuff  //
///////////////////////////

QFileIconProvider* sIconProvider = NULL;

void LocationShared::initIconProvider()
{
	if (!sIconProvider)
		sIconProvider = new QFileIconProvider();
}

void LocationShared::cleanupIconProvider()
{
	if (sIconProvider)
	{
		delete(sIconProvider);
		sIconProvider = NULL;
	}
}


//////////////////////////////////
//  Constructors & Destructors  //
//////////////////////////////////

Location::Location()
{
	mData = NULL;
}

Location::Location(const Location& other)
{
	mData = other.mData;
	mData->mReferences++;
}

Location& Location::operator=(const Location& other)
{
	if (mData != NULL)
	{
		mData->mReferences--;
		if (mData->mReferences <= 0)
			delete(mData);
	}

	mData = other.mData;
	mData->mReferences++;

	return *this;
}

Location::Location(LocationShared* data)
{
	mData = data;
	mData->mReferences++;
}

Location::Location(const QString& path)
{
	mData = new LocationShared();
	mData->setPath(path);
}

Location::Location(const Location& parent, const QString& path, Type type, int size, QDateTime lastModified)
{
	mData = new LocationShared();
	mData->setPath(path);
	mData->mType = type;
	mData->mSize = size;
	mData->mLastModified = lastModified;
	mData->mSelfLoaded = true;
	mData->mParent = parent;
}

LocationShared::LocationShared()
{
	initIconProvider();

	mReferences = 1;
	mType = Location::Unknown;
	mSize = -1;
	mSelfLoaded = false;
	mListLoaded = false;
	mRemoteHost = NULL;
}

Location::~Location()
{
	if (mData != NULL)
	{
		mData->mReferences--;
		if (mData->mReferences <= 0)
			delete (mData);
	}
}


/////////////////////////////
//  Method Implementation  //
/////////////////////////////

const QString& Location::getPath() const { return mData->mPath; }
const QString& Location::getLabel() const { return mData->mLabel; }
bool Location::isNull() const { return (mData == NULL || getPath() == ""); }
bool Location::isHidden() const { return (mData->mLabel.startsWith('.')); }
int Location::getSize() const { return mData->mSize; }
const QDateTime& Location::getLastModified() const { return mData->mLastModified; }
bool Location::isDirectory() const { return mData->mType == Directory; }

const Location& Location::getParent()
{
	if (mData->mParent.isNull())
		mData->mParent = Location(getParentPath());

	return mData->mParent;
}

QString Location::getParentPath() const
{
	QString parentPath = getPath();
	int lastSlash = parentPath.lastIndexOf('/');
	if (lastSlash < 0)
		return "";
	else
	{
		parentPath.truncate(lastSlash);
		return parentPath;
	}
}

QString Location::getDisplayPath() const
{
	QString p = getPath();

#ifdef Q_OS_WIN
	if (mData->mProtocol == Local)
		p.replace('/', '\\');
#endif

	return p;
}

QIcon Location::getIcon() const
{
	switch (mData->mProtocol)
	{
	case Location::Local:
		return sIconProvider->icon(QFileInfo(mData->mPath));

	case Location::Ssh:
		return sIconProvider->icon(isDirectory() ? QFileIconProvider::Folder : QFileIconProvider::File);

	default:
		return QIcon();
	}
}

Location::Type Location::getType() const
{
	if (!mData->mSelfLoaded && mData->mProtocol == Local)
		mData->localLoadSelf();

	return mData->mType;
}

void LocationShared::setPath(const QString &path)
{
	mPath = path.trimmed();
	mPath.replace('\\', '/');

	//	Clean off any trailing slashes
	while (mPath.endsWith('/'))
		mPath.truncate(mPath.length() - 1);

	//	Work out what to label this path...
	int lastSlashIndex = mPath.lastIndexOf('/');
	mLabel = mPath.mid(lastSlashIndex + 1);

	//	Work out what kind of path this is. Default if no pattern matches, is local.
	if (gSshServerRegExp.indexIn(mPath) > -1)
	{
		mProtocol = Location::Ssh;
		QStringList parts = gSshServerRegExp.capturedTexts();
		mRemoteUserName = parts[1];
		mRemoteHostName = parts[2];
		mRemotePath = parts[3];
	}
	else
		mProtocol = Location::Local;
}

void LocationShared::localLoadSelf()
{
	QFileInfo fileInfo = QFileInfo(mPath);
	mType = fileInfo.isDir() ? Location::Directory : Location::File;
	mSelfLoaded = true;
}

void LocationShared::localLoadListing()
{
	if (!mSelfLoaded)
		localLoadSelf();

	mChildren.clear();

	QDir directory(mPath);
	QStringList entries = directory.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
	foreach (QString entry, entries)
	{
		QFileInfo fileInfo(mPath + "/" + entry);
		mChildren.append(Location(Location(this), fileInfo.absoluteFilePath(), fileInfo.isDir() ? Location::Directory : Location::File, fileInfo.size(), fileInfo.lastModified()));
	}

	mListLoaded = true;
	emitListLoadedSignal();
}

void LocationShared::emitListLoadedSignal()
{
	emit loadListSuccessful(mChildren, mPath);
}

void LocationShared::emitListLoadError(const QString& error)
{
	emit loadListFailed(error, mPath);
}

void LocationShared::emitOpenFileFailed(const QString& error)
{
	emit openFileFailed(error);
}

void Location::asyncGetChildren(QObject* callbackTarget, const char* succeedSlot, const char* failSlot)
{
	QObject::connect(mData, SIGNAL(loadListSuccessful(QList<Location>,QString)), callbackTarget, succeedSlot);
	QObject::connect(mData, SIGNAL(loadListFailed(QString,QString)), callbackTarget, failSlot);

	if (mData->mListLoaded)
		mData->emitListLoadedSignal();
	else if (!mData->mLoading)
	{
		mData->mLoading = true;
		switch (mData->mProtocol)
		{
		case Local:
			mData->localLoadListing();
			break;

		case Ssh:
			mData->sshLoadListing();
			break;

		default:
			throw(QString("Invalid file protocol!"));
		}
	}
}

void Location::asyncOpenFile(QObject* callbackTarget, const char* succeedSlot, const char* failSlot)
{
	QObject::connect(mData, SIGNAL(openFileSuccessful(File*)), callbackTarget, succeedSlot);
	QObject::connect(mData, SIGNAL(openFileFailed(QString)), callbackTarget, failSlot);

	switch (mData->mProtocol)
	{
	case Local:
		mData->emitOpenFileFailed("Opening local files is not yet supported!");
		break;

	case Ssh:
		mData->sshOpenFile();
		break;

	default:
		mData->emitOpenFileFailed("Unknown file protocol :(");
	}
}

QString Location::getRemotePath() const
{
	return mData->mRemotePath;
}

void LocationShared::sshOpenFile()
{
	if (!ensureConnected())
	{
		emit openFileFailed("Failed to connect to remote host!");
		return;
	}

	SshRemoteController* controller = mRemoteHost->getController();
	controller->sendRequest(new SshRequest_open(Location(this)));
}

void LocationShared::sshLoadListing()
{
	if (!ensureConnected())
	{
		emit loadListFailed("Failed to connect to remote host!", mPath);
		return;
	}

	SshRemoteController* controller = mRemoteHost->getController();
	controller->sendRequest(new SshRequest_ls(Location(this)));
}

void Location::sshChildLoadResponse(const QList<Location>& children)
{
	mData->mChildren = children;
	mData->mListLoaded = true;
	mData->emitListLoadedSignal();
}

void Location::childLoadError(const QString& error)
{
	mData->emitListLoadError(error);
}

void Location::fileOpenError(const QString& error)
{
	mData->emitOpenFileFailed(error);
}

bool LocationShared::ensureConnected()
{
	//	Local locations are always connected
	if (mProtocol == Location::Local) return true;

	if (mProtocol == Location::Ssh)
	{
		if (mRemoteHost == NULL)
			mRemoteHost = SshHost::getHost(mRemoteHostName, mRemoteUserName);

		if (mRemoteHost && (mRemoteHost->isConnected() || mRemoteHost->connect()))
		{
			mPath.replace("~", mRemoteHost->getHomeDirectory());
			mRemotePath.replace("~", mRemoteHost->getHomeDirectory());
			mPath = mPath.trimmed();
			mRemotePath = mRemotePath.trimmed();

			if (mRemotePath.length() == 0)
				mRemotePath = "/";

			return true;
		}
	}

	mPath = "";
	return false;
}











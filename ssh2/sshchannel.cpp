#include "sshchannel.h"
#include "sshhost.h"
#include <QDebug>
#include <libssh2.h>

SshChannel::SshChannel(SshHost* host) : mStatus(Sessionless)
{
	mHost = host;
}

SshChannel::~SshChannel()
{
}

void SshChannel::setSession(SshSession* session)
{
	if (mSession != session)
	{
		mSession = session;
		setStatus(mSession == NULL ? Sessionless : WaitingForSession);
	}
}

bool SshChannel::updateChannel()
{
	//	If waiting for a session, move the status along to opening, then call update.
	if (mStatus == WaitingForSession)
		setStatus(Opening);

	return update();
}

void SshChannel::criticalError(const QString& error)
{
	SSHLOG_ERROR(mHost) << "SshChannel has hit a critical error: " << error;

	//	Note: Error will wind up being logged by SshSession::threadMain's catch block.
	mStatus = Error;
	mErrorDetails = error;
}

void SshChannel::setStatus(Status status)
{
	if (mStatus != status)
	{
		mStatus = status;
		mHost->invalidateOverallStatus();
	}
}

int SshChannel::getConnectionScore()
{
	if (mStatus == Sessionless)
		return mSession->getStatus();
	else
		return mStatus;
}

QString SshChannel::getConnectionDescription()
{
	switch (mStatus)
	{
	case Opening:
		return tr("Opening Channel");

	case Open:
		return tr("Connected");

	default:;
	}

	return mSession->getConnectionDescription();
}




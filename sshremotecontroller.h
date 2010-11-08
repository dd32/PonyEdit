#ifndef SSHREMOTECONTROLLER_H
#define SSHREMOTECONTROLLER_H

#include <QString>
#include <QByteArray>

class SshControllerThread;
class SshRequest;
class SshHost;

#define	KEEPALIVE_TIMEOUT 60000	/* 60 seconds */

class SshRemoteController
{
public:
	enum Status { NotConnected, Connecting, WaitingForPassword, Negotiating, UploadingSlave, StartingSlave, Connected, Error };
	static const char* sStatusStrings[];

	enum ScriptType { AutoDetect, Python, Perl, NumScriptTypes };
	static const char* sScriptTypeLabels[];

	SshRemoteController(SshHost* host);
	~SshRemoteController();

	void abortConnection();
	void sendRequest(SshRequest* request);
	const QString& getHomeDirectory() const;

	int getLastStatusChange() const;
	Status getStatus() const;
	static const char* getStatusString(Status s) { return sStatusStrings[s]; }
	const QString& getError() const;

private:
	SshControllerThread* mThread;
};

#endif // SSHREMOTECONTROLLER_H

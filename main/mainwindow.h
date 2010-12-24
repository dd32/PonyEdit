#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QStatusBar>
#include <QTextEdit>
#include <QStackedWidget>
#include <QStatusBar>
#include <QLabel>

class Editor;
class FileList;
class BaseFile;
class SearchBar;
class UnsavedChangesDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
	void newFile();
	void openFile();
	void saveFile();
	void saveFileAs();
	void closeFile();
	void fileSelected(BaseFile* file);
	void showSearchBar();
	void find(const QString& text, bool backwards);

	void options();
	void about();

	void showErrorMessage(QString error);
	void showStatusMessage(QString message);

	void fileClosed(BaseFile* file);
	void syntaxMenuOptionClicked();

	void currentEditorChanged();
	void updateSyntaxSelection();

protected:
	void closeEvent(QCloseEvent* event);

private:
	void createToolbar();
	void createFileMenu();
	void createViewMenu();
	void createSearchMenu();
	void createToolsMenu();
	void createHelpMenu();
	void createSearchBar();
	void restoreState();

	Editor* getCurrentEditor();

	FileList* mFileList;
	QStackedWidget* mEditorStack;
	QStatusBar* mStatusBar;
	QLabel* mStatusLine;
	QList<Editor*> mEditors;

	QMap<QString, QAction*> mSyntaxMenuEntries;
	QAction* mCurrentSyntaxMenuItem;
	QMenu* mSyntaxMenu;

	QDockWidget* mSearchBarWrapper;
	SearchBar* mSearchBar;
	UnsavedChangesDialog* mUnsavedChangesDialog;
};

#endif // MAINWINDOW_H

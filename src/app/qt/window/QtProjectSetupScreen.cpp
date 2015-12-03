#include "qt/window/QtProjectSetupScreen.h"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSysInfo>

#include "utility/messaging/type/MessageLoadProject.h"

#include "settings/ProjectSettings.h"

QtTextLine::QtTextLine(QWidget *parent)
	: QWidget(parent)
{
	QBoxLayout* layout = new QHBoxLayout();
	layout->setSpacing(0);
	layout->setContentsMargins(1, 1, 1, 1);
	layout->setAlignment(Qt::AlignTop);

	setLayout(layout);

	m_data = new QtLineEdit(this);
	m_data->setAttribute(Qt::WA_MacShowFocusRect, 0);
	m_data->setObjectName("locationField");

	m_button = new QPushButton("...");
	m_button->setObjectName("moreButton");

	layout->addWidget(m_data);
	layout->addWidget(m_button);

	connect(m_button, SIGNAL(clicked()), this, SLOT(handleButtonPress()));
}

QString QtTextLine::getText()
{
	return m_data->text();
}

void QtTextLine::setText(QString text)
{
	m_data->setText(text);
}

void QtTextLine::handleButtonPress()
{
	QString file = QFileDialog::getExistingDirectory(this, tr("Select Directory"), "");
	if (!file.isEmpty())
	{
		m_data->setText(file);
	}
}


QtProjectSetupScreen::QtProjectSetupScreen(QWidget *parent)
	: QtSettingsWindow(parent)
	, m_frameworkPaths(nullptr)
{
	raise();
}

QSize QtProjectSetupScreen::sizeHint() const
{
	return QSize(600,600);
}

void QtProjectSetupScreen::clear()
{
	m_projectName->setText("");
	m_projectFileLocation->setText("");

	m_sourcePaths->clear();
	m_includePaths->clear();

	if (m_frameworkPaths)
	{
		m_frameworkPaths->clear();
	}
}

void QtProjectSetupScreen::setup()
{
	setupForm();

	QPushButton* preferencesButton = new QPushButton("Preferences");
	preferencesButton->setObjectName("windowButton");
	connect(preferencesButton, SIGNAL(clicked()), this, SLOT(handlePreferencesButtonPress()));

	m_buttonsLayout->insertWidget(2, preferencesButton);
	m_buttonsLayout->insertStretch(3);
}

void QtProjectSetupScreen::loadEmpty()
{
	updateTitle("NEW PROJECT");
	updateDoneButton("Create");
}

void QtProjectSetupScreen::loadProjectSettings()
{
	updateTitle("EDIT PROJECT");
	updateDoneButton("Save");

	ProjectSettings* projSettings = ProjectSettings::getInstance().get();

	m_projectName->setText(QString::fromStdString(projSettings->getFilePath().withoutExtension().fileName()));
	m_projectFileLocation->setText(QString::fromStdString(projSettings->getFilePath().parentDirectory().str()));

	m_sourcePaths->setList(projSettings->getSourcePaths());
	m_includePaths->setList(projSettings->getHeaderSearchPaths());

	if (m_frameworkPaths)
	{
		m_frameworkPaths->setList(projSettings->getFrameworkSearchPaths());
	}
}

void QtProjectSetupScreen::populateForm(QFormLayout* layout)
{
	int minimumWidthForSecondCol = 360;

	QLabel* nameLabel = new QLabel("Name");
	m_projectName = new QLineEdit();
	m_projectName->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_projectName->setMinimumWidth(minimumWidthForSecondCol);
	m_projectName->setAttribute(Qt::WA_MacShowFocusRect, 0);
	layout->addRow(nameLabel, m_projectName);

	QLabel* locationLabel = new QLabel("Location");
	m_projectFileLocation = new QtTextLine(this);
	m_projectFileLocation->setMinimumWidth(minimumWidthForSecondCol);
	layout->addRow(locationLabel, m_projectFileLocation);

	QLabel* languageLabel = new QLabel("Language");
	QComboBox* language = new QComboBox();
	language->insertItem(0, "C++");
	layout->addRow(languageLabel, language);

	QPushButton* helpButton;

	QWidget* sourcePathsWidget = createLabelWithHelpButton("Source Paths", &helpButton);
	connect(helpButton, SIGNAL(clicked()), this, SLOT(handleSourcePathHelpPress()));
	m_sourcePaths = new QtDirectoryListBox(this);
	m_sourcePaths->setMinimumWidth(minimumWidthForSecondCol);
	layout->addRow(sourcePathsWidget, m_sourcePaths);

	QWidget* includePathsWidget = createLabelWithHelpButton("Header\nSearch Paths", &helpButton);
	connect(helpButton, SIGNAL(clicked()), this, SLOT(handleIncludePathHelpPress()));
	m_includePaths = new QtDirectoryListBox(this);
	m_includePaths->setMinimumWidth(minimumWidthForSecondCol);
	layout->addRow(includePathsWidget, m_includePaths);

	if (QSysInfo::macVersion() != QSysInfo::MV_None)
	{
		QWidget* frameworkPathsWidget = createLabelWithHelpButton("Framework\nSearch Paths", &helpButton);
		connect(helpButton, SIGNAL(clicked()), this, SLOT(handleFrameworkPathHelpPress()));
		m_frameworkPaths = new QtDirectoryListBox(this);
		m_frameworkPaths->setMinimumWidth(minimumWidthForSecondCol);
		layout->addRow(frameworkPathsWidget, m_frameworkPaths);
	}
}

void QtProjectSetupScreen::handleCancelButtonPress()
{
	emit canceled();
}

void QtProjectSetupScreen::handleUpdateButtonPress()
{
	if (m_projectName->text().isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setText("Please enter a project name.");
		msgBox.exec();
		return;
	}

	if (m_projectFileLocation->getText().isEmpty())
	{
		QMessageBox msgBox;
		msgBox.setText("Please define the location of the project file.");
		msgBox.exec();
		return;
	}

	if (!m_sourcePaths->getList().size())
	{
		QMessageBox msgBox;
		msgBox.setText("Please add at least one source path to your project.");
		msgBox.exec();
		return;
	}

	ProjectSettings* projSettings = ProjectSettings::getInstance().get();

	projSettings->clear();

	projSettings->setSourcePaths(m_sourcePaths->getList());
	projSettings->setHeaderSearchPaths(m_includePaths->getList());

	if (m_frameworkPaths)
	{
		projSettings->setFrameworkSearchPaths(m_frameworkPaths->getList());
	}

	std::string projectFile =
		m_projectFileLocation->getText().toStdString() + "/" + m_projectName->text().toStdString() + ".coatiproject";
	projSettings->save(projectFile);

	MessageLoadProject(projectFile).dispatch();
	clear();

	emit finished();
}

void QtProjectSetupScreen::handleSourcePathHelpPress()
{
	showHelpMessage(
		"Source Paths define the files and directories that will be analysed by Coati. Usually these are the source "
		"files of your project or a subset of them."
	);
}

void QtProjectSetupScreen::handleIncludePathHelpPress()
{
	showHelpMessage(
		"Header Search Paths define where additional headers, that your project depends on, are found. Usually they are "
		"header files of frameworks or libraries that your project uses. These files won't be analysed, but Coati needs "
		"them for correct analysis.\n\n"
		"Please note that you can define Header Search Paths for all your projects in Coati's preferences."
	);
}

void QtProjectSetupScreen::handleFrameworkPathHelpPress()
{
	showHelpMessage(
		"Framework Search Paths define where MacOS framework containers, that your project depends on, are found.\n\n"
		"Please note that you can define Framework Search Paths for all your projects in Coati's preferences."
	);
}

void QtProjectSetupScreen::handlePreferencesButtonPress()
{
	emit showPreferences();
}

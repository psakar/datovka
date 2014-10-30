

#ifndef _DATOVKA_H_
#define _DATOVKA_H_

#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QProgressBar>
#include <QThread>
#include <QPushButton>
#include <QNetworkReply>

#include "src/common.h"
#include "src/io/account_db.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"
#include "thread/worker.h"

namespace Ui {
	class MainWindow;
}


class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow(void);

	void setDefaultProgressStatus(void);

	QProgressBar *m_statusProgressBar; /*!< Progress-bar object. */
	QLabel *statusOnlineLabel;
	QLabel *statusDbMode;
	QStatusBar *statusBar;

protected:
	/*!
	 * Check if some worker is working on the background and show
	 * dialog if user want to close application
	 */
	void closeEvent(QCloseEvent *event);

private slots:

	/*!
	 * @brief Version response slot.
	 */
	void datovkaVersionResponce(QNetworkReply* reply);

	/*!
	 * @brief Verify if is a connection to ISDS and databox
	 * exists for a new account.
	 */
	void getAccountUserDataboxInfo(AccountModel::SettingsMap accountInfo);

	/*!
	 * @brief Redraws widgets according to selected account item.
	 */
	void accountItemSelectionChanged(const QModelIndex &current,
	    const QModelIndex &previous = QModelIndex());

	/*!
	 * @brief Generates menu to selected account item.
	 *     (And redraws widgets.)
	 */
	void accountItemRightClicked(const QPoint &point);

	/*!
	 * @brief Sets content of widgets according to selected message.
	 */
	void messageItemSelectionChanged(const QModelIndex &current,
	    const QModelIndex &previous = QModelIndex());

	/*!
	 * @brief Generates menu to selected message item.
	 *     (And redraws widgets.)
	 */
	void messageItemRightClicked(const QPoint &point);

	/*!
	 * @brief Saves message selection.
	 */
	void messageItemStoreSelection(long msgId);

	/*!
	 * @brief Saves message selection when model changes.
	 */
	void messageItemStoreSelectionOnModelChange(void);

	/*!
	 * @brief Restores message selection.
	 */
	void messageItemRestoreSelection(void);

	/*!
	 * @brief Redraws widgets according to selected attachment item.
	 */
	void attachmentItemSelectionChanged(const QModelIndex &current,
	    const QModelIndex &previous = QModelIndex());

	/*!
	 * @brief Generates menu to selected message item.
	 *     (And redraws widgets.)
	 */
	void attachmentItemRightClicked(const QPoint &point);

	/*!
	 * @brief Handle attachment double click.
	 */
	void attachmentItemDoubleClicked(const QModelIndex &index);

	/*!
	 * @brief Saves selected attachment to file.
	 */
	void saveSelectedAttachmentToFile(void);

	/*!
	 * @brief Save all attachments to dir.
	 */
	void saveAllAttachmentsToDir(void);

	/*!
	 * @brief Open attachment in default application.
	 */
	void openSelectedAttachment(void);

	/*!
	 * @brief Downloads the attachments for the selected message.
	 */
	void downloadSelectedMessageAttachments(void);

	/*!
	 * @brief Mark all messages as read in selected account item.
	 */
	void accountItemMarkAllRead(void);

	/*!
	 * @brief Deletes selected message from message list.
	 */
	void messageItemDeleteMessage(void);

	/*!
	 * @brief Downloads new messages from server for all accounts.
	 */
	void synchroniseAllAccounts(void);

	/*!
	 * @brief Downloads new messages from server for selected account.
	 */
	void synchroniseSelectedAccount(void);

	/*!
	 * @brief Creates and sends new message.
	 */
	void createAndSendMessage(void);

	/*!
	 * @brief Creates and sends a message reply for selected account.
	 */
	void createAndSendMessageReply(void);

	/*!
	 * @brief Verifies selected message and creates response dialog.
	 */
	void verifyMessage(void);

	/*!
	 * @brief Shows account properties dialog.
	 */
	void manageAccountProperties(void);

	/*!
	 * @brief Shows change password dialog.
	 */
	void changeAccountPassword(void);

	/*!
	 * @brief Shows the application preferences dialog.
	 */
	void applicationPreferences(void);

	/*!
	 * @brief Add account action and dialog.
	 */
	void addNewAccount(void);

	/*!
	 * @brief Deletion confirmation dialog.
	 */
	void deleteSelectedAccount(void);

	/*!
	 * @brief Import database directory dialog.
	 */
	void importDatabaseDirectory(void);

	/*!
	 * @brief Proxy setting dialog.
	 */
	void proxySettings(void);

	/*!
	 * @brief Move selected account up.
	 */
	void moveSelectedAccountUp(void);

	/*!
	 * @brief Move selected account down.
	 */
	void moveSelectedAccountDown(void);

	/*!
	 * @brief Change data directory dialog.
	 */
	void changeDataDirectory(void);

	/*!
	 * @brief View signature details.
	 */
	void showSignatureDetails(void);

	/*!
	 * @brief Export message into as ZFO file dialog.
	 */
	void exportSelectedMessageAsZFO(void);

	/*!
	 * @brief Export delivery information as ZFO file dialog.
	 */
	void exportDeliveryInfoAsZFO(void);

	/*!
	 * @brief Export delivery information as PDF file dialog.
	 */
	void exportDeliveryInfoAsPDF(void);

	/*!
	 * @brief Export selected message envelope as PDF file dialog.
	 */
	void exportMessageEnvelopeAsPDF(void);

	/*!
	 * @brief Open selected message in external application.
	 */
	void openSelectedMessageExternally(void);

	/*!
	 * @brief Open delivery information externally.
	 */
	void openDeliveryInfoExternally(void);

	/*!
	 * @brief Search data box dialog.
	 */
	void findDatabox(void);

	/*!
	 * @brief Authenticate message file dialog.
	 */
	void authenticateMessageFile(void);

	/*!
	 * @brief View message from file dialog.
	 */
	void viewMessageFromZFO(void);

	/*!
	 * @brief Export correspondence overview dialog.
	 */
	void exportCorrespondenceOverview(void);

	/*!
	 * @brief About application dialog.
	 */
	void aboutApplication(void);

	/*!
	 * @brief Show help.
	 */
	void showHelp(void);

	/*!
	 * @brief Clear message filter field.
	 */
	void clearFilterField(void);

	/*!
	 * @brief Filter listed messages.
	 */
	void filterMessages(const QString &text);

	/*!
	 * @brief Set new sent/received message column widths.
	 */
	void onTableColumnResized(int index, int oldSize, int newSize);

	/*!
	 * @brief Set actual sort order for current column.
	 */
	void onTableColumnSort(int column);

	/*!
	 * @brief Delete worker and thread objects, enable sync buttons.
	 */
	void deleteThreadSyncAll(void);

	/*!
	 * @brief Delete worker and thread objects, enable sync buttons.
	 */
	void deleteThreadSyncOne(void);

	/*!
	 * @brief Delete worker and thread objects, enable sync buttons.
	 */
	void deleteThreadDownMsgComplete(void);

	/*!
	 * @brief Set ProgressBar value and Status bar text.
	 */
	void setProgressBarFromWorker(QString label, int value);

	/*!
	 * @brief Refresh AccountList.
	 */
	void refreshAccountListFromWorker(const QModelIndex acntTopIdx);

	/*!
	 * @brief Set and run any actions after main window has been created.
	 */
	void setWindowsAfterInit(void);

	/*!
	 * @brief Receive and store new account database path. Change data
	 *     directory path in settings.
	 */
	void receiveNewDataPath(QString oldDir, QString newDir,
	    QString action);

	/*!
	 * @brief Get data about logged in user and his box.
	 */
	bool getOwnerInfoFromLogin(const QModelIndex &acntTopIdx);

	/*!
	 * @brief Set and show correspondence error dialog.
	 */
	void setAndShowNotificationDialog(QList<QString> errorDmId,
	   int successCnt);

	/*!
	 * @brief Set tablewidget when message download worker is done.
	 */
	void postDownloadSelectedMessageAttachments(const QModelIndex acntTopIdx,
	    QString dmId);

	/*!
	 * @brief Set info status bar from worker.
	 */
	void dataFromWorkerToStatusBarInfo(bool completed, QString accoutName,
	    int rt, int rn, int st, int sn);

private:

	QThread *threadSyncAll, *threadSyncOne, *threadDownMsgComplete;
	Worker *workerSyncAll, *workerSyncOne, *workerDownMsgComplete;
	QTimer *timer;
	int timeout;

	/*!
	 * @brief Connects top menu-bar buttons to appropriate actions.
	 */
	void connectTopMenuBarSlots(void);

	/*!
	 * @brief Connect top tool-bar buttons to appropriate actions.
	 */
	void connectTopToolBarSlots(void);

	/*!
	 * @brief Connect message-action-bar buttons to appropriate actions.
	 */
	void connectMessageActionBarSlots(void);

	/*!
	 * @brief Default settings of main window.
	 */
	void defaultUiMainWindowSettings(void) const;

	/*!
	 * @brief Set message action/button to visible.
	 */
	void setMessageActionVisibility(bool action) const;

	/*!
	 * @brief Active/Inactive account menu and buttons in the mainwindow.
	 */
	void activeAccountMenuAndButtons(bool action) const;

	/*!
	 * @brief Create configuration file if not present.
	 */
	void ensureConfPresence(void) const;

	/*!
	 * @brief Load and apply setting from configuration file.
	 */
	void loadSettings(void);

	/*!
	 * @brief Store current setting to configuration file.
	 */
	void saveSettings(void) const;

	/*!
	 * @brief Sets geometry from settings.
	 */
	void loadWindowGeometry(const QSettings &settings);

	/*!
	 * @brief Set default account from settings.
	 */
	void setDefaultAccount(const QSettings &settings);

	/*!
	 * @brief Load sent/received messages column widths from settings.
	 */
	void loadSentReceivedMessagesColumnWidth(const QSettings &settings);

	/*!
	 * @brief Load last directory paths from settings.
	 */
	void loadLastDirectoryPaths(const QSettings &settings);

	/*!
	 * @brief Load collapse info of account items from settings.
	 */
	void loadAccountCollapseInfo(QSettings &settings);

	/*!
	 * @brief Save collapse info of account items into settings.
	 */
	void saveAccountCollapseInfo(QSettings &settings) const;

	/*!
	 * @brief Save application ID and config format
	 */
	void saveAppIdConfigFormat(QSettings &settings) const;

	/*!
	 * @brief Save sent/received messages column widths into settings.
	 */
	void saveSentReceivedColumnWidth(QSettings &settings) const;

	/*!
	 * @brief Set received message column widths.
	 */
	void setReciveidColumnWidths(void);

	/*!
	 * @brief Set sent message column widths.
	 */
	void setSentColumnWidths(void);

	/*!
	 * @brief Store geometry to settings.
	 */
	void saveWindowGeometry(QSettings &settings) const;

	/*!
	 * @brief Store current account user name to settings.
	 */
	void saveAccountIndex(QSettings &settings) const;

	/*!
	 * @brief Update numbers of unread messages in account model.
	 *
	 * param[in] index Index identifying account.
	 * @return True on success.
	 *
	 * @note Does not add/remove any nodes, just updates the counts.
	 */
	bool updateExistingAccountModelUnread(QModelIndex index);

	/*!
	 * @brief Partially regenerates account model according to the database
	 *     content.
	 *
	 * @param[in] index Index identifying account.
	 * @return True on success.
	 *
	 * @note This function adds/removes nodes and does not set the
	 *     currentIndex back to its original position.
	 */
	bool regenerateAccountModelYears(QModelIndex index);

	/*!
	 * @brief Regenerates account model according to the database content.
	 *
	 * @return True on success.
	 *
	 * @note This function add/removes nodes and does not set the
	 *     currentIndex back to its original position.
	 */
	bool regenerateAllAccountModelYears(void);

	/*!
	 * @brief Generate account info HTML message.
	 */
	QString createAccountInfo(const QStandardItem &topItem) const;

	/*!
	 * @brief Generate overall account information.
	 */
	QString createAccountInfoAllField(const QString &accountName,
	    const QList< QPair<QString, int> > &receivedCounts,
	    const QList< QPair<QString, int> > &sent) const;

	/*!
	 * @brief Generate banner.
	 */
	QString createDatovkaBanner(const QString &version) const;

	/*!
	 * @brief Returns user name related to given account item.
	 * TODO - If accountItem is 0 then currently selected account is
	 *     processed.
	 */
	QString accountUserName(const QStandardItem *accountItem = 0) const;

	/*!
	 * @brief Get message db to selected account item.
	 *
	 * @note If accountItem is 0 then currently selected account is
	 *     processed.
	 */
	MessageDb * accountMessageDb(const QStandardItem *accountItem);

	/*!
	 * @brief Delete message from long term storage in ISDS.
	 */
	qdatovka_error eraseMessage(const QModelIndex &acntTopIdx, QString dmId);

	/*!
	 * @brief Verify message. Compare hash with hash stored in ISDS.
	 */
	qdatovka_error verifySelectedMessage(const QModelIndex &acntTopIdx,
	    const QModelIndex &msgIdx);

	/*!
	 * @brief Get data about logged in user.
	 */
	bool getUserInfoFromLogin(const QModelIndex &acntTopIdx);

	/*!
	 * @brief Authenticate message from ZFO file.
	 */
	qdatovka_error authenticateMessageFromZFO(void);

	/*!
	 * @brief Show message info for user if connection to ISDS fails.
	 */
	void showConnectionErrorMessageBox(int status, QString accountName);

	/*!
	 * @brief Check if connection to ISDS fails.
	 */
	bool checkConnectionError(int status, QString accountName,
	    bool showDialog);

	/*!
	 * @brief connect to ISDS databox from exist account
	 */
	bool connectToIsds(const QModelIndex acntTopIdx, bool showDialog);

	/*!
	 * @brief connect to ISDS databox from new account
	 */
	bool firstConnectToIsds(AccountModel::SettingsMap accountInfo,
	    bool showDialog);

	/*!
	 * @brief Login to ISDS server by username and password only.
	 */
	bool loginMethodUserNamePwd(const QModelIndex acntTopIdx,
	   const AccountModel::SettingsMap accountInfo, bool showDialog);

	/*!
	 * @brief Login to ISDS server by username, password and OTP code.
	 */
	bool loginMethodUserNamePwdOtp(const QModelIndex acntTopIdx,
	    const AccountModel::SettingsMap accountInfo, bool showDialog);

	/*!
	 * @brief Login to ISDS server by certificate only.
	 */
	bool loginMethodCertificateOnly(const QModelIndex acntTopIdx,
	    const AccountModel::SettingsMap accountInfo, bool showDialog);

	/*!
	 * @brief Login to ISDS server by certificate, username and password.
	 */
	bool loginMethodCertificateUserPwd(const QModelIndex acntTopIdx,
	    const AccountModel::SettingsMap accountInfo, bool showDialog);

	/*!
	 * @brief Login to ISDS server by certificate and databox ID.
	 */
	bool loginMethodCertificateIdBox(const QModelIndex acntTopIdx,
	    const AccountModel::SettingsMap accountInfo, bool showDialog);

	/*!
	 * @brief Sent and check a new version of Datovka.
	 */
	void checkNewDatovkaVersion(QString version);

	/*!
	 * @brief Download complete message synchronously
	 * without worker and thread.
	 */
	bool downloadCompleteMessage(QString dmId);

	QString m_confDirName; /*!< Configuration directory location. */
	QString m_confFileName; /*!< Configuration file location. */

	AccountModel m_accountModel; /*!<
	                              * Account tree view model. Generated from
	                              * configuration file.
	                              */
	AccountDb m_accountDb; /*!< Account information database. */
	DbMsgsTblModel *m_messageModel; /*!< Currently displayed model. */
	dbContainer m_messageDbs; /*!< Map of message databases. */
	QLineEdit *m_searchLine; /*!< Search-line object. */
	QPushButton *m_pushButton;
	QSortFilterProxyModel m_messageListProxyModel; /*!<
	                                                * Used for message
	                                                * sorting.
	                                                */

	long m_lastSelectedMessageId; /*!< Id of the last selected message. */
	long m_lastStoredMessageId; /*!< Last stored message selection. */

	int m_received_1;
	int m_received_2;
	int m_sent_1;
	int m_sent_2;
	int m_sort_column;
	QString m_sort_order;
	QString m_export_correspond_dir;
	QString m_on_export_zfo_activate;
	QString m_on_import_database_dir_activate;
	bool isMainWindow;

	Ui::MainWindow *ui;
};


#endif /* _DATOVKA_H_ */

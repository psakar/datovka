/*
 * Copyright (C) 2014-2017 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#ifndef _DATOVKA_H_
#define _DATOVKA_H_

#include <QItemSelection>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QSet>
#include <QStandardItemModel>
#include <QProgressBar>
#include <QTimer>
#include <QPushButton>
#include <QNetworkReply>

#include "src/common.h"
#include "src/io/account_db.h"
#include "src/io/exports.h"
#include "src/io/message_db.h"
#include "src/io/message_db_set.h"
#include "src/gui/dlg_import_zfo.h"
#include "src/gui/dlg_timestamp_expir.h"
#include "src/models/accounts_model.h"
#include "src/models/sort_filter_proxy_model.h"
#include "src/settings/preferences.h"
#include "src/single/single_instance.h"
#include "src/web/json.h"
#include "src/worker/task.h" /* TODO -- remove this header file. */

/* Forward declaration as we don;t wan to pull-in all header file content. */
class IsdsSessions;

namespace Ui {
	class MainWindow;
}


class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = Q_NULLPTR);
	~MainWindow(void);

	/*!
	 * @brief Create configuration file if not present.
	 */
	static
	bool ensureConfPresence(void);

	/*!
	 * @brief Connects to ISDS and downloads basic information about the
	 *     user.
	 *
	 * @param userName Account login.
	 */
	bool connectToIsds(const QString &userName);

	/*!
	 * @brief Get message db set related to given account.
	 *
	 * @note If pointer to main window is not specified, then all dialogues
	 *     will be suppressed.
	 *
	 * @param userName Account login.
	 * @param mw       Pointer to main window.
	 */
	static
	MessageDbSet *accountDbSet(const QString &userName, MainWindow *mw);

	/*!
	 * @brief Get account list from Webdatovka.
	 */
	bool wdGetAccountList(const QString &userName,
	    const QNetworkCookie &sessionid, bool syncWithAll);

protected:
	/*!
	 * @brief Check if some worker is working on the background and show
	 * dialog if user want to close application
	 */
	virtual
	void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

	/*!
	 * @brief Store original window position on window maximisation.
	 */
	virtual
	void moveEvent(QMoveEvent *event) Q_DECL_OVERRIDE;

	/*!
	 * @brief Store original window size on window maximisation.
	 */
	virtual
	void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

	/*!
	 * @brief Check window geometry, adjust size if window is too large for
	 *     desktop..
	 */
	virtual
	void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

public slots:

	/*!
	 * @brief Login to mojeID.
	 */
	void loginToMojeId(const QString &userName);

private slots:
	/*!
	 * @brief Refresh AccountList.
	 */
	void refreshAccountList(const QString &userName);

	/*!
	 * @brief Processes messages from single instance emitter.
	 */
	void processSingleInstanceMessages(const QString &message);

	/*!
	 * @brief Fired when workers performing tasks on background have
	 *     finished.
	 */
	void backgroundWorkersFinished(void);

	/*!
	 * @brief Performs action depending on message download outcome.
	 */
	void collectDownloadMessageStatus(const QString &usrName, qint64 msgId,
	    const QDateTime &deliveryTime, int result, const QString &errDesc,
	    bool listScheduled);

	/*!
	 * @brief Performs action depending on message download outcome
	 *        for webdatovka.
	 */
	void collectDownloadMessageMojeId(const QString &usrName,
	    qint64 msgId, int result, const QString &errDesc,
	    bool listScheduled);

	/*!
	 * @brief Performs action depending on message list download outcome.
	 */
	void collectDownloadMessageListStatus(const QString &usrName,
	    int direction, int result, const QString &errDesc,
	    bool add, int rt, int rn, int st, int sn);

	/*!
	 * @brief Collects information about import status.
	 */
	void collectImportZfoStatus(const QString &fileName, int result,
	    const QString &resultDesc);

	/*!
	 * @brief Performs action depending on message send outcome.
	 */
	void collectSendMessageStatus(const QString &userName,
	    const QString &transactId, int result, const QString &resultDesc,
	    const QString &dbIDRecipient, const QString &recipientName,
	    bool isPDZ, qint64 dmId);

	/*!
	 * @brief Performs action depending on webdatovka message send outcome.
	 */
	void sendMessageMojeIdAction(const QString &userName,
	    const QStringList &result, const QString &error);

	/*!
	 * @brief Version response slot.
	 */
	void datovkaVersionResponce(QNetworkReply* reply);

	/*!
	 * @brief Redraws widgets according to selected account item.
	 */
	void accountItemCurrentChanged(const QModelIndex &current,
	    const QModelIndex &previous = QModelIndex());

	/*!
	 * @brief Generates menu to selected account item.
	 *     (And redraws widgets.)
	 */
	void accountItemRightClicked(const QPoint &point);

	/*!
	 * @brief Sets contents of widgets according to selected messages.
	 */
	void messageItemsSelectionChanged(const QItemSelection &selected,
	    const QItemSelection &deselected = QItemSelection());

	/*!
	 * @brief Used for toggling the message read state.
	 */
	void messageItemClicked(const QModelIndex &index);

	/*!
	 * @brief Generates menu to selected message item.
	 *     (And redraws widgets.)
	 */
	void messageItemRightClicked(const QPoint &point);

	/*!
	 * @brief Handle message double click.
	 */
	void viewSelectedMessage(void);

	/*!
	 * @brief Saves message selection.
	 */
	void messageItemStoreSelection(qint64 msgId);

	/*!
	 * @brief Saves message selection when model changes.
	 */
	void messageItemStoreSelectionOnModelChange(void);

	/*!
	 * @brief Restores message selection.
	 */
	void messageItemRestoreSelectionOnModelChange(void);

	/*!
	 * @brief Restores message selection.
	 */
	void messageItemRestoreSelectionAfterLayoutChange(void);

	/*!
	 * @brief Select account via userName and focus on
	 *        message ID from search selection
	 */
	void messageItemFromSearchSelection(const QString &userName,
	    qint64 msgId, const QString &deliveryYear, int msgType);

	/*!
	 * @brief Redraws widgets according to attachment item selection.
	 */
	void attachmentItemsSelectionChanged(const QItemSelection &selected,
	    const QItemSelection &deselected = QItemSelection());

	/*!
	 * @brief Generates menu to selected message item.
	 *     (And redraws widgets.)
	 */
	void attachmentItemRightClicked(const QPoint &point);

	/*!
	 * @brief Saves selected attachments to file.
	 */
	void saveSelectedAttachmentsToFile(void);

	/*!
	 * @brief Save all attachments to dir.
	 */
	void saveAllAttachmentsToDir(void);

	/*!
	 * @brief Open attachment in default application.
	 */
	void openSelectedAttachment(const QModelIndex &index = QModelIndex());

	/*!
	 * @brief Mark all received messages in the current working account.
	 */
	void accountMarkReceivedRead(void);

	/*!
	 * @brief Mark all received messages in the current working account.
	 */
	void accountMarkReceivedUnread(void);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkReceivedYearRead(void);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkReceivedYearUnread(void);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkRecentReceivedRead(void);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkRecentReceivedUnread(void);

	/*!
	 * @brief Mark all received messages in the current working account.
	 */
	void accountMarkReceivedUnsettled(void);

	/*!
	 * @brief Mark all received messages in the current working account.
	 */
	void accountMarkReceivedInProgress(void);

	/*!
	 * @brief Mark all received messages in the current working account.
	 */
	void accountMarkReceivedSettled(void);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkReceivedYearUnsettled(void);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkReceivedYearInProgress(void);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkReceivedYearSettled(void);

	/*!
	 * @brief Mark recently received messages in the current
	 *     working account.
	 */
	void accountMarkRecentReceivedUnsettled(void);

	/*!
	 * @brief Mark recently received messages in the current
	 *     working account.
	 */
	void accountMarkRecentReceivedInProgress(void);

	/*!
	 * @brief Mark recently received messages in the current
	 *     working account.
	 */
	void accountMarkRecentReceivedSettled(void);

	/*!
	 * @brief Mark selected messages as read.
	 */
	void messageItemsSelectedMarkRead(void);

	/*!
	 * @brief Mark selected messages as unread.
	 */
	void messageItemsSelectedMarkUnread(void);

	/*!
	 * @brief Mark selected messages as unsettled.
	 */
	void messageItemsSelectedMarkUnsettled(void);

	/*!
	 * @brief Mark selected messages as in progress.
	 */
	void messageItemsSelectedMarkInProgress(void);

	/*!
	 * @brief Mark selected messages as settled.
	 */
	void messageItemsSelectedMarkSettled(void);

	/*!
	 * @brief Delete selected message(s) from local database and ISDS.
	 */
	void deleteMessage(void);

	/*!
	 * @brief Delete selected message(s) from local database and Webdatovka.
	 */
	void deleteMessageWebdatovka(const QString &userName);

	/*!
	 * @brief Downloads new messages from server for all accounts.
	 */
	void synchroniseAllAccounts(void);

	/*!
	 * @brief Downloads new messages from server for selected account.
	 *
	 * @param[in] userName Account user name. If empty string is supplied,
	 *                     then selected account will be determined from
	 *                     the account list.
	 * @return True if some actions have been planned.
	 */
	bool synchroniseSelectedAccount(QString userName = QString());

	/*!
	 * @brief Downloads the attachments for the selected message.
	 */
	void downloadSelectedMessageAttachments(void);

	/*!
	 * @brief Create and send a new message form selected account.
	 */
	void createAndSendMessage(void);

	/*!
	 * @brief Create reply from the selected message.
	 */
	void createAndSendMessageReply(void);

	/*!
	 * @nrief Create a message containing as attachment the selected
	 *     messages on ZFO format.
	 */
	void createAndSendMessageWithZfos(void);

	/*!
	 * @brief Create message from the template (selected message).
	 */
	void createAndSendMessageFromTmpl(void);

	/*!
	 * @brief Verifies selected message and creates response dialog.
	 */
	void verifySelectedMessage(void);

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
	void showPreferencesDialog(void);

	/*!
	 * @brief Show add new account dialog.
	 */
	void showAddNewAccountDialog(void);

	/*!
	 * @brief Add mojeID account action and dialog.
	 */
	void addNewMojeIDAccount(void);

	/*!
	 * @brief Deletion confirmation dialog.
	 */
	void deleteSelectedAccount(void);

	/*!
	 * @brief Delete account.
	 */
	void deleteAccount(const QString &userName);

	/*!
	 * @brief Show import database directory dialog.
	 */
	void showImportDatabaseDialog(void);

	/*!
	 * @brief Prepare import database directory.
	 */
	void prepareCreateAccountFromDatabaseFile(bool fromDirectory);

	/*!
	 * @brief Prepare import of messages from database.
	 */
	void prepareMsgsImportFromDatabase(void);

	/*!
	 * @brief Proxy settings dialogue.
	 */
	void showProxySettingsDialog(void);

	/*!
	 * @brief Document service dialogue.
	 */
	void showDocumentServiceDialogue(void);

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
	void showSignatureDetailsDialog(void);

	/*!
	 * @brief Export selected message into ZFO files.
	 */
	void exportSelectedMessagesAsZFO(void);

	/*!
	 * @brief Export selected delivery information as ZFO files.
	 */
	void exportSelectedDeliveryInfosAsZFO(void);

	/*!
	 * @brief Export selected delivery information as PDF files.
	 */
	void exportSelectedDeliveryInfosAsPDF(void);

	/*!
	 * @brief Export selected message envelope as PDF files.
	 */
	void exportSelectedMessageEnvelopesAsPDF(void);

	/*!
	 * @brief Export selected message envelope as PDF and attachment files.
	 */
	void exportSelectedMessageEnvelopeAttachments(void);

	/*!
	 * @brief Sends selected messages as ZFO into default e-mail client.
	 */
	void sendMessagesZfoEmail(void);

	/*!
	 * @brief Sends all attachments of selected messages into default
	 *     e-mail client.
	 */
	void sendAllAttachmentsEmail(void);

	/*!
	 * @brief Sends selected attachments into default e-mail client.
	 */
	void sendAttachmentsEmail(void);

	/*!
	 * @brief Open selected message in external application.
	 */
	void openSelectedMessageExternally(void);

	/*!
	 * @brief Open delivery information externally.
	 */
	void openDeliveryInfoExternally(void);

	/*!
	 * @brief Obtain information about stored messages from document service.
	 */
	void getStoredMsgInfoFromDocumentService(void);

	/*!
	 * @brief Upload message into document service.
	 */
	void uploadSelectedMessageToDocumentService(void);

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
	void showViewMessageFromZFODialog(void);

	/*!
	 * @brief Export correspondence overview dialog.
	 */
	void showExportCorrespondenceOverviewDialog(void);

	/*!
	 * @brief Show dialog with settings of import ZFO file(s) into database.
	 */
	void showImportZFOActionDialog(void);

	/*!
	 * @brief About application dialog.
	 */
	void showAboutApplicationDialog(void);

	/*!
	 * @brief Show help/manual in a internet browser.
	 */
	void showAppHelpInTheBrowser(void);

	/*!
	 * @brief Go to homepage.
	 */
	void goHome(void);

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
	void onTableColumnHeaderSectionClicked(int column);

	/*!
	 * @brief Set ProgressBar value and Status bar text.
	 */
	void updateProgressBar(const QString &label, int value);

	/*!
	 * @brief Set Status bar text.
	 */
	void updateStatusBarText(const QString &text);

	/*!
	 * @brief Clear progress bar text.
	 */
	void clearProgressBar(void);

	/*!
	 * @brief Clear status bar text.
	 */
	void clearStatusBar(void);

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
	 * @brief set message process state into db
	 */
	void msgSetSelectedMessageProcessState(int stateIndex);

	/*!
	 * @brief Show advanced message search dialogue.
	 */
	void showMsgAdvancedSearchDialog(void);

	/*!
	 * @brief On message dialogue exit.
	 */
	void msgAdvancedDlgFinished(int result);

	/*!
	 * @brief Show message timestamp expiration dialog.
	 */
	void showMsgTmstmpExpirDialog(void);

	/*!
	 * @brief Prepare message timestamp expiration based on action.
	 */
	void prepareMsgTmstmpExpir(enum TimestampExpirDialog::TSaction action);

	/*!
	 * @brief Split message database slot.
	 */
	void splitMsgDbByYearsSlot(void);

	/*!
	 * @brief Store last add attachment path.
	 */
	void storeAttachmentPath(const QString &userName,
	    const QString &lastDir);

	/*!
	 * @brief Show tags manager dialog for tags settings.
	 */
	void showTagDialog(void);

	/*!
	 * @brief Add/delete tags to/from selected messages.
	 */
	void addOrDeleteMsgTags(void);

	/*!
	 * @brief Vacuum message database.
	 */
	void vacuumMsgDbSlot(void);

	void callMojeId(const QString &user, const QString &lastUrl,
	    const QString &token, QString userName, QString pwd, QString otp,
	    bool syncALL, const QString &certPath);

	/*!
	 * @brief Show information about import message results.
	 */
	void showImportMessageResults(const QString &userName,
	    const QStringList &errImportList, int totalMsgs, int importedMsgs);

private:

	QTimer m_timerSyncAccounts;
	int m_timeoutSyncAccounts;

	/*!
	 * @brief Shows tag editing dialogue.
	 *
	 * @no If no message identifiers are supplied then just the tags are
	 *     edited.
	 *
	 * @param[in] userName Account login string. Must be supplied when
	 *                     message identifiers are passed.
	 * @param[in] msgIdList Messages whose tags should be edited.
	 * @param[in] msgIdWebDatovkaList Webdatovka messages whose tags should
	 *                                be edited.
	 */
	void modifyTags(const QString &userName, QList<qint64> msgIdList,
	    QList<qint64> msgIdWebDatovkaList);

	/*!
	 * @brief Get message list from webdatovka server.
	 */
	bool wdGetMessageList(const QString &userName);

	/*!
	 * @brief Do sync of account in the webdatovka server.
	 */
	bool wdSyncAccount(const QString &userName);

	/*!
	 * @brief Show status bar text with timeout.
	 */
	void showStatusTextWithTimeout(const QString &qStr);

	/*!
	 * @brief Show status bar text permanently.
	 */
	void showStatusTextPermanently(const QString &qStr);

	/*!
	 * @brief Verify whether a connection to ISDS can be establisehd
	 *     and databox exists for the account.
	 */
	void getAccountUserDataboxInfo(AcntSettings accountInfo);

	/*!
	 * @brief Used to view selected message via event filter.
	 *
	 * @param[in] mwPtr Pointer to main window.
	 */
	static
	void viewSelectedMessageViaFilter(QObject *mwPtr);

	/*!
	 * @brief Set info status bar from worker.
	 */
	void dataFromWorkerToStatusBarInfo(bool add,
	    int rt, int rn, int st, int sn);

	/*!
	 * @brief Set tablewidget when message download worker is done.
	 */
	void postDownloadSelectedMessageAttachments(const QString &userName,
	    qint64 dmId);

	/*!
	 * @brief Save attachment identified by indexes to file.
	 */
	void saveAttachmentToFile(const QString &userName,
	    const MessageDb::MsgId &msgId, const QModelIndex &attIdx);

	/*!
	 * @brief Return index for yearly entry with given properties.
	 */
	QModelIndex accountYearlyIndex(const QString &userName,
	    const QString &year, int msgType);

	/*!
	 * @brief Return index for message with given properties.
	 */
	QModelIndex messageIndex(qint64 msgId) const;

	/*!
	 * @brief Check message time stamp expiration for account.
	 */
	void checkMsgsTmstmpExpiration(const QString &userName,
	    const QStringList &filePathList);

	/*!
	 * @brief Mark all received messages in the current working account.
	 */
	void accountMarkReceivedLocallyRead(bool read);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkReceivedYearLocallyRead(bool read);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkRecentReceivedLocallyRead(bool read);

	/*!
	 * @brief Mark all received messages in the current working account.
	 */
	void accountMarkReceivedProcessState(enum MessageProcessState state);

	/*!
	 * @brief Mark all received messages in given year in the current
	 *     working account.
	 */
	void accountMarkReceivedYearProcessState(
	    enum MessageProcessState state);

	/*!
	 * @brief Mark recently received messages in the current
	 *     working account.
	 */
	void accountMarkRecentReceivedProcessState(
	    enum MessageProcessState state);

	/*!
	 * @brief Connects top menu-bar buttons to appropriate actions.
	 */
	void connectTopMenuBarSlots(void);

	/*!
	 * @brief Connect message-action-bar buttons to appropriate actions.
	 */
	void connectMessageActionBarSlots(void);

	/*!
	 * @brief Default settings of main window.
	 */
	void defaultUiMainWindowSettings(void) const;

	/*!
	 * @brief Enables message menu actions according to message selection.
	 */
	void setMessageActionVisibility(int numSelected) const;

	/*!
	 * @brief Enables message menu actions according to attachment selection.
	 */
	void setAttachmentActionVisibility(int numSelected) const;

	/*!
	 * @brief Show send message dialog and send message(s).
	 *
	 * @param[in] action Defines send action (new,reply,forward,template).
	 */
	void showSendMessageDialog(int action);

	/*!
	 * @brief Active/Inactive account menu and buttons in the mainwindow.
	 */
	void activeAccountMenuAndButtons(bool action) const;

	/*!
	 * @brief Load and apply settings from configuration file.
	 */
	void loadSettings(void);

	/*!
	 * @brief Store current settings to configuration file.
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
	void setReceivedColumnWidths(void);

	/*!
	 * @brief Set sent message column widths.
	 */
	void setSentColumnWidths(void);

	/*!
	 * @brief Saves account export paths.
	 */
	void storeExportPath(const QString &userName);

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
	bool updateExistingAccountModelUnread(const QModelIndex &index);

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
	bool regenerateAccountModelYears(const QModelIndex &index);

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
	QString createAccountInfo(const QString &userName);

	/*!
	 * @brief Generate overall account information.
	 */
	QString createAccountInfoAllField(const QString &accountName,
	    const QList< QPair<QString, int> > &receivedCounts,
	    const QList< QPair<QString, int> > &sentCounts) const;

	/*!
	 * @brief Generate overall account information only for sent or
	 *     received messages.
	 */
	QString createAccountInfoMessagesCount(const QString &accountName,
	    const QList< QPair<QString, int> > &counts,
	    enum MessageDb::MessageType type) const;

	/*!
	 * @brief Generate banner.
	 */
	QString createDatovkaBanner(const QString &version) const;

	/*!
	 * @brief Set export paths to selected account item.
	 */
	void setAccountStoragePaths(const QString &userName);

	/*!
	 * @brief Delete message from long term storage in ISDS and
	 * local database - based on delFromIsds parameter.
	 *
	 * @return True if message has been removed from local database.
	 */
	bool eraseMessage(const QString &userName,
	    const MessageDb::MsgId &msgId, bool delFromIsds);

	/*!
	 * @brief Authenticate message from ZFO file.
	 */
	int authenticateMessageFromZFO(void);

	/*!
	 * @brief Performs a ISDS log-in operation.
	 *
	 * @param[in,out] isdsSessions Sessions container reference.
	 * @param[in]     acntSettings Account settings reference.
	 * @return True on successful login.
	 */
	bool logInGUI(IsdsSessions &isdsSessions, AcntSettings &acntSettings);

	/*!
	 * @brief connect to ISDS databox from new account
	 */
	bool firstConnectToIsds(AcntSettings &accountInfo);

	/*!
	 * @brief Sent and check a new version of Datovka.
	 */
	void checkNewDatovkaVersion(void);

	/*!
	 * @brief Sent and check a new version of Datovka.
	 */
	void createAccountFromDatabaseFileList(
	    const QStringList &filePathList);

	/*!
	 * @brief Download complete message synchronously
	 * without worker and thread.
	 *
	 * @note Delivery time may change if invalid given.
	 *
	 * @param[in,out] msgId Message identifier.
	 * @return True on success.
	 */
	bool downloadCompleteMessage(MessageDb::MsgId &msgId);

	/*!
	 * @brief Shows notification dialogue and offers downloading of
	 *     missing message.
	 *
	 * @note Delivery time may change if invalid given.
	 *
	 * @param[in,out] msgId Message identifier.
	 * @return True on success.
	 */
	bool messageMissingOfferDownload(MessageDb::MsgId &msgId,
	    const QString &title);

	/*!
	 * @brief Set read status to messages with given indexes.
	 */
	void messageItemsSetReadStatus(
	    const QModelIndexList &firstMsgColumnIdxs, bool read);

	/*!
	 * @brief Set process status to messages with given indexes.
	 */
	void messageItemsSetProcessStatus(
	    const QModelIndexList &firstMsgColumnIdxs,
	    enum MessageProcessState state);

	/*!
	 * @brief Show dialogue that notifies the user about expiring password.
	 */
	int showDialogueAboutPwdExpir(const QString &accountName,
	    const QString &userName, qint64 days, const QDateTime &dateTime);

	/*!
	 * @brief Export message with expired time stamp to ZFO.
	 */
	void exportExpirMessagesToZFO(const QString &userName,
	    const QList<MessageDb::MsgId> &expirMsgIds);

	/*!
	 * @brief Show dialog for webdatovka account
	 *        that action is not implemented.
	 * @param[in] userName - account username.
	 * @param[in] txt        additional info text.
	 */
	void showWebDatovkaInfoDialog(const QString &userName, QString txt);

	/*!
	 * @brief Test if exists another mojeID account with same userId
	 *        when we delete any mojeID acccount.
	 *
	 * @param[in] userName - account username.
	 * @return true if any account exists
	 */
	bool existsAnotherMojeIdAccountWithSameUserId(const QString &userName);

	/*!
	 * @brief Export selected messages to disk.
	 *
	 * @param[in] expFileType - export file type.
	 */
	void doExportOfSelectedFiles(enum Exports::ExportFileType expFileType);

	/*
	 * @brief Show ZFO import notification dialog with results of import.
	 */
	void showImportZfoResultDialogue(int filesCnt,
	    const QList<QPair<QString,QString>> &successFilesList,
	    const QList<QPair<QString,QString>> &existFilesList,
	    const QList<QPair<QString,QString>> &errorFilesList);

	QString m_confDirName; /*!< Configuration directory location. */
	QString m_confFileName; /*!< Configuration file location. */

	AccountModel m_accountModel; /*!<
	                              * Account tree view model. Generated from
	                              * configuration file.
	                              */
	SortFilterProxyModel m_messageListProxyModel; /*!<
	                                               * Used for message
	                                               * sorting and filtering.
	                                               */

	QTimer m_messageMarker; /*!< Used for marking messages as read. */
	qint64 m_lastSelectedMessageId; /*!< Id of the last selected message. */
	qint64 m_lastStoredMessageId; /*!< Last stored message selection. */
	enum AccountModel::NodeType
	    m_lastSelectedAccountNodeType; /*!< Last selected position. */
	enum AccountModel::NodeType
	    m_lastStoredAccountNodeType; /*!< Last stored account position. */

	bool m_searchDlgActive; /*!< True if search dialogue is active. */

	int m_received_1;
	int m_received_2;
	int m_sent_1;
	int m_sent_2;
	int m_sort_column;
	QString m_sort_order;
	QString m_save_attach_dir;
	QString m_add_attach_dir;
	QString m_export_correspond_dir;
	QString m_on_export_zfo_activate;
	QString m_on_import_database_dir_activate;
	QString m_import_zfo_path;
	QRect m_geometry; /* Non-maximised window geometry. */

	QList<DbMsgsTblModel::AppendedCol> m_msgTblAppendedCols; /*< Appended columns. */

	QSet<QString> m_zfoFilesToImport; /*!< Set of files to be imported. */
	int m_numFilesToImport; /*!< Input ZFO count. */
	/*!< QPair in following lists means:
	 * first string - zfo file name,
	 * second = import result text */
	QList< QPair<QString, QString> > m_importSucceeded; /*!< Success import resulty lists. */
	QList< QPair<QString, QString> > m_importExisted; /*!< Import exists resulty lists. */
	QList< QPair<QString, QString> > m_importFailed; /*!< Import error resulty lists. */

	/* User interface elements. */
	Ui::MainWindow *ui; /*!< User interface as generated from ui files. */
	QLineEdit *mui_filterLine; /*!< Search filter line object. */
	QPushButton *mui_clearFilterLineButton; /*!< Clear filter button. */
	QStatusBar *mui_statusBar; /*!< Status bar. */
	QLabel *mui_statusDbMode; /*!< Database status label. */
	QLabel *mui_statusOnlineLabel; /*< On-line/off-line status label. */
	QProgressBar *mui_statusProgressBar; /*!< Progress bar. */

	/*!
	 * @brief Performs initial user interface initialisation.
	 */
	void setUpUi(void);

	/*!
	 * @brief Adds actions to to tool bar.
	 */
	void topToolBarSetUp(void);

	/*!
	 * @brief Sets action icons.
	 */
	void setMenuActionIcons(void);
};


#endif /* _DATOVKA_H_ */



#include "src/gui/dlg_view_zfo.h"
#include "src/io/dbs.h"


/* ========================================================================= */
/*
 * Constructor.
 */
DlgViewZfo::DlgViewZfo(const isds_message *isdsMsg, QWidget *parent)
/* ========================================================================= */
    : QDialog(parent),
    m_message(isdsMsg)
{
	setupUi(this);

	Q_ASSERT(NULL != m_message);

	envelopeTextEdit->setHtml(descriptionHtml());
	envelopeTextEdit->setReadOnly(true);
}


/* ========================================================================= */
/*
 * Generate description from supplied message.
 */
QString DlgViewZfo::descriptionHtml(void)
/* ========================================================================= */
{
	const isds_envelope *envelope;
	isds_message_status *state;

	envelope = m_message->envelope;
	Q_ASSERT(NULL != envelope);

	state = envelope->dmMessageStatus;
	Q_ASSERT(NULL != state);

	QString html;

	html += indentDivStart;
	html += "<h3>" + tr("Identification") + "</h3>";

	html += strongAccountInfoLine(tr("ID"), envelope->dmID);
	html += strongAccountInfoLine(tr("Subject"), envelope->dmAnnotation);
	html += strongAccountInfoLine(tr("Message type"), envelope->dmType);

	html += "<br/>";

	/* Information about message author. */
	html += strongAccountInfoLine("From", envelope->dmSender);
	html += strongAccountInfoLine("Sender Address",
	    envelope->dmSenderAddress);

	html += "<br/>";

	html += strongAccountInfoLine(tr("To"), envelope->dmRecipient);
	html += strongAccountInfoLine(tr("Recipient Address"),
	    envelope->dmRecipientAddress);

	html += "<h3>" + tr("Status") + "</h3>";

	html += strongAccountInfoLine(tr("Delivery time"),
	    dateTimeStrFromDbFormat(
	        timevalToDbFormat(envelope->dmDeliveryTime),
	        dateTimeDisplayFormat));
	html += strongAccountInfoLine(tr("Acceptance time"),
	    dateTimeStrFromDbFormat(
	        timevalToDbFormat(envelope->dmAcceptanceTime),
	        dateTimeDisplayFormat));
	html += strongAccountInfoLine(tr("Status"),
	    QString::number(convertHexToDecIndex(
	        *(envelope->dmMessageStatus))) + " -- " +
	    msgStatusToText(convertHexToDecIndex(
	        *(envelope->dmMessageStatus))));
	html += strongAccountInfoLine(tr("Attachments"), "TODO");

	html += "<h3>" + tr("Signature") + "</h3>";

	html += strongAccountInfoLine(tr("Message signature"), "TODO");
	html += strongAccountInfoLine(tr("Signing certificate"), "TODO");
	html += strongAccountInfoLine(tr("Time-stamp"), "TODO");

	return html;
}

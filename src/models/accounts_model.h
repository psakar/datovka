

#ifndef _ACCOUNTS_MODEL_H_
#define _ACCOUNTS_MODEL_H_


#include <QStandardItemModel>


/*!
 * @brief Account hierarchy.
 */
class AccountModel: public QStandardItemModel {

public:
	AccountModel(void);

private:
	bool addAccount(const QString &accountName);
};


#endif /* _ACCOUNTS_MODEL_H_ */

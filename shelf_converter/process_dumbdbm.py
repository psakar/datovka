#!/usr/bin/env python


#from dslib import *
#from dslib.certs import cert_finder

import dumbdbm
import os
import sys
from shelve import Shelf
import sqlite3


class DumbShelf(Shelf):
    def __init__(self, filename, flag='c', protocol=None, writeback=False):
        Shelf.__init__(self, dumbdbm.open(filename, flag), protocol, writeback)

def open_shelf(filename, flag='c', protocol=None, writeback=False):
    return DumbShelf(filename, flag, protocol, writeback)


# Commandine arguments.
if len(sys.argv) != 2:
    sys.stderr.write("Usage:\n\n%s dsgui_direrctory\n" % (sys.argv[0]))
    sys.exit(1)


# Check file presence.
shelf_prefix = sys.argv[1] + "messages.shelf"
shelf_file = shelf_prefix + ".dat"
db_file = sys.argv[1] + "messages.shelf.db"
if not os.path.isfile(shelf_file):
    sys.stderr.write("File %s does not exist.\n" % (shelf_file))
    sys.exit(1)


#(key, python type, sqlite type, target python type)
account_info_structure = [
    ("key", str, "TEXT NOT NULL PRIMARY KEY", str),
    ("dbID", str, "TEXT NOT NULL", str),
    ("dbType", str, "TEXT", str),
    ("ic", int, "INTEGER", int),
    ("pnFirstName", str, "TEXT", str),
    ("pnMiddleName", str, "TEXT", str),
    ("pnLastName", str, "TEXT", str),
    ("pnLastNameAtBirth", str, "TEXT", str),
    ("firmName", str, "TEXT", str),
    ("biDate", str, "TEXT", str),
    ("biCity", str, "TEXT", str),
    ("biCounty", str, "TEXT", str),
    ("biState", str, "TEXT", str),
    ("adCity", str, "TEXT", str),
    ("adStreet", str, "TEXT", str),
    ("adNumberInStreet", str, "TEXT", str),
    ("adNumberInMunicipality", str, "TEXT", str),
    ("adZipCode", str, "TEXT", str),
    ("adState", str, "TEXT", str),
    ("nationality", str, "TEXT", str),
    ("identifier", str, "TEXT", str),
    ("registryCode", str, "TEXT", str),
    ("dbState", int, "INTEGER", int),
    ("dbEffectiveOVM", bool, "INTEGER", int),
    ("dbOpenAddressing", bool, "INTEGER", int)
]


def create_account_info(db):
    # Create account_info table.
    sql_cmd = "CREATE TABLE IF NOT EXISTS account_info ("

    for item in account_info_structure[:-1]:
        sql_cmd += item[0] + " " + item[2] + ", "
    item =  account_info_structure[-1]
    sql_cmd += item[0] + " " + item[2]
    sql_cmd += ")"
    #print sql_cmd

    db.execute(sql_cmd)
    db.commit()


def create_password_expiration_date(db):
    # Create password_expiration_date table.

    sql_cmd = "CREATE TABLE IF NOT EXISTS password_expiration_date ("
    sql_cmd += "key" + " " + "TEXT NOT NULL PRIMARY KEY" + ", "
    sql_cmd += "expDate" + " " + "TEXT"
    sql_cmd += ")"

    db.execute(sql_cmd)
    db.commit()


shelf = open_shelf(shelf_prefix, protocol=-1)
db = sqlite3.connect(db_file)
create_account_info(db)
create_password_expiration_date(db)

for key in shelf.keys():
    content = shelf.get(key)

    identifiers = []
    questionmarks = []
    values = []
    account_info = content.get("account_info")
    identifiers.append(account_info_structure[0][0])
    questionmarks.append("?")
    values.append(key)
    for item in account_info_structure[1:]:
        value = getattr(account_info, item[0])
        if value != None:
            identifiers.append(item[0])
            if item[1] != item[3]:
                value = item[3](value)
            if not (isinstance(value, str) or isinstance(value, unicode)):
                value = str(value)
            else:
                pass
            questionmarks.append("?")
            values.append(value)

    sql_cmd = "INSERT OR REPLACE INTO account_info ("
    sql_cmd += ", ".join(identifiers)
    sql_cmd += ") VALUES ("
    sql_cmd += ", ".join(questionmarks)
    sql_cmd += ")"
    #print sql_cmd
    db.execute(sql_cmd, values)
    db.commit()

    identifiers = []
    questionmarks = []
    values = []
    pwd_exp_date = content.get("password_expiration_date")
    identifiers.append("key")
    questionmarks.append("?")
    values.append(key)
    if pwd_exp_date != None:
        identifiers.append("expDate")
        questionmarks.append("?")
        values.append(str(pwd_exp_date))
    sql_cmd = "INSERT OR REPLACE INTO password_expiration_date ("
    sql_cmd += ", ".join(identifiers)
    sql_cmd += ") VALUES ("
    sql_cmd += ", ".join(questionmarks)
    sql_cmd += ")"
    #print sql_cmd
    db.execute(sql_cmd, values)
    db.commit()
    

shelf.close()

db.close()

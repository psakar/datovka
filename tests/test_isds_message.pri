
DEFINES += \
	TEST_ISDS_MESSAGE=1

INCLUDEPATH += \
	/usr/include/libxml2

LIBS += \
	-lisds \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/isds/internal_conversion.cpp \
	$${top_srcdir}src/isds/message_functions.cpp \
        $${top_srcdir}src/isds/message_interface.cpp \
	$${top_srcdir}tests/test_isds_message.cpp

HEADERS += \
	$${top_srcdir}src/isds/internal_conversion.h \
	$${top_srcdir}src/isds/message_functions.h \
        $${top_srcdir}src/isds/message_interface.h \
	$${top_srcdir}src/isds/types.h \
	$${top_srcdir}tests/test_isds_message.h


DEFINES += \
	TEST_VERSION=1

INCLUDEPATH += \
	/usr/include/libxml2

LIBS += \
	-lisds \
	-lcrypto

SOURCES += \
	$${top_srcdir}src/about.cpp \
	$${top_srcdir}tests/test_version.cpp

HEADERS += \
	$${top_srcdir}src/about.h \
	$${top_srcdir}tests/test_version.h

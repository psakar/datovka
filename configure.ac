
AC_PREREQ([2.65])
AC_INIT([qtest], 0.0)
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.10 gnu -Wall])

#sinclude(ax_have_qt.m4)

# Check for programs.
AC_PROG_CC
AM_PROG_CC_C_O # Per-target flags.
AC_PROG_CXX
AC_PROG_INSTALL

# Config header file prefix.
AH_TOP([#ifndef _CONFIG_H_])
AH_TOP([#define _CONFIG_H_])
# Config header file suffix.
AH_BOTTOM([#endif /* !_CONFIG_H_ */])

# Qt4 related stuff.
#AX_HAVE_QT
PKG_CHECK_MODULES(QT4, [QtCore QtGui >= 4.4.0], [
  AC_PATH_PROGS(MOC, [moc-qt4 moc], moc,`eval $PKG_CONFIG --variable=exec_prefix QtCore`/bin)
  AC_PATH_PROG(RCC, rcc, rcc,`eval $PKG_CONFIG --variable=exec_prefix QtCore`/bin)
  AC_PATH_PROGS(UIC, [uic-qt4 uic], uic,`eval $PKG_CONFIG --variable=exec_prefix QtCore`/bin)
], [
  AC_MSG_ERROR([Qt 4 library not found])
])

AC_CONFIG_FILES([
	Makefile \
	src/Makefile
])

AC_OUTPUT

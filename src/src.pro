TEMPLATE = lib
TARGET = usb-moded-qt$${QT_MAJOR_VERSION}
CONFIG += link_pkgconfig
PKGCONFIG += usb_moded

QT += dbus
QT -= gui

DEFINES += QUSBMODED_LIBRARY

SOURCES += \
    qusbmode.cpp \
    qusbmoded.cpp

PUBLIC_HEADERS += \
    qusbmode.h \
    qusbmoded.h \
    qusbmoded_types.h

HEADERS += \
  $$PUBLIC_HEADERS

USB_MODED_INCLUDE_PATH = $$system(for d in `pkg-config --cflags-only-I usb_moded` ; do echo $d ; done | grep usb.moded | sed s/^-I//g)
DBUS_INTERFACES += com_meego_usb_moded
com_meego_usb_moded.files = $$USB_MODED_INCLUDE_PATH/com.meego.usb_moded.xml
com_meego_usb_moded.header_flags = -N -c QUsbModedInterface
com_meego_usb_moded.source_flags = -N -c QUsbModedInterface

headers.path = $$INSTALL_ROOT$$PREFIX/include/usb-moded-qt$${QT_MAJOR_VERSION}
headers.files = $$PUBLIC_HEADERS
INSTALLS += headers

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

pkgconfig.files = $$TARGET.pc
pkgconfig.path = $$target.path/pkgconfig-qt$${QT_MAJOR_VERSION}
QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_VERSION = $$VERSION
QMAKE_PKGCONFIG_DESCRIPTION = Qt bindings for usb_moded
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
CONFIG += create_pc create_prl no_install_prl
INSTALLS += pkgconfig

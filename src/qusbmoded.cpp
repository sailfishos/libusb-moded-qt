/*
 * Copyright (C) 2015 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "qusbmoded.h"
#include "qusbmoded_debug.h"
#include "usb_moded_interface.h"

#include "usb_moded-dbus.h"
#include "usb_moded-modes.h"

#define USB_MODED_CALL_GET_MODES    (0x01)
#define USB_MODED_CALL_GET_CONFIG   (0x02)
#define USB_MODED_CALL_MODE_REQUEST (0x04)

class QUsbModed::Private
{
public:
    static const QString UsbModeSection;
    static const QString UsbModeKeyMode;

    QStringList iSupportedModes;
    QString iConfigMode;
    QString iCurrentMode;
    QDBusConnection iBus;
    QUsbModedInterface* iInterface;
    int iPendingCalls;
    bool iAvailable;

    Private() :
        iBus(QDBusConnection::systemBus()),
        iInterface(NULL),
        iPendingCalls(0),
        iAvailable(false) {}
};

// Groups and keys (usb_moded-config.h)
const QString QUsbModed::Private::UsbModeSection("usbmode");
const QString QUsbModed::Private::UsbModeKeyMode("mode");

// States (from usb_moded-dbus.h)
const QString QUsbModed::Mode::Connected(USB_CONNECTED);
const QString QUsbModed::Mode::DataInUse(DATA_IN_USE);
const QString QUsbModed::Mode::Disconnected(USB_DISCONNECTED);
const QString QUsbModed::Mode::ModeRequest(USB_CONNECTED_DIALOG_SHOW);

// Modes (from usb_moded-modes.h)
const QString QUsbModed::Mode::Undefined(MODE_UNDEFINED);
const QString QUsbModed::Mode::Ask(MODE_ASK);
const QString QUsbModed::Mode::MassStorage(MODE_MASS_STORAGE);
const QString QUsbModed::Mode::Developer(MODE_DEVELOPER);
const QString QUsbModed::Mode::MTP(MODE_MTP);
const QString QUsbModed::Mode::Host(MODE_HOST);
const QString QUsbModed::Mode::ConnectionSharing(MODE_CONNECTION_SHARING);
const QString QUsbModed::Mode::Diag(MODE_DIAG);
const QString QUsbModed::Mode::Adb(MODE_ADB);
const QString QUsbModed::Mode::PCSuite(MODE_PC_SUITE);
const QString QUsbModed::Mode::Charging(MODE_CHARGING);
const QString QUsbModed::Mode::Charger(MODE_CHARGER);

QUsbModed::QUsbModed(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private)
{
    QDBusServiceWatcher* serviceWatcher =
        new QDBusServiceWatcher(USB_MODE_SERVICE, iPrivate->iBus,
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);

    connect(serviceWatcher,
        SIGNAL(serviceRegistered(QString)),
        SLOT(onServiceRegistered(QString)));
    connect(serviceWatcher,
        SIGNAL(serviceUnregistered(QString)),
        SLOT(onServiceUnregistered(QString)));

    if (iPrivate->iBus.interface()->isServiceRegistered(USB_MODE_SERVICE)) {
        setup();
    }
}

QUsbModed::~QUsbModed()
{
    delete iPrivate;
}

QStringList QUsbModed::supportedModes() const
{
    return iPrivate->iSupportedModes;
}

bool QUsbModed::available() const
{
    return iPrivate->iAvailable;
}

QString QUsbModed::currentMode() const
{
    return iPrivate->iCurrentMode;
}

QString QUsbModed::configMode() const
{
    return iPrivate->iConfigMode;
}

bool QUsbModed::setCurrentMode(QString aMode)
{
    if (iPrivate->iInterface) {
        connect(new QDBusPendingCallWatcher(
            iPrivate->iInterface->set_mode(aMode), iPrivate->iInterface),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onSetModeFinished(QDBusPendingCallWatcher*)));
        return true;
    }
    return false;
}

bool QUsbModed::setConfigMode(QString aMode)
{
    if (iPrivate->iInterface) {
        connect(new QDBusPendingCallWatcher(
            iPrivate->iInterface->set_config(aMode), iPrivate->iInterface),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onSetConfigFinished(QDBusPendingCallWatcher*)));
        return true;
    }
    return false;
}

void QUsbModed::onServiceRegistered(QString aService)
{
    DEBUG_(aService);
    setup();
}

void QUsbModed::onServiceUnregistered(QString aService)
{
    DEBUG_(aService);
    iPrivate->iPendingCalls = 0;
    if (iPrivate->iInterface) {
        delete iPrivate->iInterface;
        iPrivate->iInterface = NULL;
    }
    if (iPrivate->iAvailable) {
        iPrivate->iAvailable = false;
        Q_EMIT availableChanged();
    }
}

void QUsbModed::setup()
{
    delete iPrivate->iInterface; // That cancels whatever is in progress
    iPrivate->iInterface = new QUsbModedInterface(USB_MODE_SERVICE,
        USB_MODE_OBJECT, iPrivate->iBus, this);
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_state_ind(QString)),
        SLOT(onUsbStateChanged(QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_config_ind(QString,QString,QString)),
        SLOT(onUsbConfigChanged(QString,QString,QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_supported_modes_ind(QString)),
        SLOT(onUsbSupportedModesChanged(QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_state_error_ind(QString)),
        SIGNAL(usbStateError(QString)));

    // Request the current state
    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_MODES;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->get_modes(), iPrivate->iInterface),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetModesFinished(QDBusPendingCallWatcher*)));

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_CONFIG;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->get_config(), iPrivate->iInterface),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetConfigFinished(QDBusPendingCallWatcher*)));

    iPrivate->iPendingCalls |= USB_MODED_CALL_MODE_REQUEST;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->mode_request(), iPrivate->iInterface),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetModeRequestFinished(QDBusPendingCallWatcher*)));
}

void QUsbModed::onGetModesFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    QString modes;
    if (!reply.isError()) {
        modes = reply.value();
        DEBUG_(modes);
    } else {
        DEBUG_(reply.error());
    }
    updateSupportedModes(modes);
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_GET_MODES);
}

void QUsbModed::onGetConfigFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        DEBUG_(mode);
        if (iPrivate->iConfigMode != mode) {
            iPrivate->iConfigMode = mode;
            Q_EMIT configModeChanged();
        }
    } else {
        DEBUG_(reply.error());
    }
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_GET_CONFIG);
}

void QUsbModed::onGetModeRequestFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        DEBUG_(mode);
        if (iPrivate->iCurrentMode != mode) {
            iPrivate->iCurrentMode = mode;
            Q_EMIT currentModeChanged();
        }
        if (iPrivate->iCurrentMode != mode) {
            iPrivate->iCurrentMode = mode;
            Q_EMIT currentModeChanged();
        }
    } else {
        DEBUG_(reply.error());
    }
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_MODE_REQUEST);
}

void QUsbModed::updateSupportedModes(QString aModes)
{
    const QStringList result = aModes.split(',', QString::SkipEmptyParts);
    const int n = result.count();
    QStringList modes;
    for (int i=0; i<n; i++) {
        QString mode(result.at(i).trimmed());
        if (!modes.contains(mode)) modes.append(mode);
    }
    if (iPrivate->iSupportedModes != modes) {
        iPrivate->iSupportedModes = modes;
        Q_EMIT supportedModesChanged();
    }
}

void QUsbModed::setupCallFinished(int aCallId)
{
    ASSERT_(iPrivate->iPendingCalls & aCallId);
    iPrivate->iPendingCalls &= ~aCallId;
    if (!iPrivate->iPendingCalls) {
        DEBUG_("setup done");
        ASSERT_(!iPrivate->iAvailable);
        iPrivate->iAvailable = true;
        Q_EMIT availableChanged();
    }
}

void QUsbModed::onSetModeFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        DEBUG_(mode);
        if (iPrivate->iCurrentMode != mode) {
            iPrivate->iCurrentMode = mode;
            Q_EMIT currentModeChanged();
        }
    } else {
        DEBUG_(reply.error());
    }
    aCall->deleteLater();
}

void QUsbModed::onSetConfigFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        DEBUG_(mode);
        if (iPrivate->iConfigMode != mode) {
            iPrivate->iConfigMode = mode;
            Q_EMIT configModeChanged();
        }
    } else {
        DEBUG_(reply.error());
    }
    aCall->deleteLater();
}

void QUsbModed::onUsbStateChanged(QString aMode)
{
    DEBUG_(aMode);
    if (iPrivate->iCurrentMode != aMode) {
        iPrivate->iCurrentMode = aMode;
        Q_EMIT currentModeChanged();
    }
}

void QUsbModed::onUsbSupportedModesChanged(QString aModes)
{
    DEBUG_(aModes);
    updateSupportedModes(aModes);
}

void QUsbModed::onUsbConfigChanged(QString aSect, QString aKey, QString aVal)
{
    DEBUG_(aSect << aKey << aVal);
    if (aSect == Private::UsbModeSection &&
        aKey == Private::UsbModeKeyMode) {
        if (iPrivate->iConfigMode != aVal) {
            iPrivate->iConfigMode = aVal;
            Q_EMIT configModeChanged();
        }
    }
}

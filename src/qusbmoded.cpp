/*
 * Copyright (C) 2015-2019 Jolla Ltd.
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

#define USB_MODED_CALL_GET_MODES    (0x01)
#define USB_MODED_CALL_GET_CONFIG   (0x02)
#define USB_MODED_CALL_MODE_REQUEST (0x04)
#define USB_MODED_CALL_GET_HIDDEN   (0x08)
#define USB_MODED_CALL_GET_AVAILABLE_MODES (0x10)
#define USB_MODED_CALL_GET_TARGET_MODE (0x20)

class QUsbModed::Private
{
public:
    static const QString UsbModeSection;
    static const QString UsbModeKeyMode;

    QStringList iSupportedModes;
    QStringList iAvailableModes;
    QStringList iHiddenModes;
    QString iConfigMode;
    QString iCurrentMode;
    QString iTargetMode;
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

QUsbModed::QUsbModed(QObject* aParent) :
    QUsbMode(aParent),
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

QStringList QUsbModed::availableModes() const
{
    return iPrivate->iAvailableModes;
}

QStringList QUsbModed::hiddenModes() const
{
    return iPrivate->iHiddenModes;
}

bool QUsbModed::available() const
{
    return iPrivate->iAvailable;
}

QString QUsbModed::currentMode() const
{
    return iPrivate->iCurrentMode;
}

QString QUsbModed::targetMode() const
{
    return iPrivate->iTargetMode;
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

bool QUsbModed::hideMode(QString mode)
{
    if (iPrivate->iInterface) {
        connect(new QDBusPendingCallWatcher(
            iPrivate->iInterface->hide_mode(mode), iPrivate->iInterface),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onHideModeFinished(QDBusPendingCallWatcher*)));
        return true;
    }
    return false;
}

bool QUsbModed::unhideMode(QString mode)
{
    if (iPrivate->iInterface) {
        connect(new QDBusPendingCallWatcher(
            iPrivate->iInterface->unhide_mode(mode), iPrivate->iInterface),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onUnhideModeFinished(QDBusPendingCallWatcher*)));
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
        SIGNAL(sig_usb_target_state_ind(QString)),
        SLOT(onUsbTargetStateChanged(QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_current_state_ind(QString)),
        SLOT(onUsbStateChanged(QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_event_ind(QString)),
        SLOT(onUsbEventReceived(QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_config_ind(QString,QString,QString)),
        SLOT(onUsbConfigChanged(QString,QString,QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_supported_modes_ind(QString)),
        SLOT(onUsbSupportedModesChanged(QString)));
    connect(iPrivate->iInterface,
            &QUsbModedInterface::sig_usb_available_modes_ind,
            this,
            &QUsbModed::updateAvailableModes);
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_hidden_modes_ind(QString)),
        SLOT(onUsbHiddenModesChanged(QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_state_error_ind(QString)),
        SIGNAL(usbStateError(QString)));

    // Request the current state
    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_MODES;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->get_modes(), iPrivate->iInterface),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetModesFinished(QDBusPendingCallWatcher*)));

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_AVAILABLE_MODES;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->get_available_modes(), iPrivate->iInterface),
        &QDBusPendingCallWatcher::finished,
        this,
        &QUsbModed::onGetAvailableModesFinished);

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_CONFIG;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->get_config(), iPrivate->iInterface),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetConfigFinished(QDBusPendingCallWatcher*)));

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_TARGET_MODE;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->get_target_state(), iPrivate->iInterface),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetTargetModeFinished(QDBusPendingCallWatcher*)));

    iPrivate->iPendingCalls |= USB_MODED_CALL_MODE_REQUEST;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->mode_request(), iPrivate->iInterface),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetModeRequestFinished(QDBusPendingCallWatcher*)));

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_HIDDEN;
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->get_hidden(), iPrivate->iInterface),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetHiddenFinished(QDBusPendingCallWatcher*)));
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

void QUsbModed::onGetAvailableModesFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    QString modes;
    if (!reply.isError()) {
        modes = reply.value();
        DEBUG_(modes);
    } else {
        DEBUG_(reply.error());
    }
    updateAvailableModes(modes);
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_GET_AVAILABLE_MODES);
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
    } else {
        DEBUG_(reply.error());
    }
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_MODE_REQUEST);
}

void QUsbModed::onGetTargetModeFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        DEBUG_(mode);
        if (iPrivate->iTargetMode != mode) {
            iPrivate->iTargetMode = mode;
            Q_EMIT targetModeChanged();
        }
    } else {
        DEBUG_(reply.error());
    }
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_GET_TARGET_MODE);
}

void QUsbModed::onGetHiddenFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    QString modes;
    if (!reply.isError()) {
        modes = reply.value();
        DEBUG_(modes);
    } else {
        DEBUG_(reply.error());
    }
    updateHiddenModes(modes);
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_GET_HIDDEN);
}

void QUsbModed::updateHiddenModes(QString aModes)
{
    const QStringList result = aModes.split(',', QString::SkipEmptyParts);
    const int n = result.count();
    QStringList modes;
    for (int i=0; i<n; i++) {
        QString mode(result.at(i).trimmed());
        if (!modes.contains(mode)) modes.append(mode);
    }
    if (iPrivate->iHiddenModes != modes) {
        iPrivate->iHiddenModes = modes;
        Q_EMIT hiddenModesChanged();
    }
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

void QUsbModed::updateAvailableModes(const QString &aModes)
{
    const QStringList result = aModes.split(',', QString::SkipEmptyParts);
    const int n = result.count();
    QStringList modes;
    for (int i=0; i<n; i++) {
        QString mode(result.at(i).trimmed());
        if (!modes.contains(mode)) modes.append(mode);
    }
    if (iPrivate->iAvailableModes != modes) {
        iPrivate->iAvailableModes = modes;
        Q_EMIT availableModesChanged();
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
        // Note: Getting a reply does not indicate mode change.
        //       Even accepted requests could get translated to
        //       something else (e.g. charging only) if there
        //       are problems during mode activation
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

void QUsbModed::onHideModeFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (reply.isError()) {
        DEBUG_(reply.error());
        Q_EMIT hideModeFailed(reply.error().message());
    }
    aCall->deleteLater();
}

void QUsbModed::onUnhideModeFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (reply.isError()) {
        DEBUG_(reply.error());
        Q_EMIT unhideModeFailed(reply.error().message());
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

void QUsbModed::onUsbEventReceived(QString aEvent)
{
    DEBUG_(aEvent);
    Q_EMIT eventReceived(aEvent);
}

void QUsbModed::onUsbTargetStateChanged(QString aMode)
{
    DEBUG_(aMode);
    if (iPrivate->iTargetMode != aMode) {
        iPrivate->iTargetMode = aMode;
        Q_EMIT targetModeChanged();
    }
}

void QUsbModed::onUsbSupportedModesChanged(QString aModes)
{
    DEBUG_(aModes);
    updateSupportedModes(aModes);
}

void QUsbModed::onUsbHiddenModesChanged(QString modes)
{
    DEBUG_(modes);
    updateHiddenModes(modes);
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

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
#include "usb_moded_interface.h"

#include "usb_moded-dbus.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcQusb, "qusbmoded", QtWarningMsg)

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
    QUsbModedInterface* iInterface;
    int iPendingCalls;
    bool iAvailable;

    Private() :
        iInterface(nullptr),
        iPendingCalls(0),
        iAvailable(false) {}
};

// Groups and keys (usb_moded-config.h)
const QString QUsbModed::Private::UsbModeSection("usbmode");
const QString QUsbModed::Private::UsbModeKeyMode("mode");

QUsbModed::QUsbModed(QObject* aParent)
    : QUsbMode(aParent)
    , iPrivate(new Private)
{
    QDBusServiceWatcher* serviceWatcher =
        new QDBusServiceWatcher(USB_MODE_SERVICE, QDBusConnection::systemBus(),
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);

    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &QUsbModed::onServiceRegistered);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &QUsbModed::onServiceUnregistered);

    if (QDBusConnection::systemBus().interface()->isServiceRegistered(USB_MODE_SERVICE)) {
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
        auto *pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->set_mode(aMode), this);

        connect(pendingCall, &QDBusPendingCallWatcher::finished,
                this, &QUsbModed::onSetModeFinished);
        return true;
    }
    return false;
}

bool QUsbModed::setConfigMode(QString aMode)
{
    if (iPrivate->iInterface) {
        auto *pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->set_config(aMode), this);
        connect(pendingCall, &QDBusPendingCallWatcher::finished,
                this, &QUsbModed::onSetConfigFinished);
        return true;
    }
    return false;
}

bool QUsbModed::hideMode(QString mode)
{
    if (iPrivate->iInterface) {
        auto *pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->hide_mode(mode), this);
        connect(pendingCall, &QDBusPendingCallWatcher::finished,
                this, &QUsbModed::onHideModeFinished);
        return true;
    }
    return false;
}

bool QUsbModed::unhideMode(QString mode)
{
    if (iPrivate->iInterface) {
        auto *pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->unhide_mode(mode), this);
        connect(pendingCall, &QDBusPendingCallWatcher::finished,
                this, &QUsbModed::onUnhideModeFinished);
        return true;
    }
    return false;
}

void QUsbModed::onServiceRegistered(QString aService)
{
    qCDebug(lcQusb) << aService;
    setup();
}

void QUsbModed::onServiceUnregistered(QString aService)
{
    qCDebug(lcQusb) << aService;
    iPrivate->iPendingCalls = 0;

    delete iPrivate->iInterface;
    iPrivate->iInterface = nullptr;

    if (iPrivate->iAvailable) {
        iPrivate->iAvailable = false;
        Q_EMIT availableChanged();
    }
}

void QUsbModed::setup()
{
    delete iPrivate->iInterface; // That cancels whatever is in progress

    iPrivate->iInterface = new QUsbModedInterface(USB_MODE_SERVICE,
        USB_MODE_OBJECT, QDBusConnection::systemBus(), this);

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
            &QUsbModed::checkAvailableModesForUser);
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_hidden_modes_ind(QString)),
        SLOT(onUsbHiddenModesChanged(QString)));
    connect(iPrivate->iInterface,
        SIGNAL(sig_usb_state_error_ind(QString)),
        SIGNAL(usbStateError(QString)));

    // Request the current state
    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_MODES;
    auto *pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->get_modes(), this);
    connect(pendingCall, &QDBusPendingCallWatcher::finished,
            this, &QUsbModed::onGetModesFinished);

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_AVAILABLE_MODES;
    pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->get_available_modes_for_user(), this);
    connect(pendingCall, &QDBusPendingCallWatcher::finished,
            this, &QUsbModed::onGetAvailableModesFinished);

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_CONFIG;
    pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->get_config(), this);
    connect(pendingCall, &QDBusPendingCallWatcher::finished,
            this, &QUsbModed::onGetConfigFinished);

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_TARGET_MODE;
    pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->get_target_state(), this);
    connect(pendingCall, &QDBusPendingCallWatcher::finished,
            this, &QUsbModed::onGetTargetModeFinished);

    iPrivate->iPendingCalls |= USB_MODED_CALL_MODE_REQUEST;
    pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->mode_request(), this);
    connect(pendingCall, &QDBusPendingCallWatcher::finished,
            this, &QUsbModed::onGetModeRequestFinished);

    iPrivate->iPendingCalls |= USB_MODED_CALL_GET_HIDDEN;
    pendingCall = new QDBusPendingCallWatcher(iPrivate->iInterface->get_hidden(), this);
    connect(pendingCall, &QDBusPendingCallWatcher::finished,
            this, &QUsbModed::onGetHiddenFinished);
}

void QUsbModed::onGetModesFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    QString modes;
    if (!reply.isError()) {
        modes = reply.value();
        qCDebug(lcQusb) << modes;
    } else {
        qCDebug(lcQusb) << reply.error();
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
        qCDebug(lcQusb) << modes;
    } else {
        qCDebug(lcQusb) << reply.error();
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
        qCDebug(lcQusb) << mode;
        if (iPrivate->iConfigMode != mode) {
            iPrivate->iConfigMode = mode;
            Q_EMIT configModeChanged();
        }
    } else {
        qCDebug(lcQusb) << reply.error();
    }
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_GET_CONFIG);
}

void QUsbModed::onGetModeRequestFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        qCDebug(lcQusb) << mode;
        if (iPrivate->iCurrentMode != mode) {
            iPrivate->iCurrentMode = mode;
            Q_EMIT currentModeChanged();
        }
    } else {
        qCDebug(lcQusb) << reply.error();
    }
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_MODE_REQUEST);
}

void QUsbModed::onGetTargetModeFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        qCDebug(lcQusb) << mode;
        if (iPrivate->iTargetMode != mode) {
            iPrivate->iTargetMode = mode;
            Q_EMIT targetModeChanged();
        }
    } else {
        qCDebug(lcQusb) << reply.error();
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
        qCDebug(lcQusb) << modes;
    } else {
        qCDebug(lcQusb) << reply.error();
    }
    updateHiddenModes(modes);
    aCall->deleteLater();
    setupCallFinished(USB_MODED_CALL_GET_HIDDEN);
}

void QUsbModed::updateHiddenModes(QString aModes)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const QStringList result = aModes.split(',', QString::SkipEmptyParts);
#else
    const QStringList result = aModes.split(',', Qt::SkipEmptyParts);
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const QStringList result = aModes.split(',', QString::SkipEmptyParts);
#else
    const QStringList result = aModes.split(',', Qt::SkipEmptyParts);
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const QStringList result = aModes.split(',', QString::SkipEmptyParts);
#else
    const QStringList result = aModes.split(',', Qt::SkipEmptyParts);
#endif
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

void QUsbModed::checkAvailableModesForUser()
{
    connect(new QDBusPendingCallWatcher(
        iPrivate->iInterface->get_available_modes_for_user(), iPrivate->iInterface),
        &QDBusPendingCallWatcher::finished,
        this,
        &QUsbModed::onGetAvailableModesFinished);
}

void QUsbModed::setupCallFinished(int aCallId)
{
    Q_ASSERT(iPrivate->iPendingCalls & aCallId);

    iPrivate->iPendingCalls &= ~aCallId;

    if (!iPrivate->iPendingCalls) {
        qCDebug(lcQusb) << "setup done";
        Q_ASSERT(!iPrivate->iAvailable);
        iPrivate->iAvailable = true;
        Q_EMIT availableChanged();
    }
}

void QUsbModed::onSetModeFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        qCDebug(lcQusb) << mode;
        // Note: Getting a reply does not indicate mode change.
        //       Even accepted requests could get translated to
        //       something else (e.g. charging only) if there
        //       are problems during mode activation
    } else {
        qCDebug(lcQusb) << reply.error();
    }
    aCall->deleteLater();
}

void QUsbModed::onSetConfigFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (!reply.isError()) {
        QString mode = reply.value();
        qCDebug(lcQusb) << mode;
        if (iPrivate->iConfigMode != mode) {
            iPrivate->iConfigMode = mode;
            Q_EMIT configModeChanged();
        }
    } else {
        qCDebug(lcQusb) << reply.error();
    }
    aCall->deleteLater();
}

void QUsbModed::onHideModeFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (reply.isError()) {
        qCDebug(lcQusb) << reply.error();
        Q_EMIT hideModeFailed(reply.error().message());
    }
    aCall->deleteLater();
}

void QUsbModed::onUnhideModeFinished(QDBusPendingCallWatcher* aCall)
{
    QDBusPendingReply<QString> reply(*aCall);
    if (reply.isError()) {
        qCDebug(lcQusb) << reply.error();
        Q_EMIT unhideModeFailed(reply.error().message());
    }
    aCall->deleteLater();
}

void QUsbModed::onUsbStateChanged(QString aMode)
{
    qCDebug(lcQusb) << aMode;
    if (iPrivate->iCurrentMode != aMode) {
        iPrivate->iCurrentMode = aMode;
        Q_EMIT currentModeChanged();
    }
}

void QUsbModed::onUsbEventReceived(QString aEvent)
{
    qCDebug(lcQusb) << aEvent;
    Q_EMIT eventReceived(aEvent);
}

void QUsbModed::onUsbTargetStateChanged(QString aMode)
{
    qCDebug(lcQusb) << aMode;
    if (iPrivate->iTargetMode != aMode) {
        iPrivate->iTargetMode = aMode;
        Q_EMIT targetModeChanged();
    }
}

void QUsbModed::onUsbSupportedModesChanged(QString aModes)
{
    qCDebug(lcQusb) << aModes;
    updateSupportedModes(aModes);
}

void QUsbModed::onUsbHiddenModesChanged(QString modes)
{
    qCDebug(lcQusb) << modes;
    updateHiddenModes(modes);
}

void QUsbModed::onUsbConfigChanged(QString aSect, QString aKey, QString aVal)
{
    qCDebug(lcQusb) << aSect << aKey << aVal;
    if (aSect == Private::UsbModeSection &&
        aKey == Private::UsbModeKeyMode) {
        if (iPrivate->iConfigMode != aVal) {
            iPrivate->iConfigMode = aVal;
            Q_EMIT configModeChanged();
        }
    }
}

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

#ifndef QUSBMODED_H
#define QUSBMODED_H

#include "qusbmode.h"

#include <QStringList>

class QDBusPendingCallWatcher;

class QUSBMODED_EXPORT QUsbModed : public QUsbMode
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QStringList supportedModes READ supportedModes NOTIFY supportedModesChanged)
    Q_PROPERTY(QStringList availableModes READ availableModes NOTIFY availableModesChanged)
    Q_PROPERTY(QStringList hiddenModes READ hiddenModes NOTIFY hiddenModesChanged)
    Q_PROPERTY(QString currentMode READ currentMode WRITE setCurrentMode NOTIFY currentModeChanged)
    Q_PROPERTY(QString configMode READ configMode WRITE setConfigMode NOTIFY configModeChanged)

public:
    explicit QUsbModed(QObject* parent = NULL);
    ~QUsbModed();

    bool available() const;
    QStringList supportedModes() const;
    QStringList availableModes() const;
    QString currentMode() const;
    QString configMode() const;

    bool setCurrentMode(QString mode);
    bool setConfigMode(QString mode);

    QStringList hiddenModes() const;

public Q_SLOTS:
    bool hideMode(QString mode);
    bool unhideMode(QString mode);

Q_SIGNALS:
    void availableChanged();
    void supportedModesChanged();
    void availableModesChanged();
    void currentModeChanged();
    void configModeChanged();
    void usbStateError(QString error);
    void hiddenModesChanged();
    void hideModeFailed(QString mode);
    void unhideModeFailed(QString mode);

private Q_SLOTS:
    void onServiceRegistered(QString service);
    void onServiceUnregistered(QString service);
    void onGetModesFinished(QDBusPendingCallWatcher* call);
    void onGetAvailableModesFinished(QDBusPendingCallWatcher *call);
    void onGetConfigFinished(QDBusPendingCallWatcher* call);
    void onGetModeRequestFinished(QDBusPendingCallWatcher* call);
    void onSetModeFinished(QDBusPendingCallWatcher* call);
    void onSetConfigFinished(QDBusPendingCallWatcher* call);
    void onHideModeFinished(QDBusPendingCallWatcher* call);
    void onUnhideModeFinished(QDBusPendingCallWatcher* call);
    void onGetHiddenFinished(QDBusPendingCallWatcher* call);
    void onUsbConfigChanged(QString section, QString key, QString value);
    void onUsbStateChanged(QString mode);
    void onUsbSupportedModesChanged(QString modes);
    void onUsbHiddenModesChanged(QString modes);

private:
    void setup();
    void setupCallFinished(int callId);
    void updateSupportedModes(QString modes);
    void updateAvailableModes(const QString &modes);
    void updateHiddenModes(QString modes);

private:
    class Private;
    Private* iPrivate;
};

#endif // QUSBMODED_H

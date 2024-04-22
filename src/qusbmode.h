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

#ifndef QUSBMODE_H
#define QUSBMODE_H

#include "qusbmoded_types.h"

#include <QObject>

class QUSBMODED_EXPORT QUsbMode : public QObject
{
    Q_OBJECT

    // Transient Modes / "Events" (from usb_moded-dbus.h)
    Q_PROPERTY(QString USB_CONNECTED READ USB_CONNECTED CONSTANT)
    Q_PROPERTY(QString DATA_IN_USE READ DATA_IN_USE CONSTANT)
    Q_PROPERTY(QString USB_DISCONNECTED READ USB_DISCONNECTED CONSTANT)
    Q_PROPERTY(QString USB_CONNECTED_DIALOG_SHOW READ USB_CONNECTED_DIALOG_SHOW CONSTANT)
    Q_PROPERTY(QString USB_PRE_UNMOUNT READ USB_PRE_UNMOUNT CONSTANT)
    Q_PROPERTY(QString RE_MOUNT_FAILED READ RE_MOUNT_FAILED CONSTANT)
    Q_PROPERTY(QString MODE_SETTING_FAILED READ MODE_SETTING_FAILED CONSTANT)
    Q_PROPERTY(QString CHARGER_CONNECTED READ CHARGER_CONNECTED CONSTANT)
    Q_PROPERTY(QString CHARGER_DISCONNECTED READ CHARGER_DISCONNECTED CONSTANT)

    // Persistent Modes / "States" (from usb_moded-modes.h)
    Q_PROPERTY(QString MODE_UNDEFINED READ MODE_UNDEFINED CONSTANT)
    Q_PROPERTY(QString MODE_ASK READ MODE_ASK CONSTANT)
    Q_PROPERTY(QString MODE_MASS_STORAGE READ MODE_MASS_STORAGE CONSTANT)
    Q_PROPERTY(QString MODE_DEVELOPER READ MODE_DEVELOPER CONSTANT)
    Q_PROPERTY(QString MODE_MTP READ MODE_MTP CONSTANT)
    Q_PROPERTY(QString MODE_HOST READ MODE_HOST CONSTANT)
    Q_PROPERTY(QString MODE_CONNECTION_SHARING READ MODE_CONNECTION_SHARING CONSTANT)
    Q_PROPERTY(QString MODE_DIAG READ MODE_DIAG CONSTANT)
    Q_PROPERTY(QString MODE_ADB READ MODE_ADB CONSTANT)
    Q_PROPERTY(QString MODE_PC_SUITE READ MODE_PC_SUITE CONSTANT)
    Q_PROPERTY(QString MODE_CHARGING READ MODE_CHARGING CONSTANT)
    Q_PROPERTY(QString MODE_CHARGER READ MODE_CHARGER CONSTANT)
    Q_PROPERTY(QString MODE_CHARGING_FALLBACK READ MODE_CHARGING_FALLBACK CONSTANT)
    Q_PROPERTY(QString MODE_BUSY READ MODE_BUSY CONSTANT)

public:
    class Mode {
    public:
        // Transient Modes / "Events" (from usb_moded-dbus.h)
        static const QString Connected;
        static const QString DataInUse;
        static const QString Disconnected;
        static const QString ModeRequest;
        static const QString PreUnmount;
        static const QString ReMountFailed;
        static const QString ModeSettingFailed;
        static const QString ChargerConnected;
        static const QString ChargerDisconnected;

        // Persistent Modes / "States" (from usb_moded-modes.h)
        static const QString Undefined;
        static const QString Ask;
        static const QString MassStorage;
        static const QString Developer;
        static const QString MTP;
        static const QString Host;
        static const QString ConnectionSharing;
        static const QString Diag;
        static const QString Adb;
        static const QString PCSuite;
        static const QString Charging;
        static const QString Charger;
        static const QString ChargingFallback;
        static const QString Busy;

    private:
        Mode(); // Disallow instantiation
    };

    QUsbMode(QObject* parent = nullptr);

    Q_INVOKABLE static bool isEvent(const QString &modeName);
    Q_INVOKABLE static bool isState(const QString &modeName);
    Q_INVOKABLE static bool isWaitingState(const QString &modeName);
    Q_INVOKABLE static bool isFinalState(const QString &modeName);
    Q_INVOKABLE static bool isConnected(const QString &modeName);
    Q_INVOKABLE static bool isDisconnected(const QString &modeName);

private:
    // Getters for QML constants
    QString USB_CONNECTED() const { return Mode::Connected; }
    QString DATA_IN_USE() const { return Mode::DataInUse; }
    QString USB_DISCONNECTED() const { return Mode::Disconnected; }
    QString USB_CONNECTED_DIALOG_SHOW() const { return Mode::ModeRequest; }
    QString USB_PRE_UNMOUNT() const { return Mode::PreUnmount; }
    QString RE_MOUNT_FAILED() const { return Mode::ReMountFailed; }
    QString MODE_SETTING_FAILED() const { return Mode::ModeSettingFailed; }
    QString CHARGER_CONNECTED() const { return Mode::ChargerConnected; }
    QString CHARGER_DISCONNECTED() const { return Mode::ChargerDisconnected; }

    QString MODE_UNDEFINED() const { return Mode::Undefined; }
    QString MODE_ASK() const { return Mode::Ask; }
    QString MODE_MASS_STORAGE() const { return Mode::MassStorage; }
    QString MODE_DEVELOPER() const { return Mode::Developer; }
    QString MODE_MTP() const { return Mode::MTP; }
    QString MODE_HOST() const { return Mode::Host; }
    QString MODE_CONNECTION_SHARING() const { return Mode::ConnectionSharing; }
    QString MODE_DIAG() const { return Mode::Diag; }
    QString MODE_ADB() const { return Mode::Adb; }
    QString MODE_PC_SUITE() const { return Mode::PCSuite; }
    QString MODE_CHARGING() const { return Mode::Charging; }
    QString MODE_CHARGER() const { return Mode::Charger; }
    QString MODE_CHARGING_FALLBACK() const { return Mode::ChargingFallback; }
    QString MODE_BUSY() const { return Mode::Busy; }
};

#endif // QUSBMODED_H

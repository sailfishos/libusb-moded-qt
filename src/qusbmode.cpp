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

#include "qusbmode.h"

#include "usb_moded-dbus.h"
#include "usb_moded-modes.h"

// States (from usb_moded-dbus.h)
const QString QUsbMode::Mode::Connected(USB_CONNECTED);
const QString QUsbMode::Mode::DataInUse(DATA_IN_USE);
const QString QUsbMode::Mode::Disconnected(USB_DISCONNECTED);
const QString QUsbMode::Mode::ModeRequest(USB_CONNECTED_DIALOG_SHOW);
const QString QUsbMode::Mode::PreUnmount(USB_PRE_UNMOUNT);
const QString QUsbMode::Mode::ReMountFailed(RE_MOUNT_FAILED);
const QString QUsbMode::Mode::ModeSettingFailed(MODE_SETTING_FAILED);
const QString QUsbMode::Mode::ChargerConnected(CHARGER_CONNECTED);
const QString QUsbMode::Mode::ChargerDisconnected(CHARGER_DISCONNECTED);

// Modes (from usb_moded-modes.h)
const QString QUsbMode::Mode::Undefined(MODE_UNDEFINED);
const QString QUsbMode::Mode::Ask(MODE_ASK);
const QString QUsbMode::Mode::MassStorage(MODE_MASS_STORAGE);
const QString QUsbMode::Mode::Developer(MODE_DEVELOPER);
const QString QUsbMode::Mode::MTP(MODE_MTP);
const QString QUsbMode::Mode::Host(MODE_HOST);
const QString QUsbMode::Mode::ConnectionSharing(MODE_CONNECTION_SHARING);
const QString QUsbMode::Mode::Diag(MODE_DIAG);
const QString QUsbMode::Mode::Adb(MODE_ADB);
const QString QUsbMode::Mode::PCSuite(MODE_PC_SUITE);
const QString QUsbMode::Mode::Charging(MODE_CHARGING);
const QString QUsbMode::Mode::Charger(MODE_CHARGER);
const QString QUsbMode::Mode::ChargingFallback(MODE_CHARGING_FALLBACK);
const QString QUsbMode::Mode::Busy(MODE_BUSY);

QUsbMode::QUsbMode(QObject* aParent) :
    QObject(aParent)
{
}

bool QUsbMode::isEvent(const QString &modeName)
{
    // "Event" is something usb-moded can broadcast as
    //   com.meego.usb_moded.sig_usb_state_ind(modeName)
    // but is never returned as result of mode query
    //   modeName = com.meego.usb_moded.mode_request()

    // The set of possible "events" is hard-coded in usb-moded and
    // can be assumed to be fairly stable
    return (modeName == QUsbMode::Mode::Connected ||
            modeName == QUsbMode::Mode::DataInUse ||
            modeName == QUsbMode::Mode::Disconnected ||
            modeName == QUsbMode::Mode::ModeRequest ||
            modeName == QUsbMode::Mode::PreUnmount ||
            modeName == QUsbMode::Mode::ReMountFailed ||
            modeName == QUsbMode::Mode::ModeSettingFailed ||
            modeName == QUsbMode::Mode::ChargerConnected ||
            modeName == QUsbMode::Mode::ChargerDisconnected);
}

bool QUsbMode::isState(const QString &modeName)
{
    // "State" is something usb-moded can broadcast as
    //   com.meego.usb_moded.sig_usb_state_ind(modeName)
    // and can be returned as result of mode query
    //   modeName = com.meego.usb_moded.mode_request()

    // The set of "states" depends on configuration files and
    // thus the only assumption that can be made is: If it is
    // not an "event", it is a "state".
    return !isEvent(modeName);
}

bool QUsbMode::isWaitingState(const QString &modeName)
{
    // Busy -> Waiting for usb reconfiguration etc tasks related
    //         to mode switch to finish.

    // ChargingFallback -> Waiting for device state that allows
    // mode selection e.g. device to get unlocked.

    // Ask -> Waiting for user to select a mode.

    return (modeName == QUsbMode::Mode::Busy ||
            modeName == QUsbMode::Mode::ChargingFallback ||
            modeName == QUsbMode::Mode::Ask);
}

bool QUsbMode::isFinalState(const QString &modeName)
{
    return isState(modeName) && !isWaitingState(modeName);
}

bool QUsbMode::isDisconnected(const QString &modeName)
{
    return (modeName == QUsbMode::Mode::Disconnected ||
            modeName == QUsbMode::Mode::ChargerDisconnected ||
            modeName == QUsbMode::Mode::Undefined);
}

bool QUsbMode::isConnected(const QString &modeName)
{
    // Note that "busy" indicates neither connected nor disconnected.
    return !isDisconnected(modeName) && modeName != QUsbMode::Mode::Busy;
}

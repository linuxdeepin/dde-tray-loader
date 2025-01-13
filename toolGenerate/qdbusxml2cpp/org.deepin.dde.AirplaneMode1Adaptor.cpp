/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp ./dde-tray-loader/plugins/dde-dock/dbus/xml/org.deepin.dde.AirplaneMode1.xml -a ./dde-tray-loader/toolGenerate/qdbusxml2cpp/org.deepin.dde.AirplaneMode1Adaptor -i ./dde-tray-loader/toolGenerate/qdbusxml2cpp/org.deepin.dde.AirplaneMode1.h
 *
 * qdbusxml2cpp is Copyright (C) 2017 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#include "./dde-tray-loader/toolGenerate/qdbusxml2cpp/org.deepin.dde.AirplaneMode1Adaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class AirplaneMode1Adaptor
 */

AirplaneMode1Adaptor::AirplaneMode1Adaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

AirplaneMode1Adaptor::~AirplaneMode1Adaptor()
{
    // destructor
}

bool AirplaneMode1Adaptor::bluetoothEnabled() const
{
    // get the value of property BluetoothEnabled
    return qvariant_cast< bool >(parent()->property("BluetoothEnabled"));
}

bool AirplaneMode1Adaptor::enabled() const
{
    // get the value of property Enabled
    return qvariant_cast< bool >(parent()->property("Enabled"));
}

bool AirplaneMode1Adaptor::wifiEnabled() const
{
    // get the value of property WifiEnabled
    return qvariant_cast< bool >(parent()->property("WifiEnabled"));
}

void AirplaneMode1Adaptor::DumpState()
{
    // handle method call org.deepin.dde.AirplaneMode1.DumpState
    QMetaObject::invokeMethod(parent(), "DumpState");
}

void AirplaneMode1Adaptor::Enable(bool enabled)
{
    // handle method call org.deepin.dde.AirplaneMode1.Enable
    QMetaObject::invokeMethod(parent(), "Enable", Q_ARG(bool, enabled));
}

void AirplaneMode1Adaptor::EnableBluetooth(bool enabled)
{
    // handle method call org.deepin.dde.AirplaneMode1.EnableBluetooth
    QMetaObject::invokeMethod(parent(), "EnableBluetooth", Q_ARG(bool, enabled));
}

void AirplaneMode1Adaptor::EnableWifi(bool enabled)
{
    // handle method call org.deepin.dde.AirplaneMode1.EnableWifi
    QMetaObject::invokeMethod(parent(), "EnableWifi", Q_ARG(bool, enabled));
}


/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: lorn.potter@gmail.com
**

**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qofonoconnectionmanager.h"
#include "dbus/ofonoconnectionmanager.h"
#include "dbustypes.h"


struct OfonoContextStruct {
    QDBusObjectPath path;
    QVariantMap properties;
};
typedef QList<OfonoContextStruct> OfonoContextList;
Q_DECLARE_METATYPE(OfonoContextStruct)
Q_DECLARE_METATYPE(OfonoContextList)

QDBusArgument &operator<<(QDBusArgument &argument, const OfonoContextStruct &modem)
{
    argument.beginStructure();
    argument << modem.path << modem.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, OfonoContextStruct &modem)
{
    argument.beginStructure();
    argument >> modem.path >> modem.properties;
    argument.endStructure();
    return argument;
}

class QOfonoConnectionManagerPrivate
{
public:
    QOfonoConnectionManagerPrivate();
    QString modemPath;
    OfonoConnectionManager *connman;
    QVariantMap properties;
    QStringList contexts;
};

QOfonoConnectionManagerPrivate::QOfonoConnectionManagerPrivate() :
    modemPath(QString())
   , connman(0)
  ,contexts(QStringList())
{
    qDBusRegisterMetaType<OfonoContextStruct>();
    qDBusRegisterMetaType<OfonoContextList>();
}

QOfonoConnectionManager::QOfonoConnectionManager(QObject *parent) :
    QObject(parent),
    d_ptr(new QOfonoConnectionManagerPrivate)
{
}

QOfonoConnectionManager::~QOfonoConnectionManager()
{
    delete d_ptr;
}

void QOfonoConnectionManager::setModemPath(const QString &path)
{
    if (!d_ptr->connman) {
        d_ptr->modemPath = path;
        d_ptr->connman = new OfonoConnectionManager("org.ofono", path, QDBusConnection::systemBus(),this);

        if (d_ptr->connman) {
            connect(d_ptr->connman,SIGNAL(PropertyChanged(QString,QDBusVariant)),
                    this,SLOT(propertyChanged(QString,QDBusVariant)));

            QDBusReply<QVariantMap> reply;
            reply = d_ptr->connman->GetProperties();
            d_ptr->properties = reply.value();

            QDBusReply<OfonoContextList> reply2;
            OfonoContextList contexts;
            QStringList contextList;

            QDBusMessage request;

            request = QDBusMessage::createMethodCall("org.ofono",
                                                     "org.ofono.ConnectionManager",
                                                     path,
                                                     "GetContexts");

            reply2 = QDBusConnection::systemBus().call(request);

            contexts = reply2;
            foreach(OfonoContextStruct context, contexts) {
                contextList << context.path.path();
            }
            d_ptr->contexts = contextList;
        }
    }
}

QString QOfonoConnectionManager::modemPath() const
{
    return d_ptr->modemPath;
}

void QOfonoConnectionManager::deactivateAll()
{
    d_ptr->connman->DeactivateAll();
}

void QOfonoConnectionManager::addContext(const QString &type)
{
    d_ptr->connman->AddContext(type);
    if (!d_ptr->contexts.contains(type))
        d_ptr->contexts.append(type);
}

void QOfonoConnectionManager::removeContext(const QString &path)
{
    d_ptr->connman->RemoveContext(QDBusObjectPath(path));
    if (d_ptr->contexts.contains(path))
        d_ptr->contexts.removeOne(path);
}

bool QOfonoConnectionManager::attached() const
{
    if ( d_ptr->connman)
        return d_ptr->properties["Attached"].value<bool>();
    else
        return false;
}

QString QOfonoConnectionManager::bearer() const
{
    if ( d_ptr->connman)
        return d_ptr->properties["Bearer"].value<QString>();
    else
        return QString();
}

bool QOfonoConnectionManager::suspended() const
{
    if ( d_ptr->connman)
        return d_ptr->properties["Suspended"].value<bool>();
    else
        return false;
}


bool QOfonoConnectionManager::roamingAllowed() const
{
    if ( d_ptr->connman)
        return d_ptr->properties["RoamingAllowed"].value<bool>();
    else
        return false;
}

void QOfonoConnectionManager::setRoamingAllowed(bool b)
{
    if ( d_ptr->connman)
        d_ptr->connman->SetProperty("RoamingAllowed",QDBusVariant(b));
}


bool QOfonoConnectionManager::powered() const
{
    if ( d_ptr->connman)
        return d_ptr->properties["Powered"].value<bool>();
    else
        return false;
}

void QOfonoConnectionManager::setPowered(bool b)
{
    if ( d_ptr->connman)
        d_ptr->connman->SetProperty("Powered",QDBusVariant(b));
}


void QOfonoConnectionManager::propertyChanged(const QString& property, const QDBusVariant& dbusvalue)
{
    QVariant value = dbusvalue.variant();
    d_ptr->properties.insert(property,value);

    if (property == QLatin1String("Attached")) {
        Q_EMIT attachedChanged(value.value<bool>());
    } else if (property == QLatin1String("Bearer")) {
        Q_EMIT bearerChanged(value.value<QString>());
    } else if (property == QLatin1String("Suspended")) {
        Q_EMIT suspendedChanged(value.value<bool>());
    } else if (property == QLatin1String("RoamingAllowed")) {
        Q_EMIT roamingAllowedChanged(value.value<bool>());
    } else if (property == QLatin1String("Powered")) {
        Q_EMIT poweredChanged(value.value<bool>());
    }
}

QStringList QOfonoConnectionManager::contexts()
{
    return d_ptr->contexts;
}
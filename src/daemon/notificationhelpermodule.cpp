/***************************************************************************
 *   Copyright © 2009 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *   Copyright © 2009 Harald Sitter <apachelogger@ubuntu.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "notificationhelpermodule.h"

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QTimer>

// KDE includes
#include <KAboutData>
#include <KDirWatch>
#include <KLocalizedString>
#include <KPluginFactory>

// Own includes
#include "apportevent/apportevent.h"
#include "hookevent/hookevent.h"
#include "installevent/installevent.h"
#include "rebootevent/rebootevent.h"

#include "configwatcher.h"
#include "installevent/installdbuswatcher.h"

K_PLUGIN_FACTORY(NotificationHelperModuleFactory,
                 registerPlugin<NotificationHelperModule>();
                )
K_EXPORT_PLUGIN(NotificationHelperModuleFactory("notificationhelper"))


NotificationHelperModule::NotificationHelperModule(QObject* parent, const QList<QVariant>&)
        : KDEDModule(parent)
        , m_apportEvent(0)
        , m_hookEvent(0)
        , m_installEvent(0)
        , m_rebootEvent(0)
        , m_configWatcher(0)
        , m_installWatcher(0)
{
    KAboutData aboutData("notificationhelper", "notificationhelper",
                         ki18n("Kubuntu Notification Helper"),
                         "0.4.90", ki18n("A Notification Helper for Kubuntu"),
                         KAboutData::License_GPL,
                         ki18n("(C) 2009 Jonathan Thomas, (C) 2009 Harald Sitter"),
                         KLocalizedString(), "http://kubuntu.org",
                         "https://bugs.launchpad.net/ubuntu");

    QTimer::singleShot(5000, this, SLOT(init()));
}

NotificationHelperModule::~NotificationHelperModule()
{
    delete m_apportEvent;
    delete m_hookEvent;
    delete m_installEvent;
    delete m_rebootEvent;
    delete m_installWatcher;
    delete m_configWatcher;
}

void NotificationHelperModule::init()
{
    m_configWatcher = new ConfigWatcher(this);

    m_apportEvent = new ApportEvent(this, "Apport");
    m_hookEvent = new HookEvent(this, "Hook");
    m_installEvent = new InstallEvent(this, "Install" );
    m_rebootEvent = new RebootEvent(this, "Restart");

    if (!m_apportEvent->isHidden() && (QFile::exists("/usr/share/apport/apport-kde")
        || QFile::exists("/usr/share/apport/apport-gtk"))) {
        KDirWatch *apportDirWatch =  new KDirWatch(this);
        apportDirWatch->addDir("/var/crash/");
        connect(apportDirWatch, SIGNAL(dirty(const QString &)),
                this, SLOT(apportEvent()));
        connect(m_configWatcher, SIGNAL(reloadConfigCalled()),
                m_apportEvent, SLOT(reloadConfig()));

        // Force check, we just started up and there might have been crashes on reboot
        apportEvent();
    }

    if (!m_hookEvent->isHidden()) {
        KDirWatch *hooksDirWatch = new KDirWatch(this);
        hooksDirWatch->addDir("/var/lib/update-notifier/user.d/");
        connect(hooksDirWatch, SIGNAL(dirty(const QString &)),
                this, SLOT(hookEvent()));
        connect(m_configWatcher, SIGNAL(reloadConfigCalled()),
                m_hookEvent, SLOT(reloadConfig()));

        // Sometimes hooks are for the first boot, so force a check
        hookEvent();
    }

    if (!m_installEvent->isHidden())
    {
        m_installWatcher = new InstallDBusWatcher(this);
        connect(m_installWatcher, SIGNAL(installRestrictedCalled(const QString &, const QString &)),
                this, SLOT(installEvent(const QString &, const QString &)));
        connect(m_configWatcher, SIGNAL(reloadConfigCalled()),
                m_installEvent, SLOT(reloadConfig()));
    }

    if (!m_rebootEvent->isHidden()) {
        KDirWatch *stampDirWatch = new KDirWatch(this);
        stampDirWatch->addFile("/var/lib/update-notifier/dpkg-run-stamp");
        connect(stampDirWatch, SIGNAL(dirty(const QString &)),
                this, SLOT(rebootEvent()));
        connect(m_configWatcher, SIGNAL(reloadConfigCalled()),
                m_rebootEvent, SLOT(reloadConfig()));

        rebootEvent();
    }
}

void NotificationHelperModule::apportEvent()
{
    m_apportEvent->show();
}

void NotificationHelperModule::hookEvent()
{
    m_hookEvent->show();
}

void NotificationHelperModule::installEvent(const QString app, const QString package)
{
    m_installEvent->getInfo(app, package);
}

void NotificationHelperModule::rebootEvent()
{
    if (!QFile::exists("/var/run/reboot-required")) {
        return;
    }

    m_rebootEvent->show();
}

#include "notificationhelpermodule.moc"

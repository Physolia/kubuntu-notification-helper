/***************************************************************************
 *   Copyright © 2009 by Jonathan Thomas <echidnaman@kubuntu.org>          *
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

#include "hookgui.h"

// Qt includes
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

// KDE includes
#include <KGlobal>
#include <KProcess>
#include <KIcon>
#include <KLocale>
#include <KToolInvocation>
#include <KVBox>

HookGui::HookGui(QObject* parent, QMap< QString, QMap< QString, QString > > parsedHookMap)
        : QObject(parent)
        , m_dialog(0)
        , m_command(0)
        , m_terminal(false)
{
    m_dialog = new KPageDialog;
    m_dialog->setCaption(i18n("Update Information"));
    m_dialog->setWindowIcon(KIcon("help-hint"));
    m_dialog->setButtons(KDialog::Ok);
    connect(m_dialog, SIGNAL(okClicked()), SLOT(cleanUpDialog()));

    // Take the parsed upgrade hook(s) and put them in pages
    QMap< QString, QMap< QString, QString > >::iterator i;
    for (i = parsedHookMap.begin(); i !=  parsedHookMap.end(); ++i) {
        // Any way to do this without copying this to a new QMap?
        QMap< QString, QString > parsedHook = *i;

        KVBox *vbox = new KVBox();

        const QString language =  KGlobal::locale()->language();

        // Get a translated name if possible
        QString name;
        if (parsedHook.contains("Name-" + language)) {
            name = parsedHook["Name-" + language];
        } else {
            QMap<QString, QString>::const_iterator nameIter = parsedHook.constFind("Name");
            while (nameIter != parsedHook.end() && nameIter.key() == "Name") {
                name = nameIter.value();
                break;
            }
        }

        // Get a translated description if possible
        QLabel *descLabel = new QLabel(vbox);
        QString desc;
        if (parsedHook.contains("Description-" + language)) {
            desc = parsedHook["Description-" + language];
        } else {
            QMap< QString, QString >::const_iterator descIter = parsedHook.constFind("Description");
            while (descIter != parsedHook.end() && descIter.key() == "Description") {
                desc = descIter.value();
                break;
            }
        }

        descLabel->setWordWrap(true);
        descLabel->setText(desc);

        QMap< QString, QString >::const_iterator commandIter = parsedHook.constFind("Command");
        while (commandIter != parsedHook.end() && commandIter.key() == "Command") {
            m_command = commandIter.value();
            break;
        }

        QMap< QString, QString >::const_iterator terminalIter = parsedHook.constFind("Terminal");
        while (terminalIter != parsedHook.end() && terminalIter.key() == "Terminal") {
            QString terminalValue = terminalIter.value();
            if (terminalValue == "True") {
                m_terminal = true;
            }
            break;
        }

        if (!m_command.isEmpty()) {
            QPushButton *runButton = new QPushButton(KIcon("system-run"), i18n("Run this action now"), vbox);
            connect(runButton, SIGNAL(clicked()), this, SLOT(runHookCommand()));
        }

        KPageWidgetItem *page = new KPageWidgetItem(vbox, name);
        page->setIcon(KIcon("help-hint"));

        m_dialog->addPage(page);
    }

    m_dialog->show();
}

HookGui::~HookGui()
{
    delete m_dialog;
}

void HookGui::runHookCommand()
{
    if (m_terminal) {
        // if command is quoted, invokeTerminal will refuse to interpret it properly
        if (m_command.startsWith('\"') && m_command.endsWith('\"')) {
            m_command.remove(0, 1);
            m_command.remove((m_command.length() - 1), 1);
            KToolInvocation::invokeTerminal(m_command);
        }
    } else {
        KProcess *process = new KProcess();
        process->setShellCommand(m_command);
        process->startDetached();
    }

    m_command.clear();
    m_terminal = false;
}

void HookGui::cleanUpDialog()
{
    m_dialog->deleteLater();
    m_dialog = 0;
}

#include "hookgui.moc"

/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kolabobjecttest.h"

#include <QTest>

#include "kolabformat/kolabobject.h"
#include <kdebug.h>
#include <kolabformat/errorhandler.h>

void KolabObjectTest::preserveLatin1()
{
    KCalCore::Event::Ptr event(new KCalCore::Event());
    QString summary(QLatin1String("äöü%@$£é¤¼²°"));
    event->setSummary(summary);
    QCOMPARE(event->summary(), summary);
    //std::cout << event->summary().toStdString() << std::endl;
    KMime::Message::Ptr msg = Kolab::KolabObjectWriter::writeEvent(event);
//     kDebug() << msg->encodedContent();
    KCalCore::Event::Ptr readEvent = Kolab::KolabObjectReader(msg).getEvent();
    QVERIFY(readEvent);
//     std::cout << readEvent->summary().toStdString() << std::endl;
    QCOMPARE(readEvent->summary(), summary);
}

void KolabObjectTest::preserveUnicode()
{
    KCalCore::Event::Ptr event(new KCalCore::Event());
    QString summary(QString::fromUtf8("€Š�ـأبـ☺"));
    event->setSummary(summary);
    QCOMPARE(event->summary(), summary);
//     std::cout << event->summary().toStdString() << std::endl;
    KMime::Message::Ptr msg = Kolab::KolabObjectWriter::writeEvent(event);
//     kDebug() << msg->encodedContent();
    KCalCore::Event::Ptr readEvent = Kolab::KolabObjectReader(msg).getEvent();
    QVERIFY(readEvent);
//     std::cout << readEvent->summary().toStdString() << std::endl;
    QCOMPARE(readEvent->summary(), summary);
}

void KolabObjectTest::dontCrashWithEmptyOrganizer()
{
    KCalCore::Event::Ptr event(new KCalCore::Event());
    event->setOrganizer(KCalCore::Person::Ptr());
    event->setDtStart(KDateTime(QDate(2012,11,11)));
    Kolab::KolabObjectWriter::writeEvent(event, Kolab::KolabV2);
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Debug);
    Kolab::KolabObjectWriter::writeEvent(event);
    qDebug() << Kolab::ErrorHandler::instance().error();
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Debug);
}


void KolabObjectTest::dontCrashWithEmptyIncidence()
{
    Kolab::KolabObjectWriter::writeEvent(KCalCore::Event::Ptr());
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Critical);
    Kolab::KolabObjectWriter::writeTodo(KCalCore::Todo::Ptr());
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Critical);
    Kolab::KolabObjectWriter::writeJournal(KCalCore::Journal::Ptr());
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Critical);
    Kolab::KolabObjectWriter::writeIncidence(KCalCore::Event::Ptr());
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Critical);
    Kolab::KolabObjectWriter::writeNote(KMime::Message::Ptr());
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Critical);
}




QTEST_MAIN( KolabObjectTest )

#include "kolabobjecttest.moc"

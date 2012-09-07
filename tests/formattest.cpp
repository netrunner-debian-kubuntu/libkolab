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

#include "formattest.h"

#include <QTest>
#include <QFile>
#include <QDebug>
#include <qprocess.h>
#include <qtemporaryfile.h>
#include <QBuffer>
#include <kdebug.h>
#include <kolabcontainers.h>
#include <kolabformat.h>

#include <kcalcore/icalformat.h>
#include <kabc/vcardconverter.h>
#include <akonadi/notes/noteutils.h>

#include "testutils.h"
#include "kolabformat/kolabobject.h"
#include "kolabformat/errorhandler.h"
#include "kolabformat/kolabdefinitions.h"

void normalizeMimemessage(QString &content)
{
    content.replace(QRegExp("\\bLibkolab-\\d.\\d.\\d\\b", Qt::CaseSensitive), "Libkolab-x.x.x");
    content.replace(QRegExp("\\bLibkolabxml-\\d.\\d.\\d\\b", Qt::CaseSensitive), "Libkolabxml-x.x.x");
    content.replace(QRegExp("\\bLibkolab-\\d.\\d\\b", Qt::CaseSensitive), "Libkolab-x.x.x");
    content.replace(QRegExp("\\bLibkolabxml-\\d.\\d\\b", Qt::CaseSensitive), "Libkolabxml-x.x.x");
    content.replace(QRegExp("<uri>cid:*@kolab.resource.akonadi</uri>", Qt::CaseSensitive, QRegExp::Wildcard), "<uri>cid:id@kolab.resource.akonadi</uri>");
    content.replace(QRegExp("<last-modification-date>*</last-modification-date>", Qt::CaseSensitive, QRegExp::Wildcard), "<last-modification-date></last-modification-date>");

    content.replace(QRegExp("--nextPart\\S*", Qt::CaseSensitive), "--part");
    content.replace(QRegExp("\\bboundary=\"nextPart[^\\n]*", Qt::CaseSensitive), "boundary");
    content.replace(QRegExp("Date[^\\n]*", Qt::CaseSensitive), "Date");
}

static bool compareMimeMessage( const KMime::Message::Ptr &msg, const KMime::Message::Ptr &expectedMsg )
{
    // headers
    KCOMPARE( msg->subject()->asUnicodeString(), expectedMsg->subject()->asUnicodeString() );
    if ( msg->from()->isEmpty() || expectedMsg->from()->isEmpty() ) {
        KCOMPARE( msg->from()->asUnicodeString(), expectedMsg->from()->asUnicodeString() );
    } else {
        KCOMPARE( msg->from()->mailboxes().first().address(), expectedMsg->from()->mailboxes().first().address() ); // matching address is enough, we don't need a display name
    }
    KCOMPARE( msg->contentType()->mimeType(), expectedMsg->contentType()->mimeType() );
    KCOMPARE( msg->headerByType( X_KOLAB_TYPE_HEADER )->as7BitString(), expectedMsg->headerByType( X_KOLAB_TYPE_HEADER )->as7BitString() );
    // date contains conversion time...
    //   KCOMPARE( msg->date()->asUnicodeString(), expectedMsg->date()->asUnicodeString() );
    
    // body parts
    KCOMPARE( msg->contents().size(), expectedMsg->contents().size() );
    for ( int i = 0; i < msg->contents().size(); ++i ) {
        KMime::Content *part = msg->contents().at( i );
        KMime::Content *expectedPart = expectedMsg->contents().at( i );
        
        // part headers
        KCOMPARE( part->contentType()->mimeType(), expectedPart->contentType()->mimeType() );
        KCOMPARE( part->contentDisposition()->filename(), expectedPart->contentDisposition()->filename() );
        
        KCOMPARE( part->decodedContent().isEmpty(), false );
        
        QString content(part->decodedContent());
        normalizeMimemessage(content);
        QString expected(expectedPart->decodedContent());
        normalizeMimemessage(expected);
//         showDiff(expected, content);

        // part content
        KCOMPARE( content.simplified(), expected.simplified() );
    }
    return true;
}


void FormatTest::testIncidence_data()
{
    QTest::addColumn<Kolab::Version>( "version" );
    QTest::addColumn<Kolab::ObjectType>( "type" );
    QTest::addColumn<QString>( "icalFileName" );
    QTest::addColumn<QString>( "mimeFileName" );
    
    QTest::newRow( "v2eventSimple" ) << Kolab::KolabV2 << Kolab::EventObject << TESTFILEDIR+QString::fromLatin1("v2/event/simple.ics") << TESTFILEDIR+QString::fromLatin1("v2/event/simple.ics.mime");
    QTest::newRow( "v2eventComplex" ) << Kolab::KolabV2 << Kolab::EventObject << TESTFILEDIR+QString::fromLatin1("v2/event/complex.ics") << TESTFILEDIR+QString::fromLatin1("v2/event/complex.ics.mime");
    QTest::newRow( "v2eventAttachment" ) << Kolab::KolabV2 << Kolab::EventObject << TESTFILEDIR+QString::fromLatin1("v2/event/attachment.ics") << TESTFILEDIR+QString::fromLatin1("v2/event/attachment.ics.mime");
    QTest::newRow( "v2eventAllday" ) << Kolab::KolabV2 << Kolab::EventObject << TESTFILEDIR+QString::fromLatin1("v2/event/allday.ics") << TESTFILEDIR+QString::fromLatin1("v2/event/allday.ics.mime");
    //The following test just fails because we have a nicer mime message output than horde
//     QTest::newRow( "v2eventHorde" ) << Kolab::KolabV2 << Kolab::EventObject << TESTFILEDIR+QString::fromLatin1("v2/event/horde.ics") << TESTFILEDIR+QString::fromLatin1("v2/event/horde.ics.mime");
    QTest::newRow( "v2todoSimple" ) << Kolab::KolabV2 << Kolab::TodoObject << TESTFILEDIR+QString::fromLatin1("v2/task/simple.ics") << TESTFILEDIR+QString::fromLatin1("v2/task/simple.ics.mime");
    QTest::newRow( "v2todoComplex" ) << Kolab::KolabV2 << Kolab::TodoObject << TESTFILEDIR+QString::fromLatin1("v2/task/complex.ics") << TESTFILEDIR+QString::fromLatin1("v2/task/complex.ics.mime");
    QTest::newRow( "v2todoPrio1" ) << Kolab::KolabV2 << Kolab::TodoObject << TESTFILEDIR+QString::fromLatin1("v2/task/prioritytest1.ics") << TESTFILEDIR+QString::fromLatin1("v2/task/prioritytest1.ics.mime");
    QTest::newRow( "v2todoPrio2" ) << Kolab::KolabV2 << Kolab::TodoObject << TESTFILEDIR+QString::fromLatin1("v2/task/prioritytest2.ics") << TESTFILEDIR+QString::fromLatin1("v2/task/prioritytest2.ics.mime");
    QTest::newRow( "v2journalSimple" ) << Kolab::KolabV2 << Kolab::JournalObject << TESTFILEDIR+QString::fromLatin1("v2/journal/simple.ics") << TESTFILEDIR+QString::fromLatin1("v2/journal/simple.ics.mime");
    QTest::newRow( "v2journalComplex" ) << Kolab::KolabV2 << Kolab::JournalObject << TESTFILEDIR+QString::fromLatin1("v2/journal/complex.ics") << TESTFILEDIR+QString::fromLatin1("v2/journal/complex.ics.mime");

    QTest::newRow( "v3eventSimple" ) << Kolab::KolabV3 << Kolab::EventObject << TESTFILEDIR+QString::fromLatin1("v3/event/simple.ics") << TESTFILEDIR+QString::fromLatin1("v3/event/simple.ics.mime");
    QTest::newRow( "v3eventComplex" ) << Kolab::KolabV3 << Kolab::EventObject << TESTFILEDIR+QString::fromLatin1("v3/event/complex.ics") << TESTFILEDIR+QString::fromLatin1("v3/event/complex.ics.mime");
    QTest::newRow( "v3todoSimple" ) << Kolab::KolabV3 << Kolab::TodoObject << TESTFILEDIR+QString::fromLatin1("v3/task/simple.ics") << TESTFILEDIR+QString::fromLatin1("v3/task/simple.ics.mime");
    QTest::newRow( "v3todoComplex" ) << Kolab::KolabV3 << Kolab::TodoObject << TESTFILEDIR+QString::fromLatin1("v3/task/complex.ics") << TESTFILEDIR+QString::fromLatin1("v3/task/complex.ics.mime");
    QTest::newRow( "v3journalSimple" ) << Kolab::KolabV3 << Kolab::JournalObject << TESTFILEDIR+QString::fromLatin1("v3/journal/simple.ics") << TESTFILEDIR+QString::fromLatin1("v3/journal/simple.ics.mime");
    QTest::newRow( "v3journalComplex" ) << Kolab::KolabV3 << Kolab::JournalObject << TESTFILEDIR+QString::fromLatin1("v3/journal/complex.ics") << TESTFILEDIR+QString::fromLatin1("v3/journal/complex.ics.mime");
}


void FormatTest::testIncidence()
{
    QFETCH( Kolab::Version, version );
    QFETCH( Kolab::ObjectType, type );
    QFETCH( QString, icalFileName ); //To compare
    QFETCH( QString, mimeFileName ); //For parsing
    
    //Parse mime message
    bool ok = false;
    const KMime::Message::Ptr &msg = readMimeFile( mimeFileName, ok );
    QVERIFY(ok);
    Kolab::KolabObjectReader reader;
    Kolab::ObjectType t = reader.parseMimeMessage(msg);
    QCOMPARE(t, type);
    QCOMPARE(reader.getVersion(), version);
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Debug);

    KCalCore::Incidence::Ptr convertedIncidence = reader.getIncidence();
    
    //Parse ICalFile for comparison
    QFile icalFile( icalFileName );
    QVERIFY( icalFile.open( QFile::ReadOnly ) );
    KCalCore::ICalFormat format;
    KCalCore::Incidence::Ptr realIncidence( format.fromString( QString::fromUtf8( icalFile.readAll() ) ) );
        
    // fix up the converted incidence for comparisson
    normalizeIncidence(convertedIncidence);
    normalizeIncidence(realIncidence);

    // recurrence objects are created on demand, but KCalCore::Incidence::operator==() doesn't take that into account
    // so make sure both incidences have one
    realIncidence->recurrence();
    convertedIncidence->recurrence();
    
    realIncidence->setLastModified(convertedIncidence->lastModified());
    
    //The following test is just for debugging and not really relevant
    if ( *(realIncidence.data()) != *(convertedIncidence.data()) ) {
        showDiff(format.toString( realIncidence ), format.toString( convertedIncidence ));
    }
    QVERIFY( *(realIncidence.data()) ==  *(convertedIncidence.data()) );
              
    
    //Write
    Kolab::overrideTimestamp(Kolab::cDateTime(2012, 5, 5, 5,5,5, true));
    KMime::Message::Ptr convertedMime = Kolab::KolabObjectWriter::writeIncidence(realIncidence, version);
        
    if ( !compareMimeMessage( convertedMime, msg )) {
        showDiff(msg->encodedContent(), convertedMime->encodedContent());
        QVERIFY( false );
    }
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Debug);
}


void FormatTest::testContact_data()
{
    QTest::addColumn<Kolab::Version>( "version" );
    QTest::addColumn<Kolab::ObjectType>( "type" );
    QTest::addColumn<QString>( "vcardFileName" );
    QTest::addColumn<QString>( "mimeFileName" );
    
    QTest::newRow( "v2contactSimple" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/simple.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/simple.vcf.mime");
    QTest::newRow( "v2contactComplex" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/complex.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/complex.vcf.mime");
    QTest::newRow( "v2contactAddress" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/address.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/address.vcf.mime");
    QTest::newRow( "v2contactBug238996" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/bug238996.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/bug238996.vcf.mime");
    QTest::newRow( "v2contactDisplayname" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/displayname.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/displayname.vcf.mime");
    QTest::newRow( "v2contactEmails" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/emails.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/emails.vcf.mime");
    QTest::newRow( "v2contactPhonenumbers" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/phonenumbers.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/phonenumbers.vcf.mime");
    QTest::newRow( "v2contactPicture" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/picture.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/picture.vcf.mime");
    //FIXME the following test fails because the vcard implementation always writes jpeg (which is lossy). The reference vcf file is therefore probably also not really useful
//     QTest::newRow( "v2pictureJPGHorde" ) << Kolab::KolabV2 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v2/contacts/pictureJPGHorde.vcf") << TESTFILEDIR+QString::fromLatin1("v2/contacts/pictureJPGHorde.vcf.mime");
    
    QTest::newRow( "v3contactSimple" ) << Kolab::KolabV3 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v3/contacts/simple.vcf") << TESTFILEDIR+QString::fromLatin1("v3/contacts/simple.vcf.mime");
    QTest::newRow( "v3contactComplex" ) << Kolab::KolabV3 << Kolab::ContactObject << TESTFILEDIR+QString::fromLatin1("v3/contacts/complex.vcf") << TESTFILEDIR+QString::fromLatin1("v3/contacts/complex.vcf.mime");
}

bool comparePictureToReference(const QImage &picture)
{
    QImage img(TESTFILEDIR+QString::fromLatin1("picture.jpg"));
    QByteArray pic;
    QBuffer buffer(&pic);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "JPEG");
    buffer.close();

    QByteArray pic2;
    QBuffer buffer2(&pic2);
    buffer2.open(QIODevice::WriteOnly);
    picture.save(&buffer2, "JPEG");
    buffer2.close();

    if(pic.toBase64() != pic2.toBase64()) {
        qDebug() << pic.toBase64();
        qDebug() << pic2.toBase64();
        return false;
    }
    return true;
}

void FormatTest::testContact()
{
    QFETCH( Kolab::Version, version );
    QFETCH( Kolab::ObjectType, type );
    QFETCH( QString, vcardFileName ); //To compare
    QFETCH( QString, mimeFileName ); //For parsing
    
    //Parse mime message
    bool ok = false;
    const KMime::Message::Ptr &msg = readMimeFile( mimeFileName, ok );
    QVERIFY(ok);
    Kolab::KolabObjectReader reader;
    Kolab::ObjectType t = reader.parseMimeMessage(msg);
    QCOMPARE(t, type);
    QCOMPARE(reader.getVersion(), version);
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Debug);
    
    KABC::Addressee convertedAddressee = reader.getContact();
    QVERIFY(!convertedAddressee.isEmpty());
    
    //Parse vcard
    QFile vcardFile( vcardFileName );
    QVERIFY( vcardFile.open( QFile::ReadOnly ) );
    KABC::VCardConverter converter;
    const QByteArray &c = vcardFile.readAll();
    KABC::Addressee realAddressee = converter.parseVCard( c );

    // fix up the converted addressee for comparisson
    convertedAddressee.setName( realAddressee.name() ); // name() apparently is something strange
    if (version == Kolab::KolabV2) { //No creation date in xcal
        QVERIFY( !convertedAddressee.custom( "KOLAB", "CreationDate" ).isEmpty() );
        convertedAddressee.removeCustom( "KOLAB", "CreationDate" ); // that's conversion time !?
    } else {
        normalizeContact(convertedAddressee);
        normalizeContact(realAddressee);
    }
    QVERIFY( normalizePhoneNumbers( convertedAddressee, realAddressee ) ); // phone number ids are random
    QVERIFY( normalizeAddresses( convertedAddressee, realAddressee ) ); // same here
//     QCOMPARE(realAddressee.photo().type(), convertedAddressee.photo().type());
    if (realAddressee != convertedAddressee) {
        showDiff(converter.createVCard(realAddressee), converter.createVCard(convertedAddressee));
    }
    QCOMPARE( realAddressee, convertedAddressee );

    //Write
    const KMime::Message::Ptr &convertedMime = Kolab::KolabObjectWriter::writeContact(realAddressee, version);
    
    if ( !compareMimeMessage( convertedMime, msg )) {
        QString expected = msg->encodedContent();
        normalizeMimemessage(expected);
        QString converted = convertedMime->encodedContent();
        normalizeMimemessage(converted);
        showDiff(expected, converted);
        QVERIFY( false );
    }
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Debug);
}


void FormatTest::testNote_data()
{
    QTest::addColumn<Kolab::Version>( "version" );
    QTest::addColumn<Kolab::ObjectType>( "type" );
    QTest::addColumn<QString>( "noteFileName" );
    QTest::addColumn<QString>( "mimeFileName" );

    QTest::newRow( "v3noteSimple" ) << Kolab::KolabV3 << Kolab::NoteObject << TESTFILEDIR+QString::fromLatin1("v3/note/note.mime") << TESTFILEDIR+QString::fromLatin1("v3/note/note.mime.mime");
}


void FormatTest::testNote()
{
    QFETCH( Kolab::Version, version );
    QFETCH( Kolab::ObjectType, type );
    QFETCH( QString, noteFileName ); //To compare
    QFETCH( QString, mimeFileName ); //For parsing

    //Parse mime message
    bool ok = false;
    const KMime::Message::Ptr &msg = readMimeFile( mimeFileName, ok );
    QVERIFY(ok);
    Kolab::KolabObjectReader reader;
    Kolab::ObjectType t = reader.parseMimeMessage(msg);
    QCOMPARE(t, type);
    QCOMPARE(reader.getVersion(), version);
    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Debug);

    KMime::Message::Ptr convertedNote = reader.getNote();
    QVERIFY(convertedNote.get());

    //Parse note
    const KMime::Message::Ptr &realNote = readMimeFile( noteFileName, ok );
    QVERIFY(ok);
    QVERIFY(realNote.get());

    QString expected = realNote->encodedContent();
    normalizeMimemessage(expected);
    QString converted = convertedNote->encodedContent();
    normalizeMimemessage(converted);
    showDiff(expected, converted);
    
    //Write
    const KMime::Message::Ptr &convertedMime = Kolab::KolabObjectWriter::writeNote(realNote, version);
    QVERIFY(convertedMime.get());
    QVERIFY(msg.get());

    QString expected2 = msg->encodedContent();
    normalizeMimemessage(expected2);
    QString converted2 = convertedMime->encodedContent();
    normalizeMimemessage(converted2);
    showDiff(expected2, converted2);

    QCOMPARE(Kolab::ErrorHandler::instance().error(), Kolab::ErrorHandler::Debug);
}


//This function exists only to generate the reference files, it's not a real test.
void FormatTest::generateMimefile()
{
//     QFile icalFile( TESTFILEDIR+QString::fromLatin1("v3/journal/complex.ics") );
//     QVERIFY( icalFile.open( QFile::ReadOnly ) );
//     KCalCore::ICalFormat format;
//     const KCalCore::Incidence::Ptr realIncidence( format.fromString( QString::fromUtf8( icalFile.readAll() ) ) );
// 
//     QString result;
//     QTextStream s(&result);
//     Kolab::overrideTimestamp(Kolab::cDateTime(2012, 5, 5, 5,5,5, true));
//     Kolab::KolabObjectWriter::writeIncidence(realIncidence, Kolab::KolabV3)->toStream(s);
    
//     QFile vcardFile( TESTFILEDIR+QString::fromLatin1("v3/contacts/complex.vcf") );
//     QVERIFY( vcardFile.open( QFile::ReadOnly ) );
//     KABC::VCardConverter converter;
//     const KABC::Addressee realAddressee = converter.parseVCard( vcardFile.readAll() );
// 
//     kDebug() << realAddressee.photo().data();
// 
//     QString result;
//     QTextStream s(&result);
//     Kolab::overrideTimestamp(Kolab::cDateTime(2012, 5, 5, 5,5,5, true));
//     Kolab::KolabObjectWriter::writeContact(realAddressee, Kolab::KolabV3)->toStream(s);
    
//     kDebug() << result;
}

void FormatTest::generateVCard()
{
//     bool ok = false;
//     const KMime::Message::Ptr &msg = readMimeFile( QString::fromLatin1("../")+TESTFILEDIR+QString::fromLatin1("v2/contacts/pictureJPGHorde.vcf.mime"), ok );
//     qDebug() << msg->encodedContent();
//     Kolab::KolabObjectReader reader;
//     Kolab::ObjectType t = reader.parseMimeMessage(msg);
// 
//     KABC::Addressee convertedAddressee = reader.getContact();
//     KABC::VCardConverter converter;
//     qDebug() << converter.createVCard(convertedAddressee);
}

//Pseudo test to show that JPG is always lossy, even with quality set to 100
void FormatTest::proveJPGisLossy()
{
//     QImage img(TESTFILEDIR+QString::fromLatin1("picture.jpg"));
//     QByteArray pic;
//     QBuffer buffer(&pic);
//     buffer.open(QIODevice::WriteOnly);
//     img.save(&buffer, "JPEG");
//     buffer.close();
//     qDebug() << pic.toBase64();
// 
//     QImage img2;
//     QByteArray pic2;
//     QBuffer buffer2(&pic2);
//     img2.loadFromData(pic);
//     img2.save(&buffer2, "JPEG");
//     buffer2.close();
//     qDebug() << pic2.toBase64();
    
}

QTEST_MAIN( FormatTest )

#include "formattest.moc"

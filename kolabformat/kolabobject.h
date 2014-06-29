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

#ifndef KOLABOBJECT_H
#define KOLABOBJECT_H

#include <kolab_export.h>

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>
#include <kcalcore/incidence.h>
#include <kcalcore/event.h>
#include <kcalcore/journal.h>
#include <kcalcore/todo.h>
#include <kmime/kmime_message.h>

#include "kolabdefinitions.h"

namespace Kolab {

class Freebusy;


KOLAB_EXPORT KCalCore::Event::Ptr readV2EventXML(const QByteArray &xmlData, QStringList &attachments);

/**
 * Class to read Kolab Mime files
 * 
 * It implements the Kolab specifics of Mime message handling.
 * This class is not reusable and only meant to read a single object.
 * Parse the mime message and then call the correct getter, based on the type
 * 
 */
class KOLAB_EXPORT KolabObjectReader {
public:
    KolabObjectReader();
    explicit KolabObjectReader(const KMime::Message::Ptr &msg);
    ~KolabObjectReader();
    
    ObjectType parseMimeMessage(const KMime::Message::Ptr &msg);

    /**
     * Set to override the autodetected object type, before parsing the message.
     */
    void setObjectType(ObjectType);
    
    /**
     * Set to override the autodetected version, before parsing the message.
     */
    void setVersion(Version);
    
    /**
     * Returns the Object type of the parsed kolab object.
     */
    ObjectType getType() const;
    /**
     * Returns the kolab-format version of the parsed kolab object.
     */
    Version getVersion() const;
    
    /**
     * Getter to get the retrieved object.
     * Only the correct one will return a valid object.
     * 
     * Use getType() to determine the correct one to call.
     */
    KCalCore::Event::Ptr getEvent() const;
    KCalCore::Todo::Ptr getTodo() const;
    KCalCore::Journal::Ptr getJournal() const;
    KCalCore::Incidence::Ptr getIncidence() const;
    KABC::Addressee getContact() const;
    KABC::ContactGroup getDistlist() const;
    KMime::Message::Ptr getNote() const;
    QStringList getDictionary(QString &lang) const;
    Freebusy getFreebusy() const;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

/**
 * Class to write Kolab Mime files
 * 
 */
class KOLAB_EXPORT KolabObjectWriter {
public:

    static KMime::Message::Ptr writeEvent(const KCalCore::Event::Ptr &, Version v = KolabV3, const QString &productId = QString(), const QString &tz = QString());
    static KMime::Message::Ptr writeTodo(const KCalCore::Todo::Ptr &, Version v = KolabV3, const QString &productId = QString(),const QString &tz = QString());
    static KMime::Message::Ptr writeJournal(const KCalCore::Journal::Ptr &, Version v = KolabV3, const QString &productId = QString(),const QString &tz = QString());
    static KMime::Message::Ptr writeIncidence(const KCalCore::Incidence::Ptr &, Version v = KolabV3, const QString &productId = QString(),const QString &tz = QString());
    static KMime::Message::Ptr writeContact(const KABC::Addressee &, Version v = KolabV3, const QString &productId = QString());
    static KMime::Message::Ptr writeDistlist(const KABC::ContactGroup &, Version v = KolabV3, const QString &productId = QString());
    static KMime::Message::Ptr writeNote(const KMime::Message::Ptr &, Version v = KolabV3, const QString &productId = QString());
    static KMime::Message::Ptr writeDictionary(const QStringList &, const QString &lang, Version v = KolabV3, const QString &productId = QString());
    static KMime::Message::Ptr writeFreebusy(const Kolab::Freebusy &, Version v = KolabV3, const QString &productId = QString());
    
};

} //Namespace

#endif // KOLABOBJECT_H

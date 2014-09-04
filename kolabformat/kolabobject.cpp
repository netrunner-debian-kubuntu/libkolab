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


#include "kolabobject.h"
#include "v2helpers.h"
#include "kolabdefinitions.h"
#include "errorhandler.h"
#include "libkolab-version.h"

#include <kolabbase.h>
#include <kolabformatV2/journal.h>
#include <kolabformatV2/task.h>
#include <kolabformatV2/event.h>
#include <kolabformatV2/contact.h>
#include <kolabformatV2/distributionlist.h>
#include <kolabformatV2/note.h>
#include <mime/mimeutils.h>
#include <conversion/kcalconversion.h>
#include <conversion/kabcconversion.h>
#include <conversion/kolabconversion.h>
#include <conversion/commonconversion.h>
#include <akonadi/notes/noteutils.h>
#include <kolabformat.h>


namespace Kolab {


static inline QString eventKolabType() { return QString::fromLatin1(KOLAB_TYPE_EVENT); };
static inline QString todoKolabType() { return QString::fromLatin1(KOLAB_TYPE_TASK); };
static inline QString journalKolabType() { return QString::fromLatin1(KOLAB_TYPE_JOURNAL); };
static inline QString contactKolabType() { return QString::fromLatin1(KOLAB_TYPE_CONTACT); };
static inline QString distlistKolabType() { return QString::fromLatin1(KOLAB_TYPE_DISTLIST); }
static inline QString distlistKolabTypeCompat() { return QString::fromLatin1(KOLAB_TYPE_DISTLIST_V2); }
static inline QString noteKolabType() { return QString::fromLatin1(KOLAB_TYPE_NOTE); }
static inline QString configurationKolabType() { return QString::fromLatin1(KOLAB_TYPE_CONFIGURATION); }
static inline QString dictKolabType() { return QString::fromLatin1(KOLAB_TYPE_DICT); }
static inline QString freebusyKolabType() { return QString::fromLatin1(KOLAB_TYPE_FREEBUSY); }

static inline QString xCalMimeType() { return QString::fromLatin1(MIME_TYPE_XCAL); };
static inline QString xCardMimeType() { return QString::fromLatin1(MIME_TYPE_XCARD); };
static inline QString kolabMimeType() { return QString::fromLatin1(MIME_TYPE_KOLAB); };

KCalCore::Event::Ptr readV2EventXML(const QByteArray& xmlData, QStringList& attachments)
{
    return fromXML<KCalCore::Event::Ptr, KolabV2::Event>(xmlData, attachments);
}


//@cond PRIVATE
class KolabObjectReader::Private
{
public:
    Private()
    :   mObjectType( InvalidObject ),
        mVersion( KolabV3 ),
        mOverrideObjectType(InvalidObject),
        mDoOverrideVersion(false)
    {
        mAddressee = KABC::Addressee();
    }

    ObjectType readKolabV2(const KMime::Message::Ptr &msg, Kolab::ObjectType objectType);
    ObjectType readKolabV3(const KMime::Message::Ptr &msg, Kolab::ObjectType objectType);
    
    KCalCore::Incidence::Ptr mIncidence;
    KABC::Addressee mAddressee;
    KABC::ContactGroup mContactGroup;
    KMime::Message::Ptr mNote;
    QStringList mDictionary;
    QString mDictionaryLanguage;
    ObjectType mObjectType;
    Version mVersion;
    Kolab::Freebusy mFreebusy;
    ObjectType mOverrideObjectType;
    Version mOverrideVersion;
    bool mDoOverrideVersion;
};
//@endcond

KolabObjectReader::KolabObjectReader()
: d( new KolabObjectReader::Private )
{
}

KolabObjectReader::KolabObjectReader(const KMime::Message::Ptr& msg)
: d( new KolabObjectReader::Private )
{
    d->mObjectType = parseMimeMessage(msg);
}

KolabObjectReader::~KolabObjectReader()
{
    delete d;
}

void KolabObjectReader::setObjectType(ObjectType type)
{
    d->mOverrideObjectType = type;
}

void KolabObjectReader::setVersion(Version version)
{
    d->mOverrideVersion = version;
    d->mDoOverrideVersion = true;
}

Kolab::ObjectType getObjectType(const QString &type)
{
    if (type == eventKolabType()) {
        return EventObject;
    } else if (type == todoKolabType()) {
        return TodoObject;
    } else if (type == journalKolabType()) {
        return JournalObject;
    } else if (type == contactKolabType()) {
        return ContactObject;
    } else if (type == distlistKolabType() || type == distlistKolabTypeCompat()) {
        return DistlistObject;
    } else if (type == noteKolabType()) {
        return NoteObject;
    } else if (type == freebusyKolabType()) {
        return FreebusyObject;
    } else if (type.contains(dictKolabType())) { //Previous versions appended the language to the type
        return DictionaryConfigurationObject;
    }
    Warning() << "Unknown object type: " << type;
    return Kolab::InvalidObject;
}

QByteArray getTypeString(Kolab::ObjectType type)
{
    switch (type) {
        case EventObject:
            return KOLAB_TYPE_EVENT;
        case TodoObject:
            return KOLAB_TYPE_TASK;
        case JournalObject:
            return KOLAB_TYPE_JOURNAL;
        case FreebusyObject:
            return KOLAB_TYPE_FREEBUSY;
        case ContactObject:
            return KOLAB_TYPE_CONTACT;
        case DistlistObject:
            return KOLAB_TYPE_DISTLIST;
        case NoteObject:
            return KOLAB_TYPE_NOTE;
        case DictionaryConfigurationObject:
            return KOLAB_TYPE_CONFIGURATION;
        default:
            Critical() << "unknown type "<< type;
    }
    return QByteArray();
}

QByteArray getMimeType(Kolab::ObjectType type)
{
    switch (type) {
        case EventObject:
        case TodoObject:
        case JournalObject:
        case FreebusyObject:
            return MIME_TYPE_XCAL;
        case ContactObject:
        case DistlistObject:
            return MIME_TYPE_XCARD;
        case NoteObject:
        case DictionaryConfigurationObject:
            return MIME_TYPE_KOLAB;
        default:
            Critical() << "unknown type "<< type;
    }
    return QByteArray();
}

Kolab::ObjectType detectType(const KMime::Message::Ptr &msg)
{
    Q_FOREACH(const QByteArray &type, Mime::getContentMimeTypeList(msg)) {
        Kolab::ObjectType t = getObjectType(type); //works for v2 types
        if (t != InvalidObject) {
            return t;
        }
    }
    return InvalidObject;
}

void printMessageDebugInfo(const KMime::Message::Ptr &msg)
{
    //TODO replace by Debug stream for Mimemessage
    Debug() << "MessageId: " << msg->messageID()->asUnicodeString();
    Debug() << "Subject: " << msg->subject()->asUnicodeString();
//     Debug() << msg->encodedContent();
}

ObjectType KolabObjectReader::Private::readKolabV2(const KMime::Message::Ptr &msg, Kolab::ObjectType objectType)
{
    if (objectType == DictionaryConfigurationObject) {
        KMime::Content *xmlContent = Mime::findContentByType( msg, "application/xml" );
        if ( !xmlContent ) {
            Critical() << "no application/xml part found";
            printMessageDebugInfo(msg);
            return InvalidObject;
        }
        const QByteArray &xmlData = xmlContent->decodedContent();
        mDictionary = readLegacyDictionaryConfiguration(xmlData, mDictionaryLanguage);
        mObjectType = objectType;
        return mObjectType;
    }
    KMime::Content *xmlContent = Mime::findContentByType( msg, getTypeString(objectType)  );
    if ( !xmlContent ) {
        Critical() << "no part with type" << getTypeString(objectType) << " found";
        printMessageDebugInfo(msg);
        return InvalidObject;
    }
    const QByteArray &xmlData = xmlContent->decodedContent();
    Q_ASSERT(!xmlData.isEmpty());
    QStringList attachments;

    switch (objectType) {
        case EventObject:
            mIncidence = fromXML<KCalCore::Event::Ptr, KolabV2::Event>(xmlData, attachments);
            break;
        case TodoObject:
            mIncidence = fromXML<KCalCore::Todo::Ptr, KolabV2::Task>(xmlData, attachments);
            break;
        case JournalObject:
            mIncidence = fromXML<KCalCore::Journal::Ptr, KolabV2::Journal>(xmlData, attachments);
            break;
        case ContactObject:
            mAddressee = addresseeFromKolab(xmlData, msg);
            break;
        case DistlistObject:
            mContactGroup = contactGroupFromKolab(xmlData);
            break;
        case NoteObject:
            mNote = noteFromKolab(xmlData, msg->date()->dateTime());
            break;
        default:
            CRITICAL("no kolab object found ");
            break;
    }
    if (!mIncidence.isNull()) {
//             kDebug() << "v2 attachments " << attachments.size() << d->mIncidence->attachments().size();
        mIncidence->clearAttachments();
        Mime::getAttachments(mIncidence, attachments, msg);
        if (mIncidence->attachments().size() != attachments.size()) {
            Error() << "Could not extract all attachments. " << mIncidence->attachments().size() << " out of " << attachments.size();
        }
    }
    if (ErrorHandler::errorOccured()) {
        printMessageDebugInfo(msg);
        return InvalidObject;
    }
    mObjectType = objectType;
    return mObjectType;
}

ObjectType KolabObjectReader::Private::readKolabV3(const KMime::Message::Ptr &msg, Kolab::ObjectType objectType)
{
    KMime::Content * const xmlContent = Mime::findContentByType( msg, getMimeType(objectType) );
    if ( !xmlContent ) {
        Critical() << "no " << getMimeType(objectType) << " part found";
        printMessageDebugInfo(msg);
        return InvalidObject;
    }
    const QByteArray &content = xmlContent->decodedContent();
    const std::string xml = std::string(content.data(), content.size());
    switch (objectType) {
        case EventObject: {
            const Kolab::Event & event = Kolab::readEvent(xml, false);
            mIncidence = Kolab::Conversion::toKCalCore(event);
        }
            break;
        case TodoObject: {
            const Kolab::Todo & event = Kolab::readTodo(xml, false);
            mIncidence = Kolab::Conversion::toKCalCore(event);
        }
            break;
        case JournalObject: {
            const Kolab::Journal & event = Kolab::readJournal(xml, false);
            mIncidence = Kolab::Conversion::toKCalCore(event);
        }
            break;
        case ContactObject: {
            const Kolab::Contact &contact = Kolab::readContact(xml, false);
            mAddressee = Kolab::Conversion::toKABC(contact); //TODO extract attachments
        }
            break;
        case DistlistObject: {
            const Kolab::DistList &distlist = Kolab::readDistlist(xml, false);
            mContactGroup = Kolab::Conversion::toKABC(distlist);
        }
            break;
        case NoteObject: {
            const Kolab::Note &note = Kolab::readNote(xml, false);
            mNote = Kolab::Conversion::toNote(note);
        }
            break;
        case DictionaryConfigurationObject: {
            const Kolab::Configuration &configuration = Kolab::readConfiguration(xml, false);
            const Kolab::Dictionary &dictionary = configuration.dictionary();
            mDictionary.clear();
            foreach (const std::string &entry, dictionary.entries()) {
                mDictionary.append(Conversion::fromStdString(entry));
            }
            mDictionaryLanguage = Conversion::fromStdString(dictionary.language());
        }
            break;
        case FreebusyObject: {
            const Kolab::Freebusy &fb = Kolab::readFreebusy(xml, false);
            mFreebusy = fb;
        }
            break;
        default:
            Critical() << "no kolab object found ";
            printMessageDebugInfo(msg);
            break;
    }

    if (!mIncidence.isNull()) {
//             kDebug() << "getting attachments";
        Mime::getAttachmentsById(mIncidence, msg);
    }
    ErrorHandler::handleLibkolabxmlErrors();
    if (ErrorHandler::errorOccured()) {
        printMessageDebugInfo(msg);
        return InvalidObject;
    }
    mObjectType = objectType;
    return mObjectType;
}

ObjectType KolabObjectReader::parseMimeMessage(const KMime::Message::Ptr &msg)
{
    ErrorHandler::clearErrors();
    d->mObjectType = InvalidObject;
    if (msg->contents().isEmpty()) {
        Critical() << "message has no contents (we likely failed to parse it correctly)";
        printMessageDebugInfo(msg);
        return InvalidObject;
    }
    Kolab::ObjectType objectType = InvalidObject;
    if (d->mOverrideObjectType == InvalidObject) {
        if (KMime::Headers::Base *xKolabHeader = msg->getHeaderByType(X_KOLAB_TYPE_HEADER)) {
            objectType = getObjectType(xKolabHeader->asUnicodeString().trimmed());
        } else {
            Warning() << "could not find the X-Kolab-Type Header, trying autodetection" ;
            //This works only for v2 messages atm.
            objectType = detectType(msg);
        }
    } else {
        objectType = d->mOverrideObjectType;
    }
    if (objectType == InvalidObject) {
        Critical() << "unable to detect object type";
        printMessageDebugInfo(msg);
        return InvalidObject;
    }

    if (!d->mDoOverrideVersion) {
        KMime::Headers::Base *xKolabVersion = msg->getHeaderByType(X_KOLAB_MIME_VERSION_HEADER);
        if (!xKolabVersion) {
            //For backwards compatibility to development versions, can be removed in future versions
            xKolabVersion = msg->getHeaderByType(X_KOLAB_MIME_VERSION_HEADER_COMPAT);
        }
        if (!xKolabVersion || xKolabVersion->asUnicodeString() == KOLAB_VERSION_V2) {
            d->mVersion = KolabV2;
        } else {
            if (xKolabVersion->asUnicodeString() != KOLAB_VERSION_V3) { //TODO version compatibility check?
                Warning() << "Kolab Version Header available but not on the same version as the implementation: " << xKolabVersion->asUnicodeString();
            }
            d->mVersion = KolabV3;
        }
    } else {
        d->mVersion = d->mOverrideVersion;
    }

    if (d->mVersion == KolabV2) {
        return d->readKolabV2(msg, objectType);
    }
    return d->readKolabV3(msg, objectType);
}

Version KolabObjectReader::getVersion() const
{
    return d->mVersion;
}

ObjectType KolabObjectReader::getType() const
{
    return d->mObjectType;
}

KCalCore::Event::Ptr KolabObjectReader::getEvent() const
{
    return d->mIncidence.dynamicCast<KCalCore::Event>();
}

KCalCore::Todo::Ptr KolabObjectReader::getTodo() const
{
    return d->mIncidence.dynamicCast<KCalCore::Todo>();
}

KCalCore::Journal::Ptr KolabObjectReader::getJournal() const
{
    return d->mIncidence.dynamicCast<KCalCore::Journal>();
}

KCalCore::Incidence::Ptr KolabObjectReader::getIncidence() const
{
    return d->mIncidence;
}

KABC::Addressee KolabObjectReader::getContact() const
{
    return d->mAddressee;
}

KABC::ContactGroup KolabObjectReader::getDistlist() const
{
    return d->mContactGroup;
}

KMime::Message::Ptr KolabObjectReader::getNote() const
{
    return d->mNote;
}

QStringList KolabObjectReader::getDictionary(QString& lang) const
{
    lang = d->mDictionaryLanguage;
    return d->mDictionary;
}

Freebusy KolabObjectReader::getFreebusy() const
{
    return d->mFreebusy;
}


//Normalize incidences before serializing them
KCalCore::Incidence::Ptr normalizeIncidence(KCalCore::Incidence::Ptr original)
{
    KCalCore::Incidence::Ptr i = KCalCore::Incidence::Ptr(original->clone()); //We copy to avoid destructive writing
    Q_FOREACH (KCalCore::Attachment::Ptr attachment, i->attachments()) {
        attachment->setUri(QString::fromLatin1("cid:")+QString::fromLatin1(KMime::uniqueString() + '@' + "kolab.resource.akonadi")); //Serialize the attachment as attachment with uri, referencing the created mime-part
    }
    return i;
}
/*
KABC::Addressee normalizeContact(const KABC::Addressee &a)
{
    KABC::Addressee addresee = a;
    Q_FOREACH (KCalCore::Attachment::Ptr attachment, addresee.photo()) {
        attachment->setUri(QString::fromLatin1("cid:")+QString::fromLatin1(KMime::uniqueString() + '@' + "kolab.resource.akonadi")); //Serialize the attachment as attachment with uri, referencing the created mime-part
    }
    return i;
}*/

QString getProductId(const QString &pId)
{
    if (pId.isEmpty()) {
        return LIBKOLAB_LIB_VERSION_STRING;
    }
    return pId+" "+LIBKOLAB_LIB_VERSION_STRING;
}

KMime::Message::Ptr KolabObjectWriter::writeEvent(const KCalCore::Event::Ptr &i, Version v, const QString &productId, const QString &tz)
{
    ErrorHandler::clearErrors();
    if (!i) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    Q_ASSERT(!i.isNull());
    if (v == KolabV3) {
        KCalCore::Event::Ptr ic = normalizeIncidence(i).dynamicCast<KCalCore::Event>();
        const Kolab::Event &incidence = Kolab::Conversion::fromKCalCore(*ic);
        const std::string &v3String = Kolab::writeEvent(incidence, std::string(getProductId(productId).toUtf8().constData()));
        ErrorHandler::handleLibkolabxmlErrors();
        return Mime::createMessage(ic, xCalMimeType(), eventKolabType(), QString::fromUtf8(v3String.c_str()).toUtf8(), true, getProductId(productId));
    }
    const QString &xml = KolabV2::Event::eventToXML(i, tz);
    return Mime::createMessage(i, eventKolabType(), eventKolabType(), xml.toUtf8(), false, getProductId(productId));
}

KMime::Message::Ptr KolabObjectWriter::writeTodo(const KCalCore::Todo::Ptr &i, Version v, const QString &productId, const QString &tz)
{
    ErrorHandler::clearErrors();
    if (!i) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    Q_ASSERT(!i.isNull());
    if (v == KolabV3) {
        KCalCore::Todo::Ptr ic = normalizeIncidence(i).dynamicCast<KCalCore::Todo>();
        const Kolab::Todo &incidence = Kolab::Conversion::fromKCalCore(*ic);
        const std::string &v3String = Kolab::writeTodo(incidence, Conversion::toStdString(getProductId(productId)));
        ErrorHandler::handleLibkolabxmlErrors();
        return Mime::createMessage(ic, xCalMimeType(), todoKolabType(), Conversion::fromStdString(v3String).toUtf8(), true, getProductId(productId));
    }
    const QString &xml = KolabV2::Task::taskToXML(i, tz);
    return Mime::createMessage(i, todoKolabType(), todoKolabType(), xml.toUtf8(), false, getProductId(productId));
}

KMime::Message::Ptr KolabObjectWriter::writeJournal(const KCalCore::Journal::Ptr &i, Version v, const QString &productId, const QString &tz)
{
    ErrorHandler::clearErrors();
    if (!i) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    Q_ASSERT(!i.isNull());
    if (v == KolabV3) {
        KCalCore::Journal::Ptr ic = normalizeIncidence(i).dynamicCast<KCalCore::Journal>();
        const Kolab::Journal &incidence = Kolab::Conversion::fromKCalCore(*ic);
        const std::string &v3String = Kolab::writeJournal(incidence, Conversion::toStdString(getProductId(productId)));
        ErrorHandler::handleLibkolabxmlErrors();
        return  Mime::createMessage(ic, xCalMimeType(), journalKolabType(), Conversion::fromStdString(v3String).toUtf8(), true, getProductId(productId));
    }
    const QString &xml = KolabV2::Journal::journalToXML(i, tz);
    return Mime::createMessage(i, journalKolabType(), journalKolabType(), xml.toUtf8(), false, getProductId(productId));
}

KMime::Message::Ptr KolabObjectWriter::writeIncidence(const KCalCore::Incidence::Ptr &i, Version v, const QString& productId, const QString& tz)
{
    if (!i) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    switch (i->type()) {
        case KCalCore::IncidenceBase::TypeEvent:
            return writeEvent(i.dynamicCast<KCalCore::Event>(),v,productId,tz);
        case KCalCore::IncidenceBase::TypeTodo:
            return writeTodo(i.dynamicCast<KCalCore::Todo>(),v,productId,tz);
        case KCalCore::IncidenceBase::TypeJournal:
            return writeJournal(i.dynamicCast<KCalCore::Journal>(),v,productId,tz);
        default:
            Critical() << "unknown incidence type";
    }
    return KMime::Message::Ptr();
}


KMime::Message::Ptr KolabObjectWriter::writeContact(const KABC::Addressee &addressee, Version v, const QString &productId)
{
    ErrorHandler::clearErrors();
    if (v == KolabV3) {
        const Kolab::Contact &contact = Kolab::Conversion::fromKABC(addressee);
        const std::string &v3String = Kolab::writeContact(contact, Conversion::toStdString(getProductId(productId)));
        ErrorHandler::handleLibkolabxmlErrors();
        return  Mime::createMessage(addressee, xCardMimeType(), contactKolabType(), Conversion::fromStdString(v3String).toUtf8(), true, getProductId(productId));
    }
    KolabV2::Contact contact(&addressee);
    return contactToKolabFormat(contact, getProductId(productId));
}

KMime::Message::Ptr KolabObjectWriter::writeDistlist(const KABC::ContactGroup &distlist, Version v, const QString &productId)
{
    ErrorHandler::clearErrors();
    if (v == KolabV3) {
        const Kolab::DistList &dist = Kolab::Conversion::fromKABC(distlist);
        const std::string &v3String = Kolab::writeDistlist(dist, Conversion::toStdString(getProductId(productId)));
        ErrorHandler::handleLibkolabxmlErrors();
        return  Mime::createMessage(Conversion::fromStdString(dist.uid()), xCardMimeType(), distlistKolabType(), Conversion::fromStdString(v3String).toUtf8(), true, getProductId(productId));
    }
    KolabV2::DistributionList d(&distlist);
    return distListToKolabFormat(d, getProductId(productId));
}

KMime::Message::Ptr KolabObjectWriter::writeNote(const KMime::Message::Ptr &note, Version v, const QString &productId)
{
    ErrorHandler::clearErrors();
    if (!note) {
        Critical() << "passed a null pointer";
        return KMime::Message::Ptr();
    }
    Q_ASSERT(note.get());
    if (v == KolabV3) {
        const Kolab::Note &n = Kolab::Conversion::fromNote(note);
        const std::string &v3String = Kolab::writeNote(n, Conversion::toStdString(getProductId(productId)));
        ErrorHandler::handleLibkolabxmlErrors();
        return  Mime::createMessage(Conversion::fromStdString(n.uid()), kolabMimeType(), noteKolabType(), Conversion::fromStdString(v3String).toUtf8(), true, getProductId(productId));
    }
    return noteToKolab(note, getProductId(productId));
}

KMime::Message::Ptr KolabObjectWriter::writeDictionary(const QStringList &entries, const QString& lang, Version v, const QString& productId)
{
    ErrorHandler::clearErrors();
    if (v != KolabV3) {
        Critical() << "only v3 implementation available";
    }

    Kolab::Dictionary dictionary(Conversion::toStdString(lang));
    std::vector <std::string> ent;
    foreach (const QString &e, entries) {
        ent.push_back(Conversion::toStdString(e));
    }
    dictionary.setEntries(ent);
    Kolab::Configuration configuration(dictionary); //TODO preserve creation/lastModified date
    const std::string &v3String = Kolab::writeConfiguration(configuration, Conversion::toStdString(getProductId(productId)));
    ErrorHandler::handleLibkolabxmlErrors();
    return  Mime::createMessage(Conversion::fromStdString(configuration.uid()), kolabMimeType(), dictKolabType(), Conversion::fromStdString(v3String).toUtf8(), true, getProductId(productId));
}

KMime::Message::Ptr KolabObjectWriter::writeFreebusy(const Freebusy &freebusy, Version v, const QString& productId)
{
    ErrorHandler::clearErrors();
    if (v != KolabV3) {
        Critical() << "only v3 implementation available";
    }
    const std::string &v3String = Kolab::writeFreebusy(freebusy, Conversion::toStdString(getProductId(productId)));
    ErrorHandler::handleLibkolabxmlErrors();
    return  Mime::createMessage(Conversion::fromStdString(freebusy.uid()), xCalMimeType(), freebusyKolabType(), Conversion::fromStdString(v3String).toUtf8(), true, getProductId(productId));
}






}; //Namespace


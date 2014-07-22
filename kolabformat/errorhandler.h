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


#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <kolab_export.h>

#include <QString>
#include <QList>

namespace Kolab {

/**
 * Kolab Error Handler
 * 
 * Errors are reported during an operation, but the operation might still succeed.
 * The error handler therefore contains all errors which occured during a single operation,
 * and must be cleared at the start of a new operation.
 * 
 * A user of the kolabobject classes should check ErrorHandler::error() after every operation.
 * 
 * all non-const functions are not for the user of this class and only exist for internal usage.
 * 
 * TODO: Hide everything which is not meant for the user from the interface.
 * FIXME: Use Threadlocal storage to make this threadsafe.
 */
class KOLAB_EXPORT ErrorHandler
{
public:
    enum Severity {
        Debug,
        Warning, //Warning, error could be corrected, object can be used without dataloss. This warning is also used if dataloss is acceptable because a feature is explicitly not supported.
        Error, //Potentially corrupt object, writing the object back could result in dataloss. (Object could still be used to display the data readonly).
        Critical //Critical error, produced object cannot be used and should be thrown away (writing back will result in dataloss).
    };
    
    struct Err {
        Err(Severity s, const QString &m, const QString &l): severity(s), message(m), location(l){};
        Severity severity;
        QString message;
        QString location;
    };
    
    static ErrorHandler &instance()
    {
        static ErrorHandler inst;
        return inst;
    }
    
    void addError(Severity s, const QString &message, const QString &location);
    const QList <Err> &getErrors() const;
    Severity error() const;
    QString errorMessage() const;
    void clear();
    
    /**
     * Check for errors during the libkolabxml reading/writing process and copy them into this error handler.
     */
    static void handleLibkolabxmlErrors();
    
    static void clearErrors()
    {
        ErrorHandler::instance().clear();
    }
    
    static bool errorOccured()
    {
        if (ErrorHandler::instance().error() >= Error) {
            return true;
        }
        return false;
    }
    
private:
    ErrorHandler():m_worstError(Debug) {};
    ErrorHandler(const ErrorHandler &);
    ErrorHandler & operator= (const ErrorHandler &);
    
    Severity m_worstError;
    QString m_worstErrorMessage;
    QList <Err> m_errorQueue;
};

void logMessage(const QString &,const QString &, int, ErrorHandler::Severity s);

#define LOG(message) logMessage(message,__FILE__, __LINE__, ErrorHandler::Debug);
#define WARNING(message) logMessage(message,__FILE__, __LINE__, ErrorHandler::Warning);
#define ERROR(message) logMessage(message,__FILE__, __LINE__, ErrorHandler::Error);
#define CRITICAL(message) logMessage(message,__FILE__, __LINE__, ErrorHandler::Critical);


/**
 * Drop in replacement for qWarning()
 * 
 * Note that this is not only for debug builds, its a fundamental part to detect errors.
 * 
 * TODO: It should be possible to replace this with a kWarning/qWarning error handler
 */
struct KolabLogger {
    bool isLast;
    QString m_message;
    QString m_location;
    ErrorHandler::Severity m_severity;
    KolabLogger(ErrorHandler::Severity s, int line, const QString &file): isLast(true), m_message(), m_location(file + "(" + QString::number(line)+")"), m_severity(s) {};
    KolabLogger(ErrorHandler::Severity s, const QString &message, const QString &location): isLast(true), m_message(message), m_location(location), m_severity(s) {};
    ~KolabLogger(){if (isLast) ErrorHandler::instance().addError(m_severity, m_message, m_location);}
    QString maybeSpace() const { if (!m_message.isEmpty()) return " "; return ""; }
    KolabLogger operator<<(const QString &message) {
        isLast = false;
        return KolabLogger(m_severity, m_message+maybeSpace()+message, m_location);
    }
    KolabLogger operator<<(const QByteArray &message) {
        isLast = false;
        return KolabLogger(m_severity, m_message+maybeSpace()+message, m_location);
    }
    KolabLogger operator<<(const int &n) {
        isLast = false;
        return KolabLogger(m_severity, m_message+maybeSpace()+QString::number(n), m_location);
    }
    KolabLogger operator<<(const double &n) {
        isLast = false;
        return KolabLogger(m_severity, m_message+maybeSpace()+QString::number(n), m_location);
    }
    inline KolabLogger operator<<(const char* t) {
        isLast = false;
        return KolabLogger(m_severity, m_message+maybeSpace()+QString::fromAscii(t), m_location);
    }
};
#define Debug() Kolab::KolabLogger(Kolab::ErrorHandler::Debug, __LINE__, __FILE__)
#define Warning() Kolab::KolabLogger(Kolab::ErrorHandler::Warning, __LINE__, __FILE__)
#define Error() Kolab::KolabLogger(Kolab::ErrorHandler::Error, __LINE__, __FILE__)
#define Critical() Kolab::KolabLogger(Kolab::ErrorHandler::Critical, __LINE__, __FILE__)

}
#endif // ERRORHANDLER_H

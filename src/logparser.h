#pragma once

#include "logentry.h"

#include <QVector>
#include <QString>

class LogParser {
public:
    struct Result {
        QVector<LogEntry> entries;
        QString errorString;
        bool ok = false;
    };

    static Result parseFile(const QString& filePath);
};

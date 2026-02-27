#pragma once

#include <QDateTime>
#include <QString>

struct LogEntry {
    QDateTime timestamp;      // invalid for meta lines
    QString level;            // DEBUG/INFO/WARN/ERROR/FATAL/META
    int severity = -1;        // 0..4, -1 unknown

    int pid = -1;
    QString tid;              // e.g. 0x7fc...

    QString category;         // [app.core]
    QString file;             // ../ISR/main.cpp or -
    QString lineText;         // "56" or "-" etc
    int lineNumber = -1;
    QString function;

    QString message;          // may contain \n
    QString raw;              // original line(s)

    bool isMeta = false;
};

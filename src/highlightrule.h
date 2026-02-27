#pragma once

#include "logentry.h"

#include <QColor>
#include <QJsonObject>
#include <QRegularExpression>

struct HighlightRule {
    bool enabled = true;
    QString name;

    // Filters (empty = any)
    QString level;            // e.g. "ERROR"
    QString tid;              // exact match
    QString pid;              // exact match as string

    QString categoryRegex;    // regex, empty = any
    QString fileRegex;        // regex, empty = any
    QString functionRegex;    // regex, empty = any
    QString messageRegex;     // regex, empty = any

    bool useForeground = true;
    QColor foreground = Qt::black;
    bool useBackground = false;
    QColor background = Qt::transparent;

    // Compiled regex (not serialized directly)
    QRegularExpression catRe;
    QRegularExpression fileRe;
    QRegularExpression funcRe;
    QRegularExpression msgRe;

    void compile();
    bool matches(const LogEntry& e) const;

    QJsonObject toJson() const;
    static HighlightRule fromJson(const QJsonObject& o);

    static QVector<HighlightRule> defaultRules();
};

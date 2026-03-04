#pragma once

#include "logentry.h"
#include "highlightrule.h"

#include <QAbstractTableModel>
#include <QVector>

class LogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColMid = 0,
        ColTime,
        ColLevel,
        ColPid,
        ColTid,
        ColCategory,
        ColFile,
        ColLine,
        ColFunction,
        ColMessage,
        ColCount
    };

    enum Roles {
        MessageIdRole = Qt::UserRole + 1,
        TimestampRole,
        LevelRole,
        SeverityRole,
        PidRole,
        TidRole,
        CategoryRole,
        FileRole,
        LineRole,
        FunctionRole,
        MessageRole,
        RawRole,
        IsMetaRole
    };

    explicit LogModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setEntries(QVector<LogEntry> entries);
    const LogEntry& entryAt(int row) const;
    QVector<LogEntry> entries() const { return m_entries; }

    void setHighlightRules(const QVector<HighlightRule>& rules);
    QVector<HighlightRule> highlightRules() const { return m_rules; }

    void clear();

private:
    int matchedRuleIndex(int row) const;
    void invalidateRuleCache();

private:
    QVector<LogEntry> m_entries;
    QVector<HighlightRule> m_rules;
    mutable QVector<int> m_ruleCache; // -2 unknown, -1 none, >=0 rule index
};

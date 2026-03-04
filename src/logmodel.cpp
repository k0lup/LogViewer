#include "logmodel.h"

#include <QBrush>

#include <utility>

LogModel::LogModel(QObject* parent)
    : QAbstractTableModel(parent) {
}

int LogModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_entries.size();
}

int LogModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation == Qt::Horizontal) {
        switch (section) {
            case ColMid:      return QStringLiteral("MID");
            case ColTime:     return QStringLiteral("Время");
            case ColLevel:    return QStringLiteral("Тип");
            case ColPid:      return QStringLiteral("PID");
            case ColTid:      return QStringLiteral("TID");
            case ColCategory: return QStringLiteral("Категория");
            case ColFile:     return QStringLiteral("Файл");
            case ColLine:     return QStringLiteral("Строка");
            case ColFunction: return QStringLiteral("Функция");
            case ColMessage:  return QStringLiteral("Сообщение");
            default: break;
        }
    }
    return QVariant();
}

static QString tsToString(const QDateTime& dt) {
    if (!dt.isValid()) return QString();
    return dt.toString("yyyy-MM-dd HH:mm:ss.zzz");
}

int LogModel::matchedRuleIndex(int row) const {
    if (row < 0 || row >= m_entries.size()) return -1;

    if (m_ruleCache.size() != m_entries.size()) {
        m_ruleCache.resize(m_entries.size());
        for (int i = 0; i < m_ruleCache.size(); ++i) m_ruleCache[i] = -2;
    }

    int& cached = m_ruleCache[row];
    if (cached != -2) return cached;

    const LogEntry& e = m_entries.at(row);
    for (int i = 0; i < m_rules.size(); ++i) {
        if (m_rules[i].matches(e)) {
            cached = i;
            return cached;
        }
    }

    cached = -1;
    return cached;
}

void LogModel::invalidateRuleCache() {
    m_ruleCache.clear();
}

QVariant LogModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    const int row = index.row();
    const int col = index.column();
    if (row < 0 || row >= m_entries.size()) return QVariant();

    const LogEntry& e = m_entries.at(row);

    // Custom roles for filtering/sorting
    switch (role) {
        case MessageIdRole: return e.messageId;
        case TimestampRole: return e.timestamp;
        case LevelRole:     return e.level;
        case SeverityRole:  return e.severity;
        case PidRole:       return e.pid;
        case TidRole:       return e.tid;
        case CategoryRole:  return e.category;
        case FileRole:      return e.file;
        case LineRole:      return e.lineText;
        case FunctionRole:  return e.function;
        case MessageRole:   return e.message;
        case RawRole:       return e.raw;
        case IsMetaRole:    return e.isMeta;
        default: break;
    }

    if (role == Qt::DisplayRole) {
        switch (col) {
            case ColMid:      return (e.messageId >= 0 ? QString::number(e.messageId) : QString());
            case ColTime:     return tsToString(e.timestamp);
            case ColLevel:    return e.level;
            case ColPid:      return (e.pid >= 0 ? QString::number(e.pid) : QString());
            case ColTid:      return e.tid;
            case ColCategory: return e.category;
            case ColFile:     return e.file;
            case ColLine:     return e.lineText;
            case ColFunction: return e.function;
            case ColMessage:  return e.message;
            default: break;
        }
        return QVariant();
    }

    if (role == Qt::ToolTipRole) {
        // Show full entry
        QString tip;
        if (e.timestamp.isValid()) {
            tip += tsToString(e.timestamp) + " " + e.level + "\n";
        } else {
            tip += e.level + "\n";
        }
        if (e.pid >= 0) tip += QStringLiteral("pid=%1 ").arg(e.pid);
        if (!e.tid.isEmpty()) tip += QStringLiteral("tid=%1\n").arg(e.tid);
        if (!e.category.isEmpty()) tip += QStringLiteral("[%1]\n").arg(e.category);
        if (!e.file.isEmpty()) tip += QStringLiteral("%1:%2\n").arg(e.file, e.lineText);
        if (!e.function.isEmpty()) tip += e.function + "\n";
        tip += "\n" + e.message;
        return tip;
    }

    if (role == Qt::BackgroundRole || role == Qt::ForegroundRole) {
        const int ruleIdx = matchedRuleIndex(row);
        if (ruleIdx >= 0 && ruleIdx < m_rules.size()) {
            const HighlightRule& r = m_rules.at(ruleIdx);
            if (role == Qt::BackgroundRole && r.useBackground) {
                return QBrush(r.background);
            }
            if (role == Qt::ForegroundRole && r.useForeground) {
                return QBrush(r.foreground);
            }
        }
        return QVariant();
    }

    if (role == Qt::TextAlignmentRole) {
        if (col == ColPid || col == ColLine) {
            return Qt::AlignRight + Qt::AlignVCenter;
        }
        return Qt::AlignLeft + Qt::AlignVCenter;
    }

    return QVariant();
}

void LogModel::setEntries(QVector<LogEntry> entries) {
    beginResetModel();
    m_entries = std::move(entries);
    invalidateRuleCache();
    endResetModel();
}

const LogEntry& LogModel::entryAt(int row) const {
    return m_entries.at(row);
}

void LogModel::setHighlightRules(const QVector<HighlightRule>& rules) {
    m_rules = rules;
    // Ensure regex are compiled (dialog should compile too, but for safety)
    for (HighlightRule& r : m_rules) r.compile();
    invalidateRuleCache();
    if (!m_entries.isEmpty()) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1), {Qt::BackgroundRole, Qt::ForegroundRole});
    }
}

void LogModel::clear() {
    beginResetModel();
    m_entries.clear();
    invalidateRuleCache();
    endResetModel();
}

#include "logfilterproxymodel.h"
#include "logmodel.h"

#include <QDateTime>
#include <QRegExp>

static QSet<int> parsePidList(const QString& text) {
    QSet<int> out;
    const QString t = text;
    const QStringList parts = t.split(QRegExp("[\\s,;]+"), Qt::SkipEmptyParts);
    for (const QString& p : parts) {
        bool ok = false;
        const int v = p.toInt(&ok);
        if (ok) out.insert(v);
    }
    return out;
}

LogFilterProxyModel::LogFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void LogFilterProxyModel::setAllowedLevels(const QSet<QString>& levels) {
    m_levels = levels;
    invalidateFilter();
}

void LogFilterProxyModel::setAllowedTids(const QSet<QString>& tids) {
    m_tids = tids;
    invalidateFilter();
}

void LogFilterProxyModel::setAllowedCategories(const QSet<QString>& cats) {
    m_categories = cats;
    invalidateFilter();
}

void LogFilterProxyModel::setPidFilterText(const QString& text) {
    const QString t = text.trimmed();
    if (t.isEmpty()) {
        m_pidActive = false;
        m_pids.clear();
    } else {
        m_pidActive = true;
        m_pids = parsePidList(t);
    }
    invalidateFilter();
}

void LogFilterProxyModel::setMessageContains(const QString& text) {
    m_msg = text;
    invalidateFilter();
}

void LogFilterProxyModel::setFileContains(const QString& text) {
    m_file = text;
    invalidateFilter();
}

void LogFilterProxyModel::setFunctionContains(const QString& text) {
    m_func = text;
    invalidateFilter();
}

void LogFilterProxyModel::setCategoryContains(const QString& text) {
    m_catContains = text;
    invalidateFilter();
}

void LogFilterProxyModel::setShowMeta(bool show) {
    m_showMeta = show;
    invalidateFilter();
}

bool LogFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
    const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);

    const bool isMeta = sourceModel()->data(idx, LogModel::IsMetaRole).toBool();
    if (!m_showMeta && isMeta) return false;

    const QString level = sourceModel()->data(idx, LogModel::LevelRole).toString();
    if (!m_levels.contains(level)) return false;

    const QString tid = sourceModel()->data(idx, LogModel::TidRole).toString();
    if (!m_tids.contains(tid)) return false;

    const QString cat = sourceModel()->data(idx, LogModel::CategoryRole).toString();
    if (!m_categories.contains(cat)) return false;

    if (m_pidActive) {
        const int pid = sourceModel()->data(idx, LogModel::PidRole).toInt();
        if (!m_pids.contains(pid)) return false;
    }

    if (!m_catContains.trimmed().isEmpty()) {
        if (!cat.contains(m_catContains, Qt::CaseInsensitive)) return false;
    }

    if (!m_file.trimmed().isEmpty()) {
        const QString file = sourceModel()->data(idx, LogModel::FileRole).toString();
        if (!file.contains(m_file, Qt::CaseInsensitive)) return false;
    }

    if (!m_func.trimmed().isEmpty()) {
        const QString func = sourceModel()->data(idx, LogModel::FunctionRole).toString();
        if (!func.contains(m_func, Qt::CaseInsensitive)) return false;
    }

    if (!m_msg.trimmed().isEmpty()) {
        const QString msg = sourceModel()->data(idx, LogModel::MessageRole).toString();
        if (!msg.contains(m_msg, Qt::CaseInsensitive)) return false;
    }

    return true;
}

bool LogFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const {
    const int col = source_left.column();

    const qint64 leftMid = sourceModel()->data(source_left, LogModel::MessageIdRole).toLongLong();
    const qint64 rightMid = sourceModel()->data(source_right, LogModel::MessageIdRole).toLongLong();

    if (col == LogModel::ColMid) {
        if (leftMid != rightMid) return leftMid < rightMid;
        return source_left.row() < source_right.row();
    }

    if (col == LogModel::ColTime) {
        const QDateTime a = sourceModel()->data(source_left, LogModel::TimestampRole).toDateTime();
        const QDateTime b = sourceModel()->data(source_right, LogModel::TimestampRole).toDateTime();

        const bool av = a.isValid();
        const bool bv = b.isValid();

        if (av && bv) {
            if (a != b) return a < b;
            return leftMid < rightMid;
        }
        if (av && !bv) return false; // valid after meta
        if (!av && bv) return true;

        if (leftMid != rightMid) return leftMid < rightMid;
        return source_left.row() < source_right.row();
    }

    if (col == LogModel::ColLevel) {
        const int sa = sourceModel()->data(source_left, LogModel::SeverityRole).toInt();
        const int sb = sourceModel()->data(source_right, LogModel::SeverityRole).toInt();
        if (sa >= 0 && sb >= 0 && sa != sb) return sa < sb;
    }

    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

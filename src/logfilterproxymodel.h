#pragma once

#include <QSortFilterProxyModel>
#include <QSet>

class LogFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit LogFilterProxyModel(QObject* parent = nullptr);

    void setAllowedLevels(const QSet<QString>& levels);      // empty => show none
    void setAllowedTids(const QSet<QString>& tids);          // empty => show none
    void setAllowedCategories(const QSet<QString>& cats);    // empty => show none

    // PID filter is optional; empty text disables the pid filter.
    void setPidFilterText(const QString& text);

    void setMessageContains(const QString& text);
    void setFileContains(const QString& text);
    void setFunctionContains(const QString& text);
    void setCategoryContains(const QString& text);

    void setShowMeta(bool show);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
    QSet<QString> m_levels;
    QSet<QString> m_tids;
    QSet<QString> m_categories;

    bool m_pidActive = false;
    QSet<int> m_pids;

    QString m_msg;
    QString m_file;
    QString m_func;
    QString m_catContains;

    bool m_showMeta = true;
};

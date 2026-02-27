#pragma once

#include "highlightrule.h"

#include <QMainWindow>

class LogModel;
class LogFilterProxyModel;

class QTableView;
class QPlainTextEdit;
class QListWidget;
class QLineEdit;
class QLabel;
class QCheckBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void openFile();
    void reloadFile();
    void sortNewest();
    void sortOldest();
    void clearAllFilters();
    void showHighlightRules();
    void copySelectedLine();

    void updateFiltersFromUI();
    void updateCounts();
    void onCurrentChanged();

private:
    void setupUi();
    void createMenus();
    void createFilterDock();

    void loadSettings();
    void saveSettings();

    void loadLogFile(const QString& path);
    void rebuildFilterLists();

    QSet<QString> checkedItemsToSet(QListWidget* w) const;
    void setAllChecked(QListWidget* w, bool checked);

private:
    QString m_currentFile;

    LogModel* m_model = nullptr;
    LogFilterProxyModel* m_proxy = nullptr;

    QTableView* m_view = nullptr;
    QPlainTextEdit* m_details = nullptr;

    // filters
    QListWidget* m_levels = nullptr;
    QListWidget* m_tids = nullptr;
    QListWidget* m_categories = nullptr;

    QLineEdit* m_pidEdit = nullptr;
    QLineEdit* m_catContainsEdit = nullptr;
    QLineEdit* m_fileEdit = nullptr;
    QLineEdit* m_funcEdit = nullptr;
    QLineEdit* m_msgEdit = nullptr;

    QCheckBox* m_showMeta = nullptr;

    QLabel* m_countLabel = nullptr;

    QVector<HighlightRule> m_rules;
};

#include "highlightrule.h"

#include <QJsonValue>

static QRegularExpression makeRe(const QString& pattern) {
    if (pattern.trimmed().isEmpty()) return QRegularExpression();
    QRegularExpression re(pattern);
    re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    return re;
}

void HighlightRule::compile() {
    catRe  = makeRe(categoryRegex);
    fileRe = makeRe(fileRegex);
    funcRe = makeRe(functionRegex);
    msgRe  = makeRe(messageRegex);
}

bool HighlightRule::matches(const LogEntry& e) const {
    if (!enabled) return false;

    if (!level.trimmed().isEmpty()) {
        if (e.level.compare(level.trimmed(), Qt::CaseInsensitive) != 0) return false;
    }

    if (!tid.trimmed().isEmpty()) {
        if (e.tid != tid.trimmed()) return false;
    }

    if (!pid.trimmed().isEmpty()) {
        if (QString::number(e.pid) != pid.trimmed()) return false;
    }

    if (!categoryRegex.trimmed().isEmpty()) {
        if (!catRe.isValid() || !catRe.match(e.category).hasMatch()) return false;
    }

    if (!fileRegex.trimmed().isEmpty()) {
        if (!fileRe.isValid() || !fileRe.match(e.file).hasMatch()) return false;
    }

    if (!functionRegex.trimmed().isEmpty()) {
        if (!funcRe.isValid() || !funcRe.match(e.function).hasMatch()) return false;
    }

    if (!messageRegex.trimmed().isEmpty()) {
        if (!msgRe.isValid() || !msgRe.match(e.message).hasMatch()) return false;
    }

    return true;
}

QJsonObject HighlightRule::toJson() const {
    QJsonObject o;
    o["enabled"] = enabled;
    o["name"] = name;
    o["level"] = level;
    o["tid"] = tid;
    o["pid"] = pid;
    o["categoryRegex"] = categoryRegex;
    o["fileRegex"] = fileRegex;
    o["functionRegex"] = functionRegex;
    o["messageRegex"] = messageRegex;
    o["useForeground"] = useForeground;
    o["foreground"] = foreground.name(QColor::HexArgb);
    o["useBackground"] = useBackground;
    o["background"] = background.name(QColor::HexArgb);
    return o;
}

HighlightRule HighlightRule::fromJson(const QJsonObject& o) {
    HighlightRule r;
    r.enabled = o.value("enabled").toBool(true);
    r.name = o.value("name").toString();
    r.level = o.value("level").toString();
    r.tid = o.value("tid").toString();
    r.pid = o.value("pid").toString();
    r.categoryRegex = o.value("categoryRegex").toString();
    r.fileRegex = o.value("fileRegex").toString();
    r.functionRegex = o.value("functionRegex").toString();
    r.messageRegex = o.value("messageRegex").toString();
    r.useForeground = o.value("useForeground").toBool(true);
    r.foreground = QColor(o.value("foreground").toString());
    if (!r.foreground.isValid()) r.foreground = Qt::black;
    r.useBackground = o.value("useBackground").toBool(false);
    r.background = QColor(o.value("background").toString());
    if (!r.background.isValid()) r.background = Qt::transparent;
    r.compile();
    return r;
}

QVector<HighlightRule> HighlightRule::defaultRules() {
    QVector<HighlightRule> rules;

    {
        HighlightRule r;
        r.name = QStringLiteral("FATAL");
        r.level = QStringLiteral("FATAL");
        r.useForeground = true;
        r.foreground = QColor(255, 255, 255);
        r.useBackground = true;
        r.background = QColor(150, 0, 150);
        r.compile();
        rules.push_back(r);
    }
    {
        HighlightRule r;
        r.name = QStringLiteral("ERROR");
        r.level = QStringLiteral("ERROR");
        r.useForeground = true;
        r.foreground = QColor(120, 0, 0);
        r.useBackground = true;
        r.background = QColor(255, 220, 220);
        r.compile();
        rules.push_back(r);
    }
    {
        HighlightRule r;
        r.name = QStringLiteral("WARN");
        r.level = QStringLiteral("WARN");
        r.useForeground = true;
        r.foreground = QColor(90, 60, 0);
        r.useBackground = true;
        r.background = QColor(255, 245, 200);
        r.compile();
        rules.push_back(r);
    }
    {
        HighlightRule r;
        r.name = QStringLiteral("DEBUG");
        r.level = QStringLiteral("DEBUG");
        r.useForeground = true;
        r.foreground = QColor(110, 110, 110);
        r.useBackground = false;
        r.compile();
        rules.push_back(r);
    }
    {
        HighlightRule r;
        r.name = QStringLiteral("META");
        r.level = QStringLiteral("META");
        r.useForeground = true;
        r.foreground = QColor(70, 70, 70);
        r.useBackground = true;
        r.background = QColor(235, 235, 235);
        r.compile();
        rules.push_back(r);
    }

    return rules;
}

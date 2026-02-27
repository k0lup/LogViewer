#include "logparser.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#include <utility>

static int severityFromLevel(const QString& levelUpper) {
    if (levelUpper == "DEBUG") return 0;
    if (levelUpper == "INFO")  return 1;
    if (levelUpper == "WARN")  return 2;
    if (levelUpper == "ERROR") return 3;
    if (levelUpper == "FATAL") return 4;
    if (levelUpper == "META")  return -1;
    return -1;
}

LogParser::Result LogParser::parseFile(const QString& filePath) {
    Result r;

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        r.ok = false;
        r.errorString = QStringLiteral("Не удалось открыть файл: %1").arg(f.errorString());
        return r;
    }

    // Example line:
    // 2026-02-26 17:07:41.106 INFO  pid=40358 tid=0x7fc220958c00 [app.core] ../ISR/main.cpp:56 void setupLogging()  message
    // Continuation lines are written as the same prefix + "| " + text.
    static const QRegularExpression re(
        QStringLiteral(R"(^\s*(?<ts>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\s+(?<lvl>[A-Za-z]+)\s+pid=(?<pid>\d+)\s+tid=(?<tid>\S+)\s+\[(?<cat>[^\]]*)\]\s+(?<file>.+):(?<line>[^ ]+)\s+(?<func>.*?)\s{2,}(?<msg>.*)$)")
    );

    QTextStream ts(&f);
    ts.setCodec("UTF-8");

    QVector<LogEntry> out;
    out.reserve(4096);

    bool hasCurrent = false;

    while (!ts.atEnd()) {
        const QString line = ts.readLine();
        const QRegularExpressionMatch m = re.match(line);

        if (m.hasMatch()) {
            LogEntry e;
            const QString tsStr = m.captured("ts");
            e.timestamp = QDateTime::fromString(tsStr, "yyyy-MM-dd HH:mm:ss.zzz");
            e.level = m.captured("lvl").trimmed().toUpper();
            e.severity = severityFromLevel(e.level);
            e.pid = m.captured("pid").toInt();
            e.tid = m.captured("tid");
            e.category = m.captured("cat");
            e.file = m.captured("file");
            e.lineText = m.captured("line");
            bool okNum = false;
            e.lineNumber = e.lineText.toInt(&okNum);
            if (!okNum) e.lineNumber = -1;
            e.function = m.captured("func");
            e.message = m.captured("msg");
            e.raw = line;
            e.isMeta = false;

            // Continuation format: "| " at message start
            if (e.message.startsWith(QStringLiteral("| "))) {
                const QString cont = e.message.mid(2);
                if (hasCurrent && !out.isEmpty() && !out.last().isMeta) {
                    out.last().message += QStringLiteral("\n") + cont;
                    out.last().raw += QStringLiteral("\n") + line;
                } else {
                    // No previous entry - treat as separate
                    e.message = cont;
                    out.push_back(std::move(e));
                    hasCurrent = true;
                }
            } else {
                out.push_back(std::move(e));
                hasCurrent = true;
            }
            continue;
        }

        // Non-matching lines: either metadata blocks or continuation of previous message.
        const QString trimmed = line.trimmed();
        const bool isSeparator = trimmed.startsWith(QStringLiteral("===="))
            || trimmed.startsWith(QStringLiteral("----"))
            || trimmed.startsWith(QStringLiteral("Triggered by"));

        if (hasCurrent && !out.isEmpty() && !out.last().isMeta && !isSeparator) {
            if (!out.last().message.isEmpty()) out.last().message += QStringLiteral("\n");
            out.last().message += line;
            out.last().raw += QStringLiteral("\n") + line;
        } else {
            LogEntry meta;
            meta.isMeta = true;
            meta.level = QStringLiteral("META");
            meta.severity = -1;
            meta.message = line;
            meta.raw = line;
            out.push_back(std::move(meta));
            hasCurrent = true;
        }
    }

    r.entries = std::move(out);
    r.ok = true;
    return r;
}

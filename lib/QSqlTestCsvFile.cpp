/**
 * TODO:
 *     + If final column is empty it causes crash
 */
#include "QSqlTestCsvFile.h"
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTextStream>

#define CHECK_TYPE(STR, TYPE)                                                                      \
    {                                                                                              \
        bool ret;                                                                                  \
        STR.to##TYPE(&ret);                                                                        \
        if (ret) {                                                                                 \
            return QVariant(STR.to##TYPE());                                                       \
        }                                                                                          \
    }

// https://doc.qt.io/qt-5/sql-types.html
static QVariant convertType(const QString &str)
{
    CHECK_TYPE(str, Int);

    /*
    if (str.size() == 0) {
        return QVariant(QVariant::String);
    }
    */
    return str;
}

static QVariantList parseLine(const QString &string)
{
    bool isQuote = false;
    QString value;
    QVariantList line;

    for (int i = 0; i < string.size(); i++) {
        QChar current = string.at(i);

        if (!isQuote) {
            if (i == string.size() - 1) {
                // end-of-line
                line.append(convertType(value + current));
                value.clear();
            } else if (current == ',') {
                // comma
                line.append(convertType(value));
                value.clear();
            } else if (current == '"') {
                // double quote
                isQuote = true;
            } else {
                // character
                value += current;
            }
        } else {
            if (current == '"') {
                // double quote
                int index = (i + 1 < string.size()) ? i + 1 : string.size();
                QChar next = string.at(index);
                if (next == '"') {
                    value += '"';
                    i++;
                } else {
                    isQuote = false;
                }
            } else {
                // other
                value += current;
            }
        }
    }

    return line;
}

struct QSqlTestCsvFilePrivate
{
    QSqlTestCsvFilePrivate()
        : fileName()
        , lastError("No Error")
        , data()
        , headerData()
    {
    }

    QString fileName;
    QString lastError;
    QList<QVariantList> data;
    QVariantList headerData;
};

QSqlTestCsvFile::QSqlTestCsvFile(QObject *parent)
    : QAbstractTableModel(parent)
    , d(new QSqlTestCsvFilePrivate)
{
}

bool QSqlTestCsvFile::load(const QString &fileName)
{
    if (!QFile::exists(fileName)) {
        d->lastError = QString::fromLatin1("%1 does not exist").arg(fileName);
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        d->lastError = QString::fromLatin1("Error opening %1").arg(fileName);
        return false;
    }

    d->fileName = fileName;
    d->headerData.clear();
    d->data.clear();

    /*
    https://github.com/python/cpython/blob/master/Lib/csv.py#L383
    is_header = not any(cell.isdigit() for cell in csv_table[0])
    Given a CSV table csv_table, grab the top (zeroth) row. Iterate through the cells and check if
    they contain any pure digit strings. If so, it's not a header. has_header =
    ''.join(next(some_csv_reader)).isalpha() sim = (df1.dtypes.values == df2.dtypes.values).mean()
    https://stackoverflow.com/questions/2670515/autodetect-presence-of-csv-headers-in-a-file
    */

    QTextStream stream(&file);

    int lineNumber = 0;
    while (!stream.atEnd()) {
        lineNumber++;
        QString line = stream.readLine();
        if (line.length() == 0) {
            continue;
        }

        if (lineNumber == 1) {
            auto headerData = line.split(',').replaceInStrings(" ", "");
            headerData.removeAll(QString("")); // Remove trailing commas.
            for (const auto header : headerData) {
                d->headerData.append(QVariant(header.toUpper()));
            }

            emit headerDataChanged(Qt::Horizontal, 0, d->headerData.size());
            continue;
        }

        d->data.append(parseLine(line));
    }

    const auto width = d->headerData.size();
    const auto height = d->data.size();

    emit dataChanged(index(0, 0), index(height - 1, width - 1));

    return true;
}

QString QSqlTestCsvFile::fileName() const
{
    return d->fileName;
}

QString QSqlTestCsvFile::lastError() const
{
    return d->lastError;
}

int QSqlTestCsvFile::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d->data.size();
}

int QSqlTestCsvFile::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return d->headerData.size();
}

QVariant QSqlTestCsvFile::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();
    const auto column = index.column();
    const auto width = qMax(d->headerData.size(), 1);
    const auto height = qMax(d->data.size(), 1);

    if (role == Qt::DisplayRole && index.isValid() && row < height && column < width) {
        return d->data.at(row).at(column);
    }
    return QVariant();
}

QVariant QSqlTestCsvFile::headerData(int section, Qt::Orientation orientation, int role) const
{
    const auto size = d->headerData.size();
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section < size) {
        return d->headerData.at(section);
    }
    return QVariant();
}

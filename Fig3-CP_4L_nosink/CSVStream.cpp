/**************************************************************************/
/*  This file is part of LithoGraphX.                                     */
/*                                                                        */
/*  LithoGraphX is free software: you can redistribute it and/or modify   */
/*  it under the terms of the GNU General Public License as published by  */
/*  the Free Software Foundation, either version 3 of the License, or     */
/*  (at your option) any later version.                                   */
/*                                                                        */
/*  LithoGraphX is distributed in the hope that it will be useful,        */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*  GNU General Public License for more details.                          */
/*                                                                        */
/*  You should have received a copy of the GNU General Public License     */
/*  along with LithoGraphX.  If not, see <http://www.gnu.org/licenses/>.  */
/**************************************************************************/

#include "CSVStream.hpp"
#if QT_VERSION >= 0x050000
#  include <QRegularExpression>
#else
#  include <QRegExp>
#endif

namespace {
#if QT_VERSION >= 0x050000
// Regular expression for a whole field
QRegularExpression fieldRe(R"eos((?<=^|[,;])\s*([^"][^,;\n]*?|"(?:[^"]*?|"")*?")\s*(?=$|\n|[;,]))eos");
QRegularExpression escapedChars(R"([",[:space:]])");
#else
QRegExp fieldRe(R"eos((?:^|[,;])\s*([^" \n\t][^,;\n]*|"(?:[^"]*|"")*")\s*(?=$|\n|[;,]))eos", Qt::CaseSensitive, QRegExp::RegExp2);
QRegExp escapedChars(R"([",\s])");
bool initializeRe() {
    fieldRe.setMinimal(true);
    return true;
}
bool initializedRed = initializeRe();
#endif
}

QString CSVStream::shield(const QString& s)
{
  if(s.contains(escapedChars)) {
    QString s1 = s;
    s1.replace('"', "\"\"");
    return QString("\"%1\"").arg(s1);
  }
  return s;
}

QString CSVStream::unshield(const QString& s)
{
  if(s.size() > 1 and s[0] == '"' and s[s.size()-1] == '"') {
    QString s1 = s.mid(1, s.size()-2);
    s1.replace("\"\"", "\"");
    return s1;
  }
  return s;
}

CSVStream::CSVStream()
    : _partialLine(false)
{ }

CSVStream::CSVStream(QIODevice* file)
  : _ts(file)
  , _partialLine(false)
{ }

CSVStream::CSVStream(FILE *fileHandle, QIODevice::OpenMode openMode)
    : _ts(fileHandle, openMode)
    , _partialLine(false)
{ }

CSVStream::CSVStream(QString *string, QIODevice::OpenMode openMode)
    : _ts(string, openMode)
    , _partialLine(false)
{ }

CSVStream::CSVStream(QByteArray *array, QIODevice::OpenMode openMode)
    : _ts(array, openMode)
    , _partialLine(false)
{ }

CSVStream::CSVStream(const QByteArray& array, QIODevice::OpenMode openMode)
    : _ts(array, openMode)
    , _partialLine(false)
{ }

CSVStream::~CSVStream()
{ }

CSVStream& CSVStream::operator<<(const QStringList& line)
{
  QStringList shielded;
  for(const QString& s: line)
    shielded << shield(s);
  _ts << shielded.join(",") << "\n";
  return *this;
}

CSVStream& CSVStream::operator<<(const QString& item)
{
  if(_partialLine)
    _ts << ",";
  _ts << shield(item);
  _partialLine = true;
  return *this;
}

CSVStream& CSVStream::operator<<(CSVStream& (*pf)(CSVStream&))
{
  return (*pf)(*this);
}

CSVStream& CSVStream::endOfLine()
{
  _partialLine = false;
  _ts << "\n";
  return *this;
}

CSVStream& CSVStream::operator>>(QStringList& fields)
{
  fields.clear();
  QString line;
  int idx = 0;
  while(not _ts.atEnd()) {
    if(not line.isEmpty())
      line.append('\n');
    line += _ts.readLine();
    if(line.isEmpty())
      return *this; // there are no fields there
    while(true) {
#if QT_VERSION >= 0x050000
      auto m = fieldRe.match(line, idx);
      if(m.hasMatch() and m.capturedStart() == idx) {
        fields << unshield(m.captured(1));
        idx = m.capturedEnd();
#else
      if(idx > 0) --idx;
      auto new_idx = fieldRe.indexIn(line, idx);
      if(new_idx == idx) {
          fields << unshield(fieldRe.cap(1));
          idx += fieldRe.matchedLength();
#endif
        if(idx == line.size())
          return *this;
        ++idx;
      } else {
        if(line[idx] != '"') {
          fields.clear();
          setStatus(QTextStream::ReadCorruptData);
          return *this;
        }
        break;
      }
    }
  }
  if(not line.isEmpty()) {
    fields.clear(); // last field is invalid somehow
    setStatus(QTextStream::ReadCorruptData);
  }
  return *this;
}

CSVStream& eol(CSVStream& s)
{
  return s.endOfLine();
}

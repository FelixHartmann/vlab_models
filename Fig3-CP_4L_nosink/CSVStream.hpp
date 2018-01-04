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

#ifndef CSVSTREAM_HPP
#define CSVSTREAM_HPP

#include <QBuffer>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QTextStream>

/**
 * \class CSVStream
 *
 * Create a stream to read/write CSV files
 *
 * How to use it to read a file:
 *
 * \code
 *   QFile file("filename.csv");
 *   if(not file.open(QIODevice::ReadOnly))
 *      // error !
 *   CSVStream csv(&file);
 *   while(not csv.atEnd()) {
 *      QStringList line;
 *      csv >> line;
 *      // Here 'line' will contain all the fields on the current line
 *   }
 * \encode
 *
 * And how to use it to write a file:
 *
 * \code
 *   QFile file("filename.csv");
 *   if(not file.open(QIODevice::WriteOnly))
 *      // error !
 *   CSVStream csv(&file);
 *   // First we can write a whole line:
 *   auto header = QStringList() << "Col 1" << "Col 2" << "Col 3";
 *   csv << header;
 *   // But we can also write value per value
 *   // In that case, lines should end with "eol"
 *   csv << 1 << 2 << 3 << eol;
 *   csv << 3 << 2 << 1 << eol;
 * \endcode
 *
 * Note that the whole-line writing is probably faster as the element per
 * element writing relies on string streams.
 */
class CSVStream
{
  QTextStream _ts;
  bool _partialLine;

public:
  /**
   * Creates an empty stream
   *
   * Before you can use it, you need to set the device using CSVStream::setDevice
   */
  CSVStream();

  /// Build a CSV stream from an IO device
  CSVStream(QIODevice* file);

  /**
   * Construct a CSVStream from a C FILE.
   *
   * \note Simply call the corresponding QTextStream constructor
   * \see QTextStream::QTextStream(FILE *fileHandle, QIODevice::OpenMode openMode = QIODevice::ReadWrite)
   */
  CSVStream(FILE *fileHandle, QIODevice::OpenMode openMode = QIODevice::ReadWrite);

  /**
   * Construct a CSVStream using a string as buffer
   *
   * \note Simply call the corresponding QTextStream constructor
   * \see QTextStream::QTextStream(QString *string, QIODevice::OpenMode openMode = QIODevice::ReadWrite)
   */
  CSVStream(QString *string, QIODevice::OpenMode openMode = QIODevice::ReadWrite);

  /**
   * Construct a CSVStream using a string as buffer
   *
   * \note Simply call the corresponding QTextStream constructor
   * \see QTextStream::QTextStream(QByteArray *array, QIODevice::OpenMode openMode = QIODevice::ReadWrite)
   */
  CSVStream(QByteArray *array, QIODevice::OpenMode openMode = QIODevice::ReadWrite);

  /**
   * Construct a CSVStream using a string as buffer
   *
   * \note Simply call the corresponding QTextStream constructor
   * \see QTextStream::QTextStream(const QByteArray &array, QIODevice::OpenMode openMode = QIODevice::ReadOnly)
   */
  CSVStream(const QByteArray &array, QIODevice::OpenMode openMode = QIODevice::ReadOnly);

  /// Destroy the stream
  ~CSVStream();

  /**
   * Utility function to shield a string for insertion into a CSV file
   *
   * \note You don't need to call it to write in this stream.
   */
  static QString shield(const QString& s);

  /**
   * Utility function to remove shielding from string
   *
   * \note You don't need to call it to read from this stream.
   */
  static QString unshield(const QString& s);

  /// Write a line of comma-separated values
  CSVStream& operator<<(const QStringList& line);
  /// Read the next line of comma-separated values
  CSVStream& operator>>(QStringList& line);

  /// Write a single entry
  CSVStream& operator<<(const QString& item);

  /// Traditional flux operator for flux modifiers
  CSVStream& operator<<(CSVStream& (*pf)(CSVStream&));

  /// Write a typed object using stream on a buffer as intermediate.
  template <typename T>
  CSVStream& operator<<(const T& item)
  {
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    {
      QTextStream ts(&buffer);
      ts << item;
    }
    return *this << QString::fromUtf8(buffer.data());
  }

  /// End the current line
  CSVStream& endOfLine();

  //@{
  ///\name Method forwarded directly to the underlying QTextStream object
  bool atEnd() const { return _ts.atEnd(); }
  QTextCodec* codec() const { return _ts.codec(); }
  void setCodec(QTextCodec* codec) { _ts.setCodec(codec); }
  QIODevice* device() const { return _ts.device(); }
  void setDevice(QIODevice* device) { _ts.setDevice(device); }
  void flush();
  qint64 pos() const { return _ts.pos(); }
  bool seek(qint64 p) { return _ts.seek(p); }
  void setAutoDetectUnicode(bool enabled) { _ts.setAutoDetectUnicode(enabled); }
  bool autoDetectUnicode() const { return _ts.autoDetectUnicode(); }
  QTextStream::Status status() const { return _ts.status(); }
  void setStatus(QTextStream::Status status) { _ts.setStatus(status); }
  void resetStatus() { _ts.resetStatus(); }
  //@}
};

/**
 * Mark the end of a line in the CSV file
 *
 * \relates lgx::util::CSVStream
 */
CSVStream& eol(CSVStream& s);

#endif // CSVSTREAM_HPP


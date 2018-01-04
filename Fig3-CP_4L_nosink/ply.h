#ifndef PLY_H
#define PLY_H

#include <config.h> // include vve config.h file
#include <QString>
#include <QList>
#include <QHash>
class QFile;
class QTextStream;

#define FORALL_PLY_TYPES(macro) \
  macro(qint8) \
  macro(quint8) \
  macro(qint16) \
  macro(quint16) \
  macro(qint32) \
  macro(quint32) \
  macro(qint64) \
  macro(quint64) \
  macro(float) \
  macro(double)

#define FORALL_PLY_TYPEIDS(macro) \
  macro(qint8, PlyFile::CHAR) \
  macro(quint8, PlyFile::UCHAR) \
  macro(qint16, PlyFile::SHORT) \
  macro(quint16, PlyFile::USHORT) \
  macro(qint32, PlyFile::INT) \
  macro(quint32, PlyFile::UINT) \
  macro(qint64, PlyFile::LONG) \
  macro(quint64, PlyFile::ULONG) \
  macro(float, PlyFile::FLOAT) \
  macro(double, PlyFile::DOUBLE)

struct PlyFile
{
  enum TYPE
  {
    CHAR,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    FLOAT,
    DOUBLE,
    NB_TYPES,
    INVALID_TYPE = NB_TYPES
  };

#define INLINE_TYPE_ASSOC(T,TYPEID) static inline TYPE getType(const T& ) { return TYPEID; }
  FORALL_PLY_TYPEIDS(INLINE_TYPE_ASSOC)
#undef INLINE_TYPE_ASSOC

  static const unsigned int typeSizes[NB_TYPES];
  static char const * const typeNames[NB_TYPES+1];

  enum FORMAT_TYPES
  {
    UNSPECIFIED_FORMAT = 0,
    ASCII,
    BINARY_LITTLE_ENDIAN,
    BINARY_BIG_ENDIAN
  };

  static char const * const formatNames[4];

  struct Element;

  struct Property
  {
    enum KIND
    {
      VALUE,
      LIST
    };
    Property(const QString& name, Element *el = 0);
    ~Property();
    void allocate(size_t size);
    void deallocate();

    template <typename T>
    std::vector<std::vector<T> >* list()
    {
      if(_kind != LIST) return 0;
      if(getType(T()) != _memType) return 0;
      return (std::vector<std::vector<T> >*)_content;
    }

    template <typename T>
    std::vector<T>* value()
    {
      if(_kind != VALUE) return 0;
      if(getType(T()) != _memType) return 0;
      return (std::vector<T>*)_content;
    }

    template <typename T>
    const std::vector<std::vector<T> >* list() const
    {
      if(_kind != LIST) return 0;
      if(getType(T()) != _memType) return 0;
      return (const std::vector<std::vector<T> >*)_content;
    }

    template <typename T>
    const std::vector<T>* value() const
    {
      if(_kind != VALUE) return 0;
      if(getType(T()) != _memType) return 0;
      return (const std::vector<T>*)_content;
    }

    bool error(const QString& str) const;
    const QString& name() const { return _name; }
    TYPE fileType() const { return _fileType; }
    TYPE memType() const { return _memType; }
    TYPE sizeType() const { return _sizeType; }
    KIND kind() const { return _kind; }
    size_t size() const { return _size; }

    bool rename(const QString& n);
    void setFileType(TYPE ft) { _fileType = ft; }
    void setMemType(TYPE mt);
    void setSizeType(TYPE st) { _sizeType = st; }
    void setKind(KIND k); // Changing the kind after allocation looses all data!
    Element* parent() { return _parent; }
    const Element* parent() const { return _parent; }
    bool setParent(Element *parent);
    void resize(size_t s);

  protected:
    QString _name; // Name of the property
    TYPE _fileType; // type of the elements on the file
    TYPE _memType; // type of the elements in memory (INVALID_TYPE if the property should not be loaded)
    TYPE _sizeType; // type used to hold the number of elements (if any)
    KIND _kind; // Single value or list (later .. vector?)
    Element *_parent; // Element containing the property (if any)
    void *_content; // Content of the property, if allocated
    size_t _size; // Number of elements
  };

  struct Element
  {
    Element(const QString& name, PlyFile *parent = 0);
    ~Element();
    void allocate();

    void clear();

    size_t size() const { return _properties.size(); }
    size_t nbElements() const { return _nbElements; }
    void setNbElements(size_t n);
    QStringList properties() const;
    Property* property(size_t pos);
    const Property* property(size_t pos) const;
    Property* property(const QString& name);
    const Property* property(const QString& name) const;

    bool createValue(const QString& name, TYPE file, TYPE mem=INVALID_TYPE);
    bool createList(const QString& name, TYPE size, TYPE file, TYPE mem=INVALID_TYPE);

    bool error(const QString& str) const;
    bool hasProperty(const QString& name) const;
    bool attach(Property *prop);
    bool detach(Property *prop);
    Property* detach(const QString& name);

    const QString& name() const { return _name; }
    bool rename(const QString& n);

    void _rename_prop(Property *prop, const QString& new_name); // Don't call this yourself!
    bool allocated() const { return _allocated; }
    void _attach(Property *prop);
    void _detach(Property *prop);

    bool setParent(PlyFile* p);
    PlyFile* parent() { return _parent; }
    const PlyFile* parent() const { return _parent; }

  protected:
    QString _name;
    size_t _nbElements;
    QList<Property*> _properties;
    QHash<QString,int> _property_map;
    PlyFile *_parent;
    bool _allocated;
  };

  qint64 contentPosition;
  FORMAT_TYPES format;
  QString filetype, version;
  int version_major, version_minor;
  bool is_valid;
  void clear();
  void init(FORMAT_TYPES format = BINARY_LITTLE_ENDIAN, const QString& version = "1.0", const QString& filetype="ply");
  bool validate();
  void allocate();

  Element* element(size_t idx);
  const Element* element(size_t idx) const;
  Element* element(const QString& name);
  const Element* element(const QString& name) const;
  bool createElement(const QString& name, size_t nb_elements);
  Element* currentElement() { return current_element; }
  bool hasElement(const QString& m) const;

  size_t nbElements() const { return _elements.size(); }

  bool attach(Element *el);
  bool detach(Element *el);

  void _attach(Element *el);
  void _detach(Element *el);

  PlyFile();
  bool parseHeader(const QString& filename);
  bool parseContent(); // Parse the content of the file whose header has been parsed last
  bool error(const QString err) const;

  bool save(const QString& filename) const;

protected:
  bool parseAsciiContent(QFile& f);
  bool parseBinaryContent(QFile& f, bool little_endian);

  bool readFormat(const QStringList& fields);
  bool readElement(const QStringList& fields);
  bool readProperty(const QStringList& fields);

  bool writeHeader(QFile& f) const;
  bool writeAsciiContent(QFile& f) const;
  bool writeBinaryContent(QFile& f, bool little_endian) const;

  TYPE parseType(QString typeName) const;

  QList<Element*> _elements;
  QHash<QString,int> _element_map;

  Element *current_element;

  QString filename;
  int line_nb;
};

#endif // PLY_H


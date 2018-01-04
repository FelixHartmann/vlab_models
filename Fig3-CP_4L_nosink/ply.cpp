#include "ply.h"
#include <QFile>
#include <QTextStream>
#include <stdlib.h>
#include <QStringList>
#include <QtEndian>
#include <QDataStream>

static QTextStream out(stdout);

const unsigned int PlyFile::typeSizes[NB_TYPES] = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8};
char const * const PlyFile::typeNames[NB_TYPES+1] =
{
  "char",
  "uchar",
  "short",
  "ushort",
  "int",
  "uint",
  "long",
  "ulong",
  "float",
  "double",
  "invalid"
};

char const * const PlyFile::formatNames[4] =
{
  "unspecified",
  "ascii",
  "binary_little_endian",
  "binary_big_endian"
};

PlyFile::PlyFile()
  : line_nb(-1)
{
  clear();
}

void PlyFile::clear()
{
  current_element = 0;
  _elements.clear();
  _element_map.clear();
  contentPosition = -1;
  format = UNSPECIFIED_FORMAT;
  version = "";
  is_valid = false;
}

void PlyFile::init(FORMAT_TYPES f, const QString& ver, const QString& ft)
{
  clear();
  version = ver;
  format = f;
  filetype = ft;
}

bool PlyFile::validate()
{
  is_valid = false;
  if(format == UNSPECIFIED_FORMAT) return error("Format is not specified");;
  QStringList ver = version.split(".");
  if(ver.size() != 2) return error("the version must be on the form MAJOR.MINOR");
  bool ok;
  version_major = ver[0].toUInt(&ok);
  if(!ok) return error("MAJOR part of the version is not a valid unsigned integer");
  version_minor = ver[1].toUInt(&ok);
  if(!ok) return error("MINOR part of the version is not a valid unsigned integer");
  is_valid = true;
  current_element = 0;
  if(filetype == "ply")
  {
    // Need to check no extension is being used
  }
  return true;
}

bool PlyFile::error(const QString err) const
{
  if(!filename.isEmpty())
    out << "Error reading file '" << filename << "' ";
  if(line_nb > 0)
    out << "on line " << line_nb << " ";
  out << err << endl;
  return false;
}

bool PlyFile::parseHeader(const QString& filename)
{
  QFile file(filename);
  if(!file.open(QIODevice::ReadOnly))
  {
    out << "Error, cannot open file '" << filename << "' for reading" << endl;
    return false;
  }
  this->filename = filename;
  clear();
  QTextStream ts(&file);
  QString magick = ts.readLine().trimmed();
  line_nb = 1;
  if(magick != "ply") return error("first line must be 'ply\\n'");
  while(!ts.atEnd())
  {
    ++line_nb;
    QString line = ts.readLine().trimmed();
    if(not line.isEmpty())
    {
      QStringList fields = line.split(" ");
      QString fieldName = fields.front().toLower();
      fields.pop_front();
      if(fieldName == "comment")
        out << "// " << fields.join(" ") << endl;
      else if(fieldName == "format")
      {
        if(!readFormat(fields)) return false;
      }
      else if(fieldName == "element")
      {
        if(!readElement(fields)) return false;
      }
      else if(fieldName == "property")
      {
        if(!readProperty(fields)) return false;
      }
      else if(fieldName == "end_header")
      {
        contentPosition = ts.pos();
        break;
      }
      else
      {
        out << "*** WARNING *** Unknown field in header: " << line << endl;
      }
    }
  }
  is_valid = true;
  return true;
}

bool PlyFile::readFormat(const QStringList& fields)
{
  if(format != UNSPECIFIED_FORMAT) return error("the format has already been specified before");
  if(fields.size() != 2) return error("the format should have two fields: type and number");
  QString form = fields[0].toLower();
  version = fields[1];
  if(form == "ascii")
    format = ASCII;
  else if(form == "binary_little_endian")
    format = BINARY_LITTLE_ENDIAN;
  else if(form == "binary_big_endian")
    format = BINARY_BIG_ENDIAN;
  else
    return error(QString("Unknown format '%1'").arg(form));
  return validate();
}

bool PlyFile::readElement(const QStringList& fields)
{
  if(format == UNSPECIFIED_FORMAT) return error("the first element must be after the format specification");
  if(fields.size() != 2) return error("an element must have a name and the size");
  bool ok;
  size_t size = fields[1].toUInt(&ok);
  if(!ok) return error("the element size is not a valid unsigned integer");
  if(!createElement(fields[0], size)) return false;
  return true;
}

PlyFile::TYPE PlyFile::parseType(QString typeName) const
{
  typeName = typeName.toLower();
  for(int i = 0 ; i < NB_TYPES ; ++i)
  {
    if(typeName == typeNames[i])
      return (TYPE)i;
  }
  return INVALID_TYPE;
}

bool PlyFile::readProperty(const QStringList& fields)
{
  if(currentElement() == 0) return error("property defined outside any element");
  if(fields.size() != 2 and fields.size() != 4) return error("property must be one of 'type name' or 'list type type name'");
  QString type = fields[0].toLower();
  if(type == "list")
  {
    if(fields.size() != 4) return error("list property must have four fields: 'list' TYPE TYPE NAME");
    TYPE tc = parseType(fields[1]);
    TYPE te = parseType(fields[2]);
    if(tc == INVALID_TYPE or tc == FLOAT or tc == DOUBLE) return error("type for element count is invalid");
    if(te == INVALID_TYPE) return error("type of element is invalid");
    if(!currentElement()->createList(fields[3], tc, te)) return false;
  }
  else
  {
    if(fields.size() != 2) return error("property must have two fields: TYPE NAME");
    TYPE t = parseType(fields[0]);
    if(t == INVALID_TYPE) return error("type of element is invalid");
    if(!currentElement()->createValue(fields[1], t)) return false;
  }
  return true;
}

bool PlyFile::parseContent()
{
  if(not is_valid) return error("Error, you first need to parse the header of the file.");
  QFile f(filename);
  if(!f.open(QIODevice::ReadOnly))
  {
    out << "Error, cannot open file '" << filename << "' for reading" << endl;
    return false;
  }
  f.seek(contentPosition);
  switch(format)
  {
    case ASCII:
      return parseAsciiContent(f);
    case BINARY_LITTLE_ENDIAN:
      return parseBinaryContent(f, true);
    case BINARY_BIG_ENDIAN:
      return parseBinaryContent(f, false);
    case UNSPECIFIED_FORMAT:
      return error("Cannot process unspecified format header");
  }
  return true;
}

namespace
{

template <typename T>
T toValue(QDataStream& ds)
{
  T val;
  ds >> val;
  return val;
}

template <>
float toValue<float>(QDataStream& ds)
{
  ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
  float val;
  ds >> val;
  return val;
}

template <>
double toValue<double>(QDataStream& ds)
{
  ds.setFloatingPointPrecision(QDataStream::DoublePrecision);
  double val;
  ds >> val;
  return val;
}


template <typename T>
T toValue(QTextStream& ts)
{
  T val;
  ts >> val;
  return val;
}

template <>
int8_t toValue<int8_t>(QTextStream& ts)
{
  int val;
  ts >> val;
  return (int8_t)val;
}

template <>
uint8_t toValue<uint8_t>(QTextStream& ts)
{
  uint val;
  ts >> val;
  return (uint8_t)val;
}

template <typename T, typename Stream>
std::vector<T> toList(Stream& ss, PlyFile::TYPE sizeType)
{
  std::vector<T> lst;
  size_t size = 0;
  switch(sizeType)
  {
#define READ_SIZE(TS,TYPEID) \
    case TYPEID: \
      size = (size_t)toValue<TS>(ss); break;

    FORALL_PLY_TYPEIDS(READ_SIZE)

#undef READ_SIZE
    case PlyFile::INVALID_TYPE:
      break;
  }
  if(size > 0)
  {
    lst.resize(size);
    for(size_t i = 0 ; i < size ; ++i)
      lst[i] = toValue<T>(ss);
  }
  return lst;
}

template <typename T>
void convertList(PlyFile::Property* property, const std::vector<T>& list, int element_pos)
{
  switch(property->memType())
  {
#define ASSIGN(T1, TYPEID) \
    case TYPEID: \
    { \
      std::vector<T1>& memlist = (*property->list<T1>())[element_pos]; \
      memlist.resize(list.size()); \
      for(size_t i1 = 0 ; i1 < list.size() ; ++i1) memlist[i1] = (T1)list[i1]; \
    } break;
    FORALL_PLY_TYPEIDS(ASSIGN)
#undef ASSIGN
    case PlyFile::INVALID_TYPE:
      break;
  }
}

template <typename T>
void storeValue(PlyFile::Property* property, const T& value, int element_pos)
{
  switch(property->memType())
  {
#define ASSIGN(T1, TYPEID) \
    case TYPEID: \
      (*property->value<T1>())[element_pos] = (T1)value; break;
    FORALL_PLY_TYPEIDS(ASSIGN)
#undef ASSIGN
    case PlyFile::INVALID_TYPE:
      break;
  }
}

template <typename Stream>
bool setValue(PlyFile::Property* property, int element_pos, Stream& ts, const QString& element_name, PlyFile *ply)
{
  switch(property->fileType())
  {
#define READ(T, TYPEID) \
    case TYPEID: \
    { \
      storeValue(property, toValue<T>(ts), element_pos); \
      if(ts.status() == Stream::ReadCorruptData) \
        return ply->error(QString("Cannot parse value for property %1 of item %2 in element %3").arg(property->name()).arg(element_pos).arg(element_name)); \
    } break;

    FORALL_PLY_TYPEIDS(READ)

#undef READ
    case PlyFile::INVALID_TYPE:
      break;
  }
  return true;
}

template <typename Stream>
bool setValueList(PlyFile::Property* property, int element_pos, Stream& ts, const QString& element_name, PlyFile *ply)
{
  switch(property->fileType())
  {
#define READ(T, TYPEID) \
    case TYPEID: \
    { \
      convertList(property, toList<T>(ts, property->sizeType()), element_pos); \
      if(ts.status() == Stream::ReadCorruptData) \
        return ply->error(QString("Cannot parse list for property %1 of item %2 in element %3").arg(property->name()).arg(element_pos).arg(element_name)); \
    } break;

    FORALL_PLY_TYPEIDS(READ)

#undef READ
    case PlyFile::INVALID_TYPE:
      break;
  }
  return true;
}

template <typename Stream>
bool readProperty(PlyFile::Property* property, Stream& fields, int element_pos, const QString& element_name, PlyFile *ply)
{
  if(fields.atEnd()) return ply->error(QString("Error, there are not enough values for the item %1 of the element %2").arg(element_pos).arg(element_name));
  switch(property->kind())
  {
    case PlyFile::Property::VALUE:
      return setValue(property, element_pos, fields, element_name, ply);
    case PlyFile::Property::LIST:
      return setValueList(property, element_pos, fields, element_name, ply);
  }
  return true;
}

template <typename T1>
void convertContent(const std::vector<T1>& old_c,
                    void *newContent, PlyFile::TYPE newType)
{
  switch(newType)
  {
#define CONV_NEW(T, TYPEID) \
    case TYPEID: \
    { \
      std::vector<T>& new_c = *(std::vector<T>*)newContent; \
      new_c.resize(old_c.size()); \
      for(size_t i = 0 ; i < new_c.size() ; ++i) \
        new_c[i] = (T)old_c[i]; \
    } break;

    FORALL_PLY_TYPEIDS(CONV_NEW)

#undef CONV_NEW
    case PlyFile::INVALID_TYPE:
      break;
  }
}

void convertContent(void *oldContent, PlyFile::TYPE oldType,
                    void *newContent, PlyFile::TYPE newType)
{
  out << "Convert content from " << PlyFile::typeNames[oldType] << " to " << PlyFile::typeNames[newType] << endl;
  switch(oldType)
  {
#define CONV_OLD(T, TYPEID) \
    case TYPEID: \
    { \
      std::vector<T>* old_c = (std::vector<T>*)oldContent; \
      convertContent(*old_c, newContent, newType); \
      delete old_c; \
    } break;

    FORALL_PLY_TYPEIDS(CONV_OLD)

#undef CONV_OLD
    case PlyFile::INVALID_TYPE:
      break;
  }
}


template <typename T1>
void convertContentList(const std::vector<T1>& old_c,
                        void *newContent, PlyFile::TYPE newType)
{
  switch(newType)
  {
#define CONV_NEW(T, TYPEID) \
    case TYPEID: \
    { \
      std::vector<std::vector<T> >& new_c = *(std::vector<std::vector<T> >*)newContent; \
      new_c.resize(old_c.size()); \
      for(size_t i = 0 ; i < new_c.size() ; ++i) \
      { \
        new_c[i].resize(old_c[i].size()); \
        for(size_t j = 0 ; j < new_c[i].size() ; ++j) \
          new_c[i][j] = (T)old_c[i][j]; \
      } \
    } break;

    FORALL_PLY_TYPEIDS(CONV_NEW)

#undef CONV_NEW
    case PlyFile::INVALID_TYPE:
      break;
  }
}

void convertContentList(void *oldContent, PlyFile::TYPE oldType,
                        void *newContent, PlyFile::TYPE newType)
{
  switch(oldType)
  {
#define CONV_OLD(T, TYPEID) \
    case TYPEID: \
    { \
      std::vector<std::vector<T> >* old_c = (std::vector<std::vector<T> >*)oldContent; \
      convertContentList(*old_c, newContent, newType); \
      delete old_c; \
    } break;

    FORALL_PLY_TYPEIDS(CONV_OLD)

#undef CONV_OLD
    case PlyFile::INVALID_TYPE:
      break;
  }
}

}

bool PlyFile::parseBinaryContent(QFile& f, bool little_endian)
{
  line_nb = -1;
  QDataStream ds(&f);
  if(little_endian)
    ds.setByteOrder(QDataStream::LittleEndian);
  else
    ds.setByteOrder(QDataStream::BigEndian);
  if(ds.atEnd()) return error("There is no content in the file!!!");
  for(int i = 0 ; i < _elements.size() ; ++i)
  {
    Element* element = _elements[i];
    element->allocate();
    for(size_t j = 0 ; j < element->nbElements() ; ++j)
    {
      for(int k = 0 ; k < element->size() ; ++k)
        if(!::readProperty(element->property(k), ds, j, element->name(), this)) return false;
    }
  }
  if(!ds.atEnd()) return error("there are more data than used");
  return true;
}

bool PlyFile::parseAsciiContent(QFile& f)
{
  QTextStream ts(&f);
  for(int i = 0 ; i < _elements.size() ; ++i)
  {
    Element* element = _elements[i];
    element->allocate();
    for(size_t j = 0 ; j < element->nbElements() ; ++j)
    {
      QString line = ts.readLine().trimmed();
      QTextStream content(&line);
      for(int k = 0 ; k < element->size() ; ++k)
        if(!::readProperty(element->property(k), content, j, element->name(), this)) return false;
      if(!content.atEnd()) return error(QString("there are more fields defined than used for item %1 of element %2").arg(j).arg(element->name()));
    }
  }
  return true;
}

PlyFile::Property::Property(const QString& name, Element *p)
  : _name(name)
  , _fileType(INVALID_TYPE)
  , _memType(INVALID_TYPE)
  , _sizeType(INVALID_TYPE)
  , _content(0)
  , _parent(0)
  , _size(0)
{
  if(p) setParent(p);
}

PlyFile::Property::~Property()
{
  setParent(0);
  deallocate();
}

bool PlyFile::Property::setParent(Element *p)
{
  if(p != _parent)
  {
    if(p and p->hasProperty(_name)) return false;
    if(_parent)
      _parent->_detach(this);
    _parent = p;
    if(p)
      _parent->_attach(this);
  }
  return true;
}

bool PlyFile::Property::error(const QString& str) const
{
  out << "Error " << str << endl;
  return false;
}

bool PlyFile::Property::rename(const QString& n)
{
  if(_parent)
  {
    if(_parent->hasProperty(n)) return error(QString("Cannot rename property, its element already has a property named %1").arg(n));
    _parent->_rename_prop(this, n);
  }
  _name = n;
  return true;
}

void PlyFile::Element::_rename_prop(Property *prop, const QString& new_name)
{
  int idx = _property_map[prop->name()];
  _property_map.remove(prop->name());
  _property_map[new_name] = idx;
}

void PlyFile::Property::setKind(KIND k)
{
  if(k != _kind)
  {
    bool need_allocation = (bool)_content;
    if(need_allocation) deallocate();
    _kind = k;
    if(need_allocation) allocate(_size);
  }
}

void PlyFile::Property::setMemType(TYPE mt)
{
  if(mt != _memType)
  {
    if(_content)
    {
      TYPE oldType = _memType;
      void *oldContent = _content;
      size_t s = _size;
      _size = 0;
      _content = 0;
      _memType = mt;
      allocate(_size);
      if(_kind == VALUE)
        ::convertContent(oldContent, oldType, _content, _memType);
      else
        ::convertContentList(oldContent, oldType, _content, _memType);
    }
    else
      _memType = mt;
  }
}

void PlyFile::Property::allocate(size_t size)
{
  if(_content) deallocate();
  switch(kind())
  {
    case VALUE:
      switch(memType())
      {
#define ALLOC_VALUE(T, TYPEID) case TYPEID: _content = new std::vector<T>(size); break;
        FORALL_PLY_TYPEIDS(ALLOC_VALUE)
#undef ALLOC_VALUE
        case INVALID_TYPE:
          break;
      }
      break;
    case LIST:
      switch(memType())
      {
#define ALLOC_VALUE(T, TYPEID) case TYPEID: _content = new std::vector<std::vector<T> >(size); break;
        FORALL_PLY_TYPEIDS(ALLOC_VALUE)
#undef ALLOC_VALUE
        case INVALID_TYPE:
          break;
      }
      break;
  }
  if(_content) _size = size;
  else _size = 0;
}

void PlyFile::Property::resize(size_t s)
{
  if(_content)
  {
    switch(kind())
    {
      case VALUE:
        switch(memType())
        {
#define RESIZE_VEC(T, TYPEID) \
          case TYPEID: \
          { \
            std::vector<T>* c = (std::vector<T>*)_content; \
            c->resize(s); \
          } break;

          FORALL_PLY_TYPEIDS(RESIZE_VEC)

#undef RESIZE_VEC
          case INVALID_TYPE:
            break;
        }
        break;
      case LIST:
        switch(memType())
        {
#define RESIZE_VEC(T, TYPEID) \
          case TYPEID: \
          { \
            std::vector<std::vector<T> >* c = (std::vector<std::vector<T> >*)_content; \
            c->resize(s); \
          } break;

          FORALL_PLY_TYPEIDS(RESIZE_VEC)

#undef RESIZE_VEC
          case INVALID_TYPE:
            break;
        }
        break;
    }
  }
  _size = s;
}

void PlyFile::Property::deallocate()
{
  if(not _content) return;
  switch(kind())
  {
    case VALUE:
      switch(memType())
      {
#define FREE_VALUE(T, TYPEID) case TYPEID: delete (std::vector<T>*)_content; break;
        FORALL_PLY_TYPEIDS(FREE_VALUE)
#undef FREE_VALUE
        case INVALID_TYPE:
          break;
      }
      break;
    case LIST:
      switch(memType())
      {
#define FREE_VALUE(T, TYPEID) case TYPEID: delete (std::vector<std::vector<T> >*)_content; break;
        FORALL_PLY_TYPEIDS(FREE_VALUE)
#undef FREE_VALUE
        case INVALID_TYPE:
          break;
      }
      break;
  }
  _content = 0;
  _size = 0;
}

PlyFile::Element* PlyFile::element(const QString& name)
{
  QHash<QString,int>::iterator found = _element_map.find(name);
  if(found != _element_map.end()) return _elements[found.value()];
  return 0;
}

PlyFile::Property* PlyFile::Element::property(const QString& name)
{
  QHash<QString,int>::iterator found = _property_map.find(name);
  if(found != _property_map.end()) return _properties[found.value()];
  return 0;
}

PlyFile::Property* PlyFile::Element::property(size_t idx)
{
  return _properties[idx];
}

bool PlyFile::Element::hasProperty(const QString& name) const
{
  return _property_map.contains(name);
}

const PlyFile::Property* PlyFile::Element::property(size_t idx) const
{
  return _properties[idx];
}

const PlyFile::Element* PlyFile::element(const QString& name) const
{
  QHash<QString,int>::const_iterator found = _element_map.find(name);
  if(found != _element_map.end()) return _elements[found.value()];
  return 0;
}

const PlyFile::Property* PlyFile::Element::property(const QString& name) const
{
  QHash<QString,int>::const_iterator found = _property_map.find(name);
  if(found != _property_map.end()) return _properties[found.value()];
  return 0;
}

bool PlyFile::hasElement(const QString& name) const
{
  return _element_map.contains(name);
}

bool PlyFile::createElement(const QString& name, size_t nb_elements)
{
  if(_element_map.contains(name)) return error(QString("there is already an element called '%1'").arg(name));
  Element *el = new Element(name, this);
  el->setNbElements(nb_elements);
  current_element = el;
  return true;
}

PlyFile::Element::Element(const QString& n, PlyFile *p)
  : _name(n)
  , _nbElements(0)
  , _parent(0)
{
  if(p) setParent(p);
}

PlyFile::Element::~Element()
{
  if(_parent)
    _parent->detach(this);
  clear();
}

bool PlyFile::Element::setParent(PlyFile* p)
{
  if(p != _parent)
  {
    if(p and p->hasElement(_name)) return false;
    if(_parent)
      _parent->_detach(this);
    _parent = p;
    if(p)
      _parent->_attach(this);
  }
  return true;
}

void PlyFile::Element::setNbElements(size_t n)
{
  if(_allocated)
  {
    for(int i = 0 ; i < _properties.size() ; ++i)
      _properties[i]->resize(n);
  }
  _nbElements = n;
}

void PlyFile::Element::allocate()
{
  for(int i = 0 ; i < _properties.size() ; ++i)
    _properties[i]->allocate(_nbElements);
}

bool PlyFile::Element::error(const QString& str) const
{
  out << "Error " << str << endl;
  return false;
}

void PlyFile::Element::clear()
{
  for(int i = 0 ; i < _properties.size() ; ++i)
    delete _properties[i];
  _properties.clear();
  _property_map.clear();
  _nbElements = 0;
}

bool PlyFile::Element::attach(Property *prop)
{
  return prop->setParent(this);
}

void PlyFile::Element::_attach(Property *prop)
{
  _property_map[prop->name()] = _properties.size();
  _properties << prop;
}

bool PlyFile::Element::detach(Property *prop)
{
  return prop->setParent(0);
}

void PlyFile::Element::_detach(Property *prop)
{
  int idx = _property_map[prop->name()];
  _property_map.remove(prop->name());
  _properties.removeAt(idx);
}

PlyFile::Property* PlyFile::Element::detach(const QString& name)
{
  if(not _property_map.contains(name))
  {
    error(QString("No property named '%1'").arg(name));
    return 0;
  }
  int idx = _property_map[name];
  _property_map.remove(name);
  Property *p = _properties[idx];
  _properties.removeAt(idx);
  return p;
}

bool PlyFile::Element::createValue(const QString& name, TYPE file, TYPE mem)
{
  if(_property_map.contains(name))
    return error(QString("there is already a property called '%1' is the element '%2'").arg(name).arg(this->name()));
  Property *prop = new Property(name, this);
  prop->setFileType(file);
  prop->setMemType(mem == INVALID_TYPE ? file : mem);
  prop->setKind(Property::VALUE);
  return true;
}

bool PlyFile::Element::createList(const QString& name, TYPE size, TYPE file, TYPE mem)
{
  if(_property_map.contains(name)) return error(QString("there is already a property called '%1' is the element '%2'").arg(name).arg(this->name()));
  Property *prop = new Property(name, this);
  prop->setFileType(file);
  prop->setMemType(mem == INVALID_TYPE ? file : mem);
  prop->setSizeType(size);
  prop->setKind(Property::LIST);
  return true;
}

PlyFile::Element* PlyFile::element(size_t idx)
{
  return _elements[idx];
}

const PlyFile::Element* PlyFile::element(size_t idx) const
{
  return _elements[idx];
}

bool PlyFile::attach(Element* el)
{
  return el->setParent(this);
}

bool PlyFile::detach(Element* el)
{
  return el->setParent(0);
}

void PlyFile::_attach(Element* el)
{
  _element_map[el->name()] = _elements.size();
  _elements << el;
}

void PlyFile::_detach(Element* el)
{
  int idx = _element_map[el->name()];
  _element_map.remove(el->name());
  _elements.removeAt(idx);
}

void PlyFile::allocate()
{
  if(!is_valid) return;
  for(int i = 0 ; i < _elements.size() ; ++i)
    _elements[i]->allocate();
}

bool PlyFile::save(const QString& filename) const
{
  QFile f(filename);
  if(!f.open(QIODevice::WriteOnly))
  {
    out << "Error, cannot open file '" << filename << "' for writing" << endl;
    return false;
  }

  writeHeader(f);

  switch(format)
  {
    case ASCII:
      return writeAsciiContent(f);
    case BINARY_LITTLE_ENDIAN:
      return writeBinaryContent(f, true);
    case BINARY_BIG_ENDIAN:
      return writeBinaryContent(f, false);
    case UNSPECIFIED_FORMAT:
      return error("Cannot write content of unspecified format");
  }

  return true;
}

bool PlyFile::writeHeader(QFile& f) const
{
  QTextStream ts(&f);
  ts << filetype << "\n";
  ts << "format " << formatNames[format] << " " << version << endl;
  ts << "comment File generated by VVe writer\n";
  for(int i = 0 ; i < _elements.size() ; ++i)
  {
    const Element& el = *_elements[i];
    ts << "element " << el.name() << " " << el.nbElements() << endl;
    for(int j = 0 ; j < el.size() ; ++j)
    {
      const Property& prop = *el.property(j);
      ts << "property ";
      switch(prop.kind())
      {
        case Property::LIST:
          ts << "list " << typeNames[prop.sizeType()] << " ";
        case Property::VALUE:
          ts << typeNames[prop.fileType()] << " " << prop.name() << endl;
      }
    }
  }
  ts << "end_header" << endl;
  return true;
}

namespace
{

  template <typename T>
  void writeValue(const T& val, QDataStream& ds)
  {
    ds << val;
  }

  void writeValue(const float& val, QDataStream& ds)
  {
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);
    ds << val;
  }

  void writeValue(const double& val, QDataStream& ds)
  {
    ds.setFloatingPointPrecision(QDataStream::DoublePrecision);
    ds << val;
  }

  template <typename T>
  void writeValue(const T& val, QTextStream& ds)
  {
    ds << val << " ";
  }

  template <typename T>
  void writeList(const std::vector<T>& lst, PlyFile::TYPE , PlyFile::TYPE , QTextStream& ds)
  {
    writeValue(lst.size(), ds);
    for(size_t i = 0 ; i < lst.size() ; ++i)
      writeValue(lst[i], ds);
  }

  template <typename T>
  void writeList(const std::vector<T>& lst, PlyFile::TYPE sizeType, PlyFile::TYPE fileType, QDataStream& ds)
  {
    switch(sizeType)
    {
#define WRITE(T1, TYPEID) \
      case TYPEID: \
        writeValue((T1)lst.size(), ds); break;
      FORALL_PLY_TYPEIDS(WRITE)
#undef WRITE
      case PlyFile::INVALID_TYPE:
        break;
    }
    for(size_t i = 0 ; i < lst.size() ; ++i)
    {
      switch(fileType)
      {
#define WRITE(T1, TYPEID) \
      case TYPEID:\
        writeValue((T1)lst[i], ds); break;
      FORALL_PLY_TYPEIDS(WRITE)
#undef WRITE
      case PlyFile::INVALID_TYPE:
        break;
      }
    }
  }

  template <typename T, typename Stream>
  void dumpValue(const T& val, PlyFile::TYPE type, Stream& ss)
  {
    switch(type)
    {
#define WRITE(T1, TYPEID) case TYPEID: writeValue((T1)val, ss); break;
      FORALL_PLY_TYPEIDS(WRITE)
#undef WRITE
      case PlyFile::INVALID_TYPE:
        break;
    }
  }

  template <typename Stream>
  bool writeValue(const PlyFile::Property& prop, size_t element_pos, Stream& ds, const QString& element_name, const PlyFile* ply)
  {
    switch(prop.memType())
    {
#define WRITE(T, TYPEID) \
      case TYPEID: \
      { \
        const T& val = (*prop.value<T>())[element_pos]; \
        dumpValue(val, prop.fileType(), ds); \
        if(ds.status() == Stream::ReadCorruptData) \
          return ply->error(QString("Cannot write list for property %1 of item %2 in element %3").arg(prop.name()).arg(element_pos).arg(element_name)); \
      } break;

      FORALL_PLY_TYPEIDS(WRITE)

#undef WRITE
      case PlyFile::INVALID_TYPE:
        break;
    }
    return true;
  }

template <typename Stream>
bool writeValueList(const PlyFile::Property& prop, size_t element_pos, Stream& ds, const QString& element_name, const PlyFile* ply)
{
  switch(prop.memType())
  {
#define WRITE(T, TYPEID) \
    case TYPEID: \
    { \
      const std::vector<T>& lst = (*prop.list<T>())[element_pos]; \
      writeList(lst, prop.sizeType(), prop.fileType(), ds); \
      if(ds.status() == Stream::ReadCorruptData) \
        return ply->error(QString("Cannot write list for property %1 of item %2 in element %3").arg(prop.name()).arg(element_pos).arg(element_name)); \
    } break;

    FORALL_PLY_TYPEIDS(WRITE)

#undef WRITE
    case PlyFile::INVALID_TYPE:
      break;

  }
  return true;
}

template <typename Stream>
bool writeProperty(const PlyFile::Property& prop, Stream& ds, size_t element_pos,
                   const QString& element_name, const PlyFile* ply)
{
  switch(prop.kind())
  {
    case PlyFile::Property::VALUE:
      return writeValue(prop, element_pos, ds, element_name, ply);
    case PlyFile::Property::LIST:
      return writeValueList(prop, element_pos, ds, element_name, ply);
  }
  return true;
}

void endOfElement(QTextStream& ts)
{
  ts << endl;
}

void endOfElement(QDataStream&) { }

template <typename Stream>
bool writeContent(Stream& ss, const PlyFile* ply)
{
  for(int i = 0 ; i < ply->nbElements() ; ++i)
  {
    const PlyFile::Element& el = *ply->element(i);
    for(size_t j = 0 ; j < el.nbElements() ; ++j)
    {
      for(int k = 0 ; k < el.size() ; ++k)
      {
        const PlyFile::Property& prop = *el.property(k);
        if(!writeProperty(prop, ss, j, el.name(), ply)) return false;
      }
      endOfElement(ss);
    }
  }
  return true;
}

}

bool PlyFile::writeAsciiContent(QFile& f) const
{
  QTextStream ts(&f);
  return writeContent(ts, this);
}

bool PlyFile::writeBinaryContent(QFile& f, bool little_endian) const
{
  QDataStream ds(&f);
  if(little_endian)
    ds.setByteOrder(QDataStream::LittleEndian);
  else
    ds.setByteOrder(QDataStream::BigEndian);
  return writeContent(ds, this);
}


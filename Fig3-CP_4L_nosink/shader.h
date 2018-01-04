#ifndef SHADER_H
#define SHADER_H

#include <util/parms.h>
#include <util/gl.h>
#include <string>
#include <vector>
#include <iostream>
#include <util/vector.h>
#include <util/matrix.h>
#include <util/assert.h>

#define REPORT_GL_ERROR(str) util::Shader::reportGLError(str, __FILE__, __LINE__)

class QString;

namespace util
{
  typedef util::Vector<1,GLint> ivec1;
  typedef util::Vector<2,GLint> ivec2;
  typedef util::Vector<3,GLint> ivec3;
  typedef util::Vector<4,GLint> ivec4;
  typedef util::Vector<1,GLfloat> vec1;
  typedef util::Vector<2,GLfloat> vec2;
  typedef util::Vector<3,GLfloat> vec3;
  typedef util::Vector<4,GLfloat> vec4;
  typedef util::Matrix<2,2,GLfloat> mat2;
  typedef util::Matrix<3,3,GLfloat> mat3;
  typedef util::Matrix<4,4,GLfloat> mat4;

  enum UNIFORM_TYPE
  {
    UNIFORM_INT,
    UNIFORM_INT2,
    UNIFORM_INT3,
    UNIFORM_INT4,
    UNIFORM_FLOAT,
    UNIFORM_FLOAT2,
    UNIFORM_FLOAT3,
    UNIFORM_FLOAT4,
    UNIFORM_MATRIX2,
    UNIFORM_MATRIX3,
    UNIFORM_MATRIX4
  };

  class GLSLValue
  {
    class Value
    {
    public:
      virtual ~Value() {}
      virtual void setUniform(GLint location) const = 0;
      virtual void setAttrib(GLuint location) const = 0;
      virtual QTextStream& read(QTextStream& s) = 0;
      virtual QTextStream& write(QTextStream& s) const = 0;
      virtual std::istream& read(std::istream& s) = 0;
      virtual std::ostream& write(std::ostream& s) const = 0;
      virtual Value* copy() = 0;
    } *value;

    template <typename T>
    class ValueImpl : public Value
    {
    public:
      typedef typename T::value_type value_type;
      typedef void GLAPIENTRY (*uniform_fct)(GLint,GLsizei,const value_type*);
      typedef void GLAPIENTRY (*attrib_fct)(GLuint,const value_type*);

      ValueImpl(uniform_fct ufct, attrib_fct afct, const T& v)
        : value(v)
        , glUniform(ufct)
        , glVertexAttrib(afct)
      {}

      ValueImpl(uniform_fct ufct, attrib_fct afct)
        : glUniform(ufct)
        , glVertexAttrib(afct)
      {}

      ValueImpl(const ValueImpl& copy)
        : value(copy.value)
        , glUniform(copy.glUniform)
        , glVertexAttrib(copy.glVertexAttrib)
      {}

      virtual Value* copy() { return new ValueImpl(*this); }

      virtual void setUniform(GLint location) const { glUniform(location, 1, value.c_data()); }
      virtual void setAttrib(GLuint location) const
      {
        vvassert_msg(glVertexAttrib, "Attribute of invalid type.");
        glVertexAttrib(location, value.c_data());
      }
      virtual QTextStream& read(QTextStream& s)
      {
        s >> value;
        return s;
      }
      virtual QTextStream& write(QTextStream& s) const
      {
        s << value;
        return s;
      }
      virtual std::istream& read(std::istream& s)
      {
        s >> value;
        return s;
      }
      virtual std::ostream& write(std::ostream& s) const
      {
        s << value;
        return s;
      }
      T value;
      uniform_fct glUniform;
      attrib_fct glVertexAttrib;
    };

    UNIFORM_TYPE type;

  public:
    GLSLValue() : value(0), type(UNIFORM_INT) {}

    GLSLValue(const GLSLValue& copy) : value(0), type(copy.type) { if(copy.value) value = copy.value->copy(); }

    template <typename T>
    explicit GLSLValue(const T& val) : value(0) { setValue(val); }

    ~GLSLValue() { delete value; }
    GLSLValue& operator=(const GLSLValue& copy)
    {
      delete value;
      value = 0;
      if(copy.value)
        value = copy.value->copy();
      type = copy.type;
      return *this;
    }
    void setUniform(GLint location) const { value->setUniform(location); }
    void setAttrib(GLuint location) const { value->setAttrib(location); }
    std::istream& read(std::istream& s);
    std::ostream& write(std::ostream& s) const;
    QTextStream& read(QTextStream& s);
    QTextStream& write(QTextStream& s) const;
    void setValue(const GLint& value);
    void setValue(const ivec1& value);
    void setValue(const ivec2& value);
    void setValue(const ivec3& value);
    void setValue(const ivec4& value);
    void setValue(const GLfloat& value);
    void setValue(const vec1& value);
    void setValue(const vec2& value);
    void setValue(const vec3& value);
    void setValue(const vec4& value);
    void setValue(const mat2& value);
    void setValue(const mat3& value);
    void setValue(const mat4& value);
    bool valid() const { return value != 0; }
  };

  inline QTextStream& operator<<(QTextStream& s, const GLSLValue& ut) { return ut.write(s); }

  inline QTextStream& operator>>(QTextStream& s, GLSLValue& ut) { return ut.read(s); }

  inline std::ostream& operator<<(std::ostream& s, const GLSLValue& ut) { return ut.write(s); }

  inline std::istream& operator>>(std::istream& s, GLSLValue& ut) { return ut.read(s); }

  class Shader
  {
  public:
    Shader(int verbosity=1);

    bool init();

    void readParms(Parms& parms, QString shaders_section, QString uniforms_section);

    bool setupShaders();
    bool useShaders();
    bool stopUsingShaders();

    static bool reportGLError( const char* id, const char* file, int line );

    QString shaderTypeName(GLenum shader_type);

    GLhandleARB compileShaderFile(GLenum shader_type, QString filename);
    GLhandleARB compileShader(GLenum shader_type, QString content);

    void printInfoLog(GLhandleARB object);
    void cleanShaders();

    bool hasShaders() const { return has_shaders; }

    GLhandleARB program() const { return _program; }

    void setupUniforms();
    void locateAttributes();

    GLuint attribLocation(const QString& name);

    void setAttrib(const QString& name, const GLSLValue& value);
    void setAttrib(GLuint loc, const GLSLValue& value);

    bool setUniform(const QString& name, const GLSLValue& value);

    bool initialized() const { return _initialized; }

  protected:
    void loadUniform(GLint program, const QString& name, const GLSLValue& value);

    bool has_shaders;

    std::vector<QString> vertex_shaders_files, fragment_shaders_files;
    std::vector<QString> vertex_shaders_code, fragment_shaders_code;

    std::vector<GLhandleARB> vertex_shaders, fragment_shaders;

    std::vector<QString> uniform_names, model_uniform_names;
    std::vector<GLSLValue> uniform_values, model_uniform_values;

    int verbosity;

    GLhandleARB _program;

    bool _initialized;
  };
}

#endif // SHADER_H


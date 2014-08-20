#ifndef NKIT_VX_BASE_BUILDER_H
#define NKIT_VX_BASE_BUILDER_H


#include "nkit/constants.h"
#include "nkit/types.h"
#include <string>

namespace nkit
{
  template< typename Policy >
  class Builder
  {
  public:
    typedef typename Policy::type type;

    void InitAsBoolean( std::string const & value )
    {
      p_._InitAsBoolean(value);
    }

    void InitAsBooleanFormat( std::string const & value, const std::string & )
    {
      p_._InitAsBoolean(value);
    }

    void InitAsBooleanDefault()
    {
      p_._InitAsBoolean(nkit::S_FALSE_);
    }

    void InitAsInteger( std::string const & value )
    {
      p_._InitAsInteger(value);
    }

    void InitAsIntegerFormat( std::string const & value, const std::string & )
    {
      p_._InitAsInteger(value);
    }

    void InitAsIntegerDefault()
    {
      p_._InitAsInteger(nkit::S_ZERO_);
    }

    void InitAsString( std::string const & value )
    {
      p_._InitAsString(value);
    }

    void InitAsStringFormat( std::string const & value, const std::string & )
    {
      p_._InitAsString(value);
    }

    void InitAsStringDefault()
    {
      p_._InitAsString(nkit::S_EMPTY_);
    }

    void InitAsUndefined()
    {
      p_._InitAsUndefined();
    }

    void InitAsFloat( std::string const & value )
    {
      p_._InitAsFloatFormat( value, NKIT_FORMAT_DOUBLE );
    }

    void InitAsFloatFormat( std::string const & value,
        std::string const & format )
    {
      p_._InitAsFloatFormat( value, format.c_str() );
    }

    void InitAsFloatDefault()
    {
      p_._InitAsFloatFormat( nkit::S_ZERO_, NKIT_FORMAT_DOUBLE );
    }

    void InitAsDatetime( std::string const & value )
    {
      p_._InitAsDatetimeFormat( value, nkit::DATE_TIME_DEFAULT_FORMAT_ );
    }

    void InitAsDatetimeFormat( std::string const & value,
        std::string const & format )
    {
      p_._InitAsDatetimeFormat( value, format.c_str() );
    }

    void InitAsDatetimeDefault()
    {
      p_._InitAsDatetimeFormat( "", nkit::DATE_TIME_DEFAULT_FORMAT_);
    }

    void InitAsList()
    {
      p_._InitAsList();
    }

    void InitAsDict()
    {
      p_._InitAsDict();
    }

    void AppendToList( type const & obj )
    {
      p_._ListCheck();
      p_._AppendToList(obj);
    }

    void SetDictKeyValue( std::string const & key, type const & var )
    {
      p_._DictCheck();
      p_._SetDictKeyValue( key, var );
    }

    type const & get() const
    {
      return p_.get();
    }

  private:
    Policy p_;
  };

} // namespace nkit

#endif // NKIT_VX_BASE_BUILDER_H

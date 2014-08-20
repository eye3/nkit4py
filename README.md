# Introduction

nkit4nodejs - is a [nkit](https://github.com/eye3/nkit.git) C++ library port to 
Python.

Currently, only an XML to Python object or list converter and filter
is exported to Python from nkit library.

You can:
 
- create Python data structures, which are different from the structure 
  of XML source.
  
- explicitly identify those elements and attributes in XML source that you
  want to use for building Python data structures.
  Thus, it's possible to filter out unnecessary XML-data.
  
- explicitly define Python type of scalar data, fetched from XML source.
  Integers, numbers, strings, datetimes and booleans are supported.

Conversion is carried out using SAX parser Expat, so it's fast and uses less 
memory when parsing huge XML files.

Module supports not only native Expat XML encodings, but also many others
(see /deps/nkit/src/vx/encodings.inc)

# Installation

This module is still under development. Please come back later.

# Author

Boris T. Darchiev (boris.darchiev@gmail.com)

On github: https://github.com/eye3

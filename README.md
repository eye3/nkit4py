# Introduction

nkit4nodejs - is a [nkit](https://github.com/eye3/nkit.git) C++ library port to 
Node.js server.

Currently, only an XML to Javascript object converter and filter is exported to 
Node.js from nkit library.

You can:
 
- create Javascript data structures, which are different from the structure 
  of XML source.
  
- explicitly identify those elements and attributes in XML source that you
  want to use for building JavaScript data structures.
  Thus, it's possible to filter out unnecessary XML-data.
  
- explicitly define Javascript type of scalar data, fetched from XML source.
  Integers, numbers, strings, datetimes and booleans are supported.

Conversion is carried out using SAX parser Expat, so it's fast and uses less 
memory when parsing huge XML files.

This module faster then any other xml-to-javascript module, written in pure JavaScript.
For example, nkit4nodejs is about 10 times faster than popular 
[xml2js](https://www.npmjs.org/package/xml2js) module on parsing 
20Mb XML file (see test/compare.js for comparison code).

Module supports not only native Expat XML encodings, but also many others
(see /deps/nkit/src/vx/encodings.inc)

# Installation

This module is still under development. Please come back later.

# Author

Boris T. Darchiev (boris.darchiev@gmail.com)

On github: https://github.com/eye3

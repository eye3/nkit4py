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
(see /deps/nkit/src/vx/encodings_inc_gen.cpp)

# Installation

## On Linux & Mac OS

    npm install nkit4nodejs
    
## On Windows

Library compiles on MSVS Express version >= 2012.
For MSVS 2012:

    SET VS90COMNTOOLS=%VS100COMNTOOLS% 
    pip install nkit4nodejs

For MSVS 2013:

    SET VS90COMNTOOLS=%VS120COMNTOOLS%
    pip install nkit4nodejs


# Usage

Suppose, we have this xml string:

    <?xml version="1.0"?>
    <any_name>
        <person>
            <name>Jack</name>
            <phone>+122233344550</phone>
            <phone>+122233344551</phone>
            <age>33</age>
            <married firstTime="No">Yes</married>
            <birthday>1980-02-28</birthday>
            <address>
                <city>New York</city>
                <street>Park Ave</street>
                <buildingNo>1</buildingNo>
                <flatNo>1</flatNo>
            </address>
            <address>
                <city>Boston</city>
                <street>Centre St</street>
                <buildingNo>33</buildingNo>
                <flatNo>24</flatNo>
            </address>
        </person>
        <person>
            <name>Boris</name>
            <phone>+122233344553</phone>
            <phone>+122233344554</phone>
            <age>34</age>
            <married firstTime="Yes">Yes</married>
            <birthday>1979-05-16</birthday>
            <address>
                <city>Moscow</city>
                <street>Kahovka</street>
                <buildingNo>1</buildingNo>
                <flatNo>2</flatNo>
            </address>
            <address>
                <city>Tula</city>
                <street>Lenina</street>
                <buildingNo>3</buildingNo>
                <flatNo>78</flatNo>
            </address>
        </person>
    </any_name>


### To build list-of-strings from xml string:

    var nkit = require('nkit4nodejs');
    
    // Here mapping is list, described by '/path/to/element' and list-item-description.
    // List item here is a 'string' scalar.
    // Scalar definition contains type name and optional default value.
    var mapping = ["/person/phone", "string"];
    //var mapping = ["/person/phone", "string|optionalDefaultValue"];

    var builder = new nkit.Xml2VarBuilder(mapping);
    builder.feed(xmlString); // can be more than one call to feed(xmlChunk) method
    var result = builder.end();
    console.log(JSON.stringify(result, null, '  '));

Result:

    [
      "+122233344550",
      "+122233344551",
      "+122233344553",
      "+122233344554"
    ]
    

# Author

Boris T. Darchiev (boris.darchiev@gmail.com)

On github: https://github.com/eye3

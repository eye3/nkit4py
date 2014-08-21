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

	# Here mapping is list, described by '/path/to/element' and list-item-description.
	# List item here is a 'string' scalar.
	# Scalar definition contains type name and optional default value.
	mapping = '["/person/phone", "string"]';
	# mapping = '["/person/phone", "string|optionalDefaultValue"]';

	builder = Xml2VarBuilder(mapping)
	builder.feed(xmlString)
	result = builder.end()

Result:

    [
      "+122233344550",
      "+122233344551",
      "+122233344553",
      "+122233344554"
    ]
    
### To build simple object from xml string (last 'person' xml element will be used):

	#  Here mapping is object, described by set of mappings, each containing
	#  key definition and scalar definition.
	#  Keys are described by "/sub/path -> optionalKeyName".
	#  If optionalKeyName doesn't provided, then last element name in /sub/path
	#  will be used for key name.
	#  Scalar definition may have optional "...|defaultValue"
	mapping = """{
	    "/person/name -> lastPersonName": "string|Captain Nemo",
	    "/person/married/@firstTime -> lastPersonIsMarriedFirstTime":
	        "boolean|True",
	    "/person/age": "integer"
	}"""
	builder = Xml2VarBuilder(mapping)
	builder.feed(xmlString)
	result = builder.end()

Result:

	{
	  "age": 34, 
	  "lastPersonName": "Boris", 
	  "lastPersonIsMarriedFirstTime": true
	}


### To build list-of-lists-of-strings from xml string
	 
	#  Here mapping is list, described by /path/to/element and list item
	#  description. List item is described as 'list' sub-mapping, described 
	#  by sub-path and'string' scalar definition
	mapping = '["/person", ["/phone", "string"]]';

	builder = Xml2VarBuilder(mapping);
	builder.feed(xmlString); # can be more than one call to feed(xmlChunk) method
	result = builder.end();

Result:

	[
	  [
	    "+122233344550", 
	    "+122233344551"
	  ], 
	  [
	    "+122233344553", 
	    "+122233344554"
	  ]
	]


### To build list-of-objects-with-lists from xml string
 
	#  Here mapping is list, described by /path/to/element and list item description.
	#  List item is described as 'object' sub-mapping.
	#  This 'object' sub-mapping described by set of mappings, each containing
	#  key definition and sub-mapping or scalar.
	#  Keys are described by "/sub/path -> optionalKeyName".
	#  If optionalKeyName doesn't provided, then last element name in "/sub/path"
	#  will be used for key name
	#  Scalar definition may have optional "...|defaultValue"
	#  'datetime' scalar definition MUST contain default value and formatting string
	mapping = """["/person",
	    {
	        "/birthday": "datetime|1970-01-01|%Y-%m-%d",
	        "/phone -> phones": ["/", "string"],
	        "/address -> cities": ["/city", "string"],
	            // same as "/address/city -> cities": ["/", "string"]
	        "/married/@firstTime -> isMerriedFirstTime": "boolean"
	    }
	]"""
	
	builder = Xml2VarBuilder(mapping);
	builder.feed(xmlString); # can be more than one call to feed(xmlChunk) method
	result = builder.end();

Result:

	[
	  {
	    "phones": [
	      "+122233344550", 
	      "+122233344551"
	    ], 
	    "cities": [
	      "New York", 
	      "Boston"
	    ], 
	    "birthday": "1970-11-28 00:00:00", 
	    "isMerriedFirstTime": false
	  }, 
	  {
	    "phones": [
	      "+122233344553", 
	      "+122233344554"
	    ], 
	    "cities": [
	      "Moscow", 
	      "Tula"
	    ], 
	    "birthday": "1969-07-16 00:00:00", 
	    "isMerriedFirstTime": true
	  }
	]

# Notes

Possible scalar types:

    - string
    - integer
    - number    // with floating point
    - datetime  // on Windows - without localization support yet
    - boolean
    
Scalar types can be followed by '|' sign and default value

**datetime** type MUST be followed by '|' sign, default value,
another '|' sign and format string. See 
[man strptime](http://linux.die.net/man/3/strptime) for datetime formatting
syntax. Default value of datetime must correspond to format string.

Path in mapping specifications are very simple XPath now. Only

    /element/with/optional/@attribute
    
paths are supported.
    
JavaScript object keys get their names from the last element name in the path.
If you want to change key names, use this notation:

    "/path/to/element -> newKeyName": ...
    "/path/to/element/@attribute -> newKeyName": ...

# TODO

- /path/with/*/signs/in/any/place
- options: trim, etc
- More then one 'mapping' parameters for nkit.Xml2VarBuilder(...) constructor to
  create more then one JavaScript data structures from one xml string:


    var mapping1 = ...;
    var mapping2 = ...;
    var builder = nkit.Xml2VarBuilder(mapping1, mapping2);
    builder.feed(xmlString);
    var result_list = builder.end();
    var result1 = result_list[0];
    var result2 = result_list[1];
    

# Author

Boris T. Darchiev (boris.darchiev@gmail.com)

On github: https://github.com/eye3

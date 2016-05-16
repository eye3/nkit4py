

<!-- toc -->

* [Introduction](#introduction)
* [Installation](#installation)
  * [Requirements](#requirements)
  * [On Linux and Mac OS](#on-linux-and-mac-os)
  * [On Windows](#on-windows)
* [XML to Python data conversion](#xml-to-python-data-conversion)
  * [Quick start without mappings](#quick-start-without-mappings)
  * [Getting started with mappings](#getting-started-with-mappings)
  * [Building simple object from xml string (last 'person' xml element will be used)](#building-simple-object-from-xml-string-last-person-xml-element-will-be-used)
  * [Building list-of-objects from xml string](#building-list-of-objects-from-xml-string)
  * [Building list-of-objects-with-lists from xml string](#building-list-of-objects-with-lists-from-xml-string)
  * [Creating keys in object for non-existent xml elements](#creating-keys-in-object-for-non-existent-xml-elements)
  * [Using attribute values to generate Dict keys](#using-attribute-values-to-generate-dict-keys)
  * [Building data structures from big XML source, reading it chunk by chunk](#building-data-structures-from-big-xml-source-reading-it-chunk-by-chunk)
  * [Options](#options)
    * ['attrkey' option](#attrkey-option)
  * [Notes](#notes)
* [Python data to XML conversion](#python-data-to-xml-conversion)
  * [Quick start](#quick-start)
  * [Options for var2xml](#options-for-var2xml)
* [Python version support](#python-version-support)
* [Change log](#change-log)
* [Author](#author)
* [Travis](#travis)

<!-- toc stop -->



# Introduction

nkit4py - is a [nkit](https://github.com/eye3/nkit.git) C++ library port to Python.

With nkit4py module you can convert XML string to Python data and vise versa.

**With XML-to-Python-data possibilities you can:**
 
- Simply convert XML to Python data with the same structure.
  Usual Python Dict objects as well as collections.OrderedDict objects can be created.
  
- Convert XML to Python data with the structure, which is different from the structure 
  of XML source (using mapping).
  
- Create multiple Python structures from one XML source.

- Explicitly identify those elements and attributes in XML source that you
  want to use for building Python data structures.
  Thus, it's possible to filter out unnecessary XML-data.
  
- Explicitly define Python type of scalar (primitive) data,
  fetched from XML source.
  Integers, numbers, strings, datetimes and booleans are supported.

- Control progress of chunked download and parsing of big XML string

- With extra options you can tune some aspects of conversion:
	- trim text data
	- explicitly define white space characters for trim option
	- choose unicode or string type for text data
	- define special key to collect all element attributes

Conversion is carried out using SAX parser Expat, so it's fast and uses less 
memory when parsing huge XML files.

Module supports not only native Expat XML encodings, but also many others
(see /deps/nkit/src/encoding/langs.inc)


**With Python-data-to-XML possibilities you can:**

- Create xml string with the same structure as Python data
- Define root element name of result xml string
- Define default element name for list items
- Define encoding of result xml string
- Pretty print with custom indentation and newline characters
- Define special object key name for attributes
- Define special object key name for text
- Define which elements of result xml string must contain CDATA section
- Define precision for float numbers
- Define format for Date objects
- Define representation for *True* and *False* values
- Explicitly define order in which DICT keys will be printed to XML text
- Define type o result: unicode or stringf


# Installation

## Requirements

This module must be compiled, so you have to install "build essentials" 
(if not yet):

- for debian compatible linux (Debian, Ubuntu, etc.):

	sudo apt-get install build-essential

- for red hat compatible linux (RHEL, SUSE, CentOS, etc.):

	sudo yum groupinstall "Development Tools"

- for Windows:
	
	Install MS Visual Studio 2012 or higher	([MSVS Express](http://www.visualstudio.com/en-us/products/visual-studio-express-vs.aspx) is ok)
	
- in Mac OS use XCode & brew

## On Linux and Mac OS

    pip install nkit4py

## On Windows

Library compiles on MSVS version >= 2012.

For MSVS 2012:

    SET VS90COMNTOOLS=%VS110COMNTOOLS%

    pip install nkit4py

For MSVS 2013:

    SET VS90COMNTOOLS=%VS120COMNTOOLS%

    pip install nkit4py


# XML to Python data conversion

## Quick start without mappings

Suppose, we have this xml string:

```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<persons type="sample">
    <person>
        <name>Jack</name>
        <phone>+122233344550</phone>
        <phone>+122233344551</phone>
    </person>
    <person>
        <name>Boris</name>
        <phone>+122233344553</phone>
        <phone>+122233344554</phone>
    </person>
    any text
</persons>
```

If we call this script:

```python
import nkit4py

options = {
    "trim": True,
    "attrkey": "$",
    "textkey": "_",
    "ordered_dict": False
}

builder = nkit4py.AnyXml2VarBuilder(options)
builder.feed(xml_string)
result = builder.end()
print builder.root_name()
print result
```

we will receive the following structure in 'result':

```json
{
  "person": [
    {
      "phone": [
        "+122233344550", 
        "+122233344551"
      ], 
      "name": [
        "Jack"
      ]
    }, 
    {
      "phone": [
        "+122233344553", 
        "+122233344554"
      ], 
      "name": [
        "Boris"
      ]
    }
  ], 
  "$": {
    "type": "sample"
  }, 
  "_": "any text"
}
```

With option 'ordered_dict' == True we will get collections.OrderedDict objects
instead of usual python Dict.

We can get same XML string back with the following script:

```python
import nkit4py

options = {
    "rootname": "persons",
    "encoding": "UTF-8",
    "xmldec": {
        "version": "1.0",
        "standalone": True,
    },
    "priority": ["name",
                 "phone"
    ],
    "pretty": {
        "indent": "    ",
        "newline": "\n",
    },
    "attrkey": "$",
    "textkey": "_",
}

xml = nkit4py.var2xml(result, options)
print xml
```

NOTE: 'priority' option is important if you want print XML elements in fixed order

## Getting started with mappings

Suppose, we have this xml string:

```xml
<?xml version="1.0"?>
<any_name type="sample">
    <person>
        <phone>+122233344550</phone>
        <name>Jack</name>
        <phone>+122233344551</phone>
		<photos>
            <photo>img1</photo>
            <photo>img2</photo>
            <ph>img3</ph>
        </photos>
        <age>33</age>
        <married firstTime="No">Yes</married>
        <birthday>Wed, 28 Mar 1979 12:13:14 +0300</birthday>
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
        <phone>+122233344553</phone>
        <name>Boris</name>
        <phone>+122233344554</phone>
		<photos>
            <photo>img3</photo>
            <photo>img4</photo>
        </photos>
        <age>34</age>
        <married firstTime="Yes">Yes</married>
        <birthday>Mon, 31 Aug 1970 02:03:04 +0300</birthday>
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
```

With this script we cat generate two data structures:

```python
from nkit4py import Xml2VarBuilder

mappings = {
    "list_of_strings": ["/person/phone", "string"],
    "list_of_lists_of_strings": ["/person", ["/phone", "string"]]
}

builder = Xml2VarBuilder(mappings)
builder.feed(xml_string)
result = builder.end()
list_of_strings = result["list_of_strings"]
list_of_lists_of_strings = result["list_of_lists_of_strings"]
```

Value of list_of_strings:

```json
[
  "+122233344550",
  "+122233344551",
  "+122233344553",
  "+122233344554"
]
```
    
Value of list_of_lists_of_strings:

```json
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
```

Let's consider script above.

First of all, we importing nkit4py's class Xml2VarBuilder,
which is responsible for xml-to-python-structures conversion.

```python
from nkit4py import Xml2VarBuilder
```

Xml2VarBuilder class uses 'mappings' structure, i.e. some directives about how
to perform conversion. Mappings can be written in JSON string (or they can be
JSON-compatible python structures, as in our example), they describe conversion
process and final structures of python data. Our example contains two mappings:
'list_of_strings' and 'list_of_lists_of_strings':

```python
mappings = {
    "list_of_strings": ["/person/phone", "string"],
    "list_of_lists_of_strings": ["/person", ["/phone", "string"]]
}
```

This means that after conversion we expect to get two data structures:
list of all phones of all persons, and list of phone lists for each person.

First mapping - ["/person/phone", "string"]. It is enclosed in [] brackets.
This means that we expect to get python list. This type of mapping is called
'list-mapping'. (Braces - {} - means that we want to get python objects.
Not in this example - see below).
First item of list-mapping defines the XPath where we want to find
data. Second item defines a sub-mapping, which in our case is a
scalar-submapping.
Scalar-submapping contains information about type of data we want to get
('string' in our case). 
During conversion nkit4py module will find all elements at path "/person/phone",
convert their values to python unicode and put them into python list.

Second mapping - ["/person", ["/phone", "string"]] is another list-mapping, but
first item points to "/person" XPath, and second item is list-submapping.
List-submapping also contains two elements: sub-xpath and another submapping
('string' scalar-sabmapping in our case). Sub-xpath MUST be continuation of
parent mapping xpath.
During conversion nkit4py module will find all "person" elements and for each
"person" element it will find all "phone" sub-elements, convert their values to
python unicode and put them into python list, which in turn will be placed to
main list.

Each mapping have to be placed in "mappings" structure with some user defined
name. This name will be used in the future to get actual data from result.
In our case these names are: 'list_of_strings' and 'list_of_lists_of_strings'.

Now we create builder object:

```python
builder = Xml2VarBuilder(mappings)
```

and feed our xml string to it:

```python
builder.feed(xml_string)
```

If we receiving xml by chunks, then it is possible to call builder.feed() method
many times (one time for each chunk in order they received).

After feeding all chunks to builder we call end() method to indicate that xml
has been completely received. Also builder.end() method returns all data
structures at once (to "result" variable in our case):

```python
result = builder.end()
```

Now we can get our structures by their mapping names:

```python
list_of_strings = result["list_of_strings"]
list_of_lists_of_strings = result["list_of_lists_of_strings"]
```

## Building simple object from xml string (last 'person' xml element will be used)

```python
from nkit4py import Xml2VarBuilder

mapping = {   # <- opening brace for object-mapping
    
        "/person/name -> lastPersonName": "string|Captain Nemo",
        "/person/married/@firstTime -> lastPersonIsMarriedFirstTime":
            "boolean|True",
        "/person/age": "integer"
    
    }   # <- closing brace of object-mapping

mappings = {"last_person": mapping}

builder = Xml2VarBuilder(mappings)
builder.feed(xml_string)
result = builder.end()
last_person = result["last_person"]
```

Value of last_person:

```json
{
  "name": "Boris", 
  "is_married_first_time": true,
  "age": 34
}
```

Now we use object-mapping (look at the {} braces). Object mapping consists of
mapping items. In our case there is three mapping items:

1. "/person/name": "string|Captain Nemo"
2. "/person/married/@firstTime -> is_married_first_time": "boolean|True"
3. "/person/age": "integer"

Each mapping item consists of object-key-definition and sub-mapping.

Object-key-definitions are described by "xpath" or "xpath -> optional_key_name".
If no optional_key_name has been provided, then last element name in XPath will
be used for key name ("name" in mapping item #1 and "age" in mapping item #3).
Thus, our result object will contain three items with "name",
"is_married_first_time" and "age" keys.

Value for each mapping item will be constructed from data at provided XPath
according to their sub-mappings. In our example, all sub-mappings are scalars.
Note, that it is possible to use "delault values" by putting them in
scalar-submapping after type definition and "|" delimiter (
"Captain Nemo" in mapping item #1 and "True" in mapping item #2).
Default values for scalars are working only in object-mappings, not in
list-mappings.



## Building list-of-objects from xml string
 
```python
from nkit4py import Xml2VarBuilder

mapping = ["/person",
    {
        "/birthday": "datetime|Fri, 22 Aug 2014 13:59:06 +0000|%a, %d %b %Y %H:%M:%S %z",
        "/name": "string"
    }
]

mappings = {"persons": mapping}

builder = Xml2VarBuilder(mappings)
builder.feed(xml_string)
result = builder.end()
persons = result["persons"]
```

Value of persons:

```json
[
  {
    "birthday": "1970-11-28 00:00:00", 
    "name": "Jack"
  }, 
  {
    "birthday": "1969-07-16 00:00:00", 
    "name": "Boris"
  }
]
```

Now we use list-mapping and object-submapping. Module will:

- find all person elements
- for each person element construct object from their "birthday" and "name" sub-elements and 
- put those objects to main list.

Node: datetime scalar-mapping MUST consists of three elements, divided by "|":

    "datetime|default-value|format-string-in-C-strptime()-function-syntax"
    
Default value MUST correspond to format string

## Building list-of-objects-with-lists from xml string
 
```python
from nkit4py import Xml2VarBuilder

mapping = ["/person",
    {
        "/address/city -> cities": ["/", "string"]
        # or:
        # "/address -> cities": ["/city", "string"]
        # or:
        # "/ -> cities": ["/address/city", "string"]
        "/photos": ["/*", "string"],
        "/name": "string"
    }
]

mappings = {"persons": mapping}

builder = Xml2VarBuilder(mappings)
builder.feed(xml_string)
result = builder.end()
persons = result["persons"]
```

Value of persons:

```json
[
  {
    "photos": ["img1","img2","img3"],
    "cities": [
      "New York", 
      "Boston"
    ], 
    "name": "Jack"
  }, 
  {
    "photos": ["img3","img4"],
    "cities": [
      "Moscow", 
      "Tula"
    ], 
    "name": "Boris"
  }
]
```

As you can see, you can include list- or object-mappings in each other. List-mapping can contain
list- or object-submapping and vise-versa. Also, it is possible to use '*' char in XPath.


## Creating keys in object for non-existent xml elements 


```python
from nkit4py import Xml2VarBuilder

mapping = ["/person",
    {
        "/photos": ["/*", {
            "/ -> url" : "string",
            "/width": "integer|0", # element "width" doesn't exist in xml, 
                                   # but because of default value in scalar-mapping "integer|0"
                                   # key "width" will be created with this default value
            "/height": "integer|0" # element "height" doesn't exist in xml, 
                                   # but because of default value in scalar-mapping "integer|0"
                                   # key "height" will be created with this default value
        }],
        "/name": "string"
    }
]

mappings = {"persons": mapping}

builder = Xml2VarBuilder(mappings)
builder.feed(xml_string)
result = builder.end()
persons = result["persons"]
```

Value of persons:

```json
[
  {
    "photos": [
      {
        "url": "img1", 
        "width": 0, 
        "height": 0
      }, 
      {
        "url": "img2", 
        "width": 0, 
        "height": 0
      }, 
      {
        "url": "img3", 
        "width": 0, 
        "height": 0
      }
    ], 
    "name": "Jack"
  }, 
  {
    "photos": [
      {
        "url": "img3", 
        "width": 0, 
        "height": 0
      }, 
      {
        "url": "img4", 
        "width": 0, 
        "height": 0
      }
    ], 
    "name": "Boris"
  }
]
```

## Using attribute values to generate Dict keys

Suppose, we have this XML:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<GTSResponse>
    <Record>
        <Field name="TITLE">Empire Burlesque</Field>
        <Field name="ARTIST">Bob Dylan</Field>
        <Field name="COUNTRY">USA</Field>
        <Field name="COMPANY">Columbia</Field>
        <Field name="PRICE">10.90</Field>
        <Field name="YEAR">1985</Field>
    </Record>
    <Record>
        <Field name="TITLE">Hide your heart</Field>
        <Field name="ARTIST">Bonnie Tyler</Field>
        <Field name="COUNTRY">UK</Field>
        <Field name="COMPANY">CBS Records</Field>
        <Field name="PRICE">9.90</Field>
        <Field name="YEAR">1988</Field>
    </Record>
</GTSResponse>
```

And we want to get such structure:

```json
{ "records": [
    { "TITLE": "Empire Burlesque",
      "ARTIST": "Bob Dylan",
      "COUNTRY": "USA",
      "COMPANY": "Columbia",
      "PRICE": "10.90",
      "YEAR": "1985"
    },
    { "TITLE": "Hide your heart",
      "ARTIST": "Bonnie Tyler",
      "COUNTRY": "UK",
      "COMPANY": "CBS Records",
      "PRICE": "9.90",
      "YEAR": "1988"
    }
  ]
}
```

We can do it with this mapping:


```json
["/Record", {"/Field -> @name": "string"} ]
```

Here object key definition "/Field -> @name" contain key alias '@name',
that means that module will use 'name' attribute values for Dict keys.

Full example:

```python
import nkit4py

mapping = ["/Record", {"/Field -> @name": "string"} ]

mappings = {"main": mapping}

builder = Xml2VarBuilder(mappings)
builder.feed(xml_string)
result = builder.end()

result = result["main"]

```

## Building data structures from big XML source, reading it chunk by chunk

This example requires Tornado server to be installed (pip install tornado)

```python
import nkit4py, json
import tornado, tornado.ioloop
from tornado.web import RequestHandler, Application
from tornado.httpclient import HTTPRequest, AsyncHTTPClient

class XmlDownloader:
    def __init__(self):
        self.http = AsyncHTTPClient()

    @tornado.gen.coroutine
    def run(self, url, mapping):
        builder = nkit4py.Xml2VarBuilder({"any_mapping_name": mapping})
        def on_chunk(chunk): # this callback will be called many times
            builder.feed(chunk)
            lst = builder.get("any_mapping_name") # get currently constructed list
            print len(lst)
            # del lst[:] # uncomment this to clear list every time
        yield self.http.fetch(HTTPRequest(url, streaming_callback=on_chunk))
        raise tornado.gen.Return(builder.end()["any_mapping_name"])

class MainHandler(RequestHandler):
    @tornado.gen.coroutine
    def get(self):
        downloader = XmlDownloader()
        result = yield downloader.run(
        	"http://rt.com/rss/", 
            ["/channel/item", {
                "/title": "string",
                "/content:encoded": "string",
                "/description": "string"
            }])
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write(json.dumps(result, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    app = Application([tornado.web.url(r"/", MainHandler),])
    app.listen(8888)
    tornado.ioloop.IOLoop.current().start()
```


## Options

With options you can tune some aspects of conversion:
 
```python
mapping = ["/person",
    {
        "/*": "string"
    }
]

mappings = {"persons": mapping}

options = {
    "trim": True,
    "white_spaces": " \t\n\r",
    "unicode": False
}

builder = Xml2VarBuilder(options, mappings)
builder.feed(xml_string)
result = builder.end()
persons = result["persons"]
```

Following options are supported:

- "trim": Trim out whitespaces at the beginning and at ending of strings.
   Boolean. True or False. Default is False.
- "white_spaces": Characters which are must be considered as white spaces.
   String. Default - "\t\n\r ", i.e. tab, new line, carriage return and space.
- "unicode": Boolean flag that defines type of created python textual data.
   True - unicode, False - string. Default - True.
- "attrkey": Special key for object-mapping for all element attributes. See example below.

### 'attrkey' option

If defined, this option cause nkit4py module to collect all element attributes for all
object-mappings (if corresponding elements has attributes, of course).

Example:

```python
options = {"attrkey": "$"}

mapping = ["/person",
    {
        "/name": "string",
        "/married": {"/ -> Now": "string"} # Elements '/person/married' has attributes
                                           # Module will collect them
    }
]

mappings = {"married_info": mapping}

builder = Xml2VarBuilder(options, mappings)
builder.feed(xml_string)
result = builder.end()
married_info = result["married_info"]
```

Value of married_info:

```json
[
  {
    "married": {
      "Now": "Yes", 
      "$": { // <- Key '$' will hold all attributes for '/person/married' elements
        "firstTime": "No"
      }
    }, 
    "name": "Jack"
  }, 
  {
    "married": {
      "Now": "Yes", 
      "$": {
        "firstTime": "Yes"
      }
    }, 
    "name": "Boris"
  }
]
```


## Notes

Possible scalar types:

    - string
    - unicode
    - integer
    - number
    - datetime
    - boolean
    
Scalar types can be followed by '|' sign and default value

**datetime** type MUST be followed by '|' sign, default value,
another '|' sign and format string. See 
[man strptime](http://linux.die.net/man/3/strptime) for datetime formatting
syntax. Default value of datetime MUST correspond to format string.

Path in mapping specifications are very simple XPath now. Only

    /path/to/element
    /path/to/element/with/optional/@attribute
    /paths/to/element/with/*/sign
    /paths/to/element/with/*/sign/with/optional/@attribute

paths are supported.

Python object keys get their names from the last element name in the path.
If you want to change key names, use this notation:

    "/path/to/element -> newKeyName": ...
    "/path/to/element/@attribute -> newKeyName": ...

# Python data to XML conversion

## Quick start

```python
import nkit4py

ENCODING = "UTF-8"
# ENCODING = "windows-1251"
DATA = {
    "$": {"p1": "в1&v2\"'", "p2": "v2"},
    "_": "Hello(Привет) world(мир)",
    "int_число": 1,
    "true": True,
    "false": False,
    "float": 1.123456789,
    "cdata1": "text < > & \" '",
    "cdata2": "%^&*()-=+ < > & \" '",
    "list": [[1, 2], 2, 3],
    "datetime": datetime.now(),
    "dict": {
        "$": {"a1": "V1", "a2": "V2"},
        "int": 1,
        "float": 1.11234567891234,
        "sub_string": "text < > & \" '",
        "list": [[1], 2, 3]
    }
}

OPTIONS = {
    "rootname": "ROOT",
    "itemname": "item",
    "encoding": ENCODING,
    "xmldec": {
        "version": "1.0",
        "standalone": True,
    },
    "pretty": {
        "indent": "  ",
        "newline": "\n",
    },
    "attrkey": "$",
    "textkey": "_",
    "cdata": ["cdata1", "cdata2"],
    "float_precision": 10,
    "date_time_format": "%Y-%m-%d %H:%M:%S",
    "bool_true": "Yes",
    "bool_false": "No"
}

print nkit4py.var2xml(DATA, OPTIONS)
```

Output:

```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ROOT p2="v2" p1="в1&amp;v2&quot;&apos;">
  <false>No</false>
  <float>1.1234567890</float>
  <cdata2><![CDATA[%^&*()-=+ < > & " ']]></cdata2>
  <cdata1><![CDATA[text < > & " ']]></cdata1>
  <datetime>2014-11-12 20:13:34</datetime>
  <true>Yes</true>
  <int_число>1</int_число>
  <list>
    <item>1</item>
    <item>2</item>
  </list>
  <list>2</list>
  <list>3</list>
  <dict a1="V1" a2="V2">
    <int>1</int>
    <list>
      <item>1</item>
    </list>
    <list>2</list>
    <list>3</list>
    <float>1.1123456789</float>
    <sub_string>text &lt; &gt; &amp; &quot; &apos;</sub_string>
  </dict>
  Hello(Привет) world(мир)
</ROOT>
```

## Options for var2xml

Following options are supported:

- **rootname**: name of root element;
- **itemname**: default element name for Python list items. Default - 'item';
- **encoding**: "UTF-8" or some other encodings (see */deps/nkit/src/encoding/langs.inc* for list of supported encodings).
Default - "UTF-8";
- **xmldec**: XML declaration. Default - NO XML declaration. Sub-options:

    - *version*: xml version;
    - *standalone*: True or False;

- **pretty**: pretty XML - with indents and custom newlines. Default - NO pretty print, i.e. print XML in single line.
Sub-options:

    - *indent*: any string for indentation;
    - *newline*: any string for line ending (default '\n');
    
- **attrkey**: any string for DICT key name, that holds attributes for element. Default '$';
- **textkey**: any string for DICT key name, that holds text for element. Default '_';
- **cdata**: list of key names whose values mast be print to XML string as CDATA Default - empty list;
- **float_precision**: for float numbers - number of symbols after '.' to be printed Default - 2;
- **date_time_format**: format string of Date objects
See [man strftime](http://www.cplusplus.com/reference/ctime/strftime/?kw=strftime) for datetime formatting syntax.
Default "%Y-%m-%d %H:%M:%S";
- **bool_true**: representation for 'true' boolean value. Default '1';
- **bool_false**: representation for 'false' boolean value. Default '0';
- **priority**: list of element names. All DICT keys are printed to XML in order they
enumerated in this list. Other DICT keys are printed in unexpected order.
- **unicode**: If True, creates unicode string instead of simple string. Default - False

If NO *rootname* has been provided then *xmldec* will no effect.

If NO *rootname* has been provided and **data** is Object (not Array) then *attrkey* option will no effect for root Object.

If **data** is Array then *itemname* will be used as element name for its items.

# Python version support

	==2.6
	==2.7
	>=3.3

# Change log

- 2.4.0 (2016-05-16):
  - Now we can use XML attribute values to generate Dict keys

- 2.3.6 (2016-04-03):
    - Python version >= 3.3 support

- 2.3:
  - New 'priority' option for nkit4py.var2xml()
	- New 'unicode' option for nkit4py.var2xml()
  - New class AnyXml2VarBuilder for converting XML without mapping
	- AnyXml2VarBuilder.root_name() method
	- Support of compatible python structures - any kind of sequencies and dics
	- Support of ordered dicts
	- Bug fixes

- 2.2:
    - Options changes for nkit4py.var2xml(): standalone 'encoding' option.
      In previous version 'encoding' option was in 'xmldec'
    - New 'attrkey' option for nkit4py.Xml2VarBuilder for collecting all element attributes
      
- 2.1:
    - nkit4py.var2xml() method for converting Python data to XML
    
- 2.0:
    - Multi-mappings and options for nkit4py.Xml2VarBuilder class
    
- 1.X:
    - nkit4py.Xml2VarBuilder class for converting XML to Python data

# Author

Boris T. Darchiev (boris.darchiev@gmail.com)

On github: https://github.com/eye3

Any feedback or pull request are welcome!

# Travis

[![Build Status](https://travis-ci.org/eye3/nkit4py.svg?branch=master)](https://travis-ci.org/eye3/nkit4py)
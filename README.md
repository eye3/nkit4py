Introduction
============

nkit4nodejs - is a [nkit](https://github.com/eye3/nkit.git) C++ library port to 
Python. There is the same port to Node.js - [nkit4nodejs](https://github.com/eye3/nkit4nodejs.git).

Currently, only an XML to Python object or list converter and filter
is exported to Python from nkit library.

With version 1.0 you can:
 
- Create Python data structures, which are different from the structure 
  of XML source.
  
- Create multiple Python structures from one XML source.

- Explicitly identify those elements and attributes in XML source that you
  want to use for building Python data structures.
  Thus, it's possible to filter out unnecessary XML-data.
  
- Explicitly define Python type of scalar data, fetched from XML source.
  Integers, numbers, strings, datetimes and booleans are supported.

- With extra options you can tune some aspects of conversion:
	- trim out white spaces
	- explicitly define white space characters
	- choose unicode or string type for text scalar values

Conversion is carried out using SAX parser Expat, so it's fast and uses less 
memory when parsing huge XML files.

Module supports not only native Expat XML encodings, but also many others
(see /deps/nkit/src/vx/encodings_inc_gen.cpp)


Installation
============

On Linux & Mac OS
-----------------

    pip install nkit4py --pre

On Windows
----------

Library compiles on MSVS Express version >= 2012.

For MSVS 2012:

    SET VS90COMNTOOLS=%VS110COMNTOOLS%

    pip install nkit4py --pre

For MSVS 2013:

    SET VS90COMNTOOLS=%VS120COMNTOOLS%

    pip install nkit4py --pre


Usage
=====

Suppose, we have this xml string:

```xml
<?xml version="1.0"?>
<any_name>
	<academy>
		<title>Delhi Academy Of Medical Sciences</title>
		<link>http://www.damsdelhi.com/dams.php</link>
	</academy>
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

Quick start:
-----------------------------------------

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

Xml2VarBuilder class uses 'mappings', i.e. some directives about how to perform
conversion. Mappings are written in JSON (or they can be JSON-compatible
python structures, as in our example), they describe conversion process and
final structures of python data. Our example contains two mappings:
'list_of_strings' and 'list_of_lists_of_strings'.
This means that after conversion we expect to get two data structures:
list of all phones of all persons, and list of phone lists for each person.

First mapping - ["/person/phone", "string"]. It is enclosed in [] brackets.
This means that we expect to get python list. This mapping type called
'list-mapping'.
(Braces - {} - means that we want to get python objects. Not in this example - see below).
First item of list-mapping defines the XPath where we want to find
data. Second item defines a sub-mapping, which in our case is a scalar-submapping.
Scalar submapping contains information about type of data we want to get
('string' in our case). 
During conversion module will find all elements at path "/person/phone",
convert their values to python unicode and put them into python list.

Second mapping - ["/person", ["/phone", "string"]] is another list-mapping, but
first item points to "/person" XPath, and second item is list-submapping.
List-submapping also contains two elements: sub-xpath and another submapping
('string' scalar-sabmapping in our case). Sub-xpath MUST be continuation of
parent mapping xpath.
During conversion module will find all "person" elements and for each "person"
element it will find all "phone" sub-elements, convert their values to python
unicode and put them into python list, which in turn will be placed to main list.

Each mapping placed in mappings with some user defined name. This name will be
used in the future to get actual data from result. In our case thees names are:
'list_of_strings' and 'list_of_lists_of_strings'.

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
structures in one variable ("result" in our case):

```python
result = builder.end()
```

Now we can get our structures by their mapping names:

```python
list_of_strings = result["list_of_strings"]
list_of_lists_of_strings = result["list_of_lists_of_strings"]
```

Building simple object from xml string (last 'person' xml element will be used):
--------------------------------------------------------------------------------

```python
from nkit4py import Xml2VarBuilder

mappings = {"last_person":
    
    {   # <- opening brace for object-mapping
    
        "/person/name -> lastPersonName": "string|Captain Nemo",
        "/person/married/@firstTime -> lastPersonIsMarriedFirstTime":
            "boolean|True",
        "/person/age": "integer"
    
    }   # <- closing brace of object-mapping
}
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
Default values for scalars are work only in object-mappings, not in
list-mappings.



Building list-of-objects from xml string:
----------------------------------------------------
 
```python
from nkit4py import Xml2VarBuilder

mapping = ["/person",
    {
        "/birthday": "datetime|Fri, 22 Aug 2014 13:59:06 +0000|%a, %d %b %Y %H:%M:%S %z",
        "/name": "string"
    }
]

builder = Xml2VarBuilder({"persons": mapping})
builder.feed(xml_string)
result = builder.end()
result = result["persons"]
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

Now we use list-mapping and object-submapping. Module will find all person
elements, for each person element construct object from their "birthday" and
"name" sub-elements and put those objects to main list.

Node: datetime scalar-mapping MUST consists of three elements, devided by "|":

    "datetime|default-value|format-string-in-C-strptime()-function-syntax"
    
Default value MUST correspond to format string

Building list-of-objects-with-lists from xml string:
----------------------------------------------------
 
```python
from nkit4py import Xml2VarBuilder

#  Here mapping is list, described by /path/to/element and list item description.
#  List item is described as 'object' sub-mapping.
#  This 'object' sub-mapping described by set of mappings, each containing
#  key definition and sub-mapping or scalar.
#  Keys are described by "/sub/path -> optionalKeyName".
#  If optionalKeyName doesn't provided, then last element name in "/sub/path"
#  will be used for key name
#  Scalar definition may have optional "...|defaultValue"
#  'datetime' scalar definition MUST contain default value and formatting string
mapping = ["/person",
    {
        "/address -> cities": ["/city", "string"]
        "/photos": ["/*", "string"],
        "/mame": "string"
    }
]

builder = Xml2VarBuilder({"persons": mapping})
builder.feed(xmlString) # can be more than one call to feed(xmlChunk) method
result = builder.end()
result = result["persons"]
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
	
To build list-of-objects from big XML source, reading it chunk by chunk
------------------------------------------------------------------------

This example requires Tornado server to be installed (pip install tornado)

```python
import nkit4py, json
import tornado, tornado.ioloop
from tornado.web import RequestHandler, Application
from tornado.httpclient import HTTPRequest, AsyncHTTPClient

class XmlDownloader:
    def __init__(self, ):
        self.http = AsyncHTTPClient()

    @tornado.gen.coroutine
    def run(self, url, mapping):
        builder = nkit4py.Xml2VarBuilder({"any_mapping_name": mapping})
        def on_chunk(chunk): # this callback will be called many times
            builder.feed(chunk)
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


Notes
=====

Possible scalar types:

    - string
    - integer
    - number
    - datetime
    - boolean
    
Scalar types can be followed by '|' sign and default value

**datetime** type MUST be followed by '|' sign, default value,
another '|' sign and format string. See 
[man strptime](http://linux.die.net/man/3/strptime) for datetime formatting
syntax. Default value of datetime must correspond to format string.

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

Author
======

Boris T. Darchiev (boris.darchiev@gmail.com)

On github: https://github.com/eye3

Any feedback or pull request are welcome!

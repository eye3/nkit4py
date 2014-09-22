#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json, os, sys
from datetime import *
from nkit4py import Xml2VarBuilder


# ------------------------------------------------------------------------------
class DatetimeEncoder(json.JSONEncoder):
    def default( self, obj ):
        if isinstance( obj, datetime ):
            return obj.strftime("%Y-%m-%d %H:%M:%S")
        if isinstance( obj, date ):
            return obj.strftime("%Y-%m-%d")
        if isinstance( obj, time ):
            return obj.strftime("%H:%M:%S")

        return json.JSONEncoder.default( self, obj )


def to_json(v):
    return json.dumps(v, indent=2, ensure_ascii=False, cls=DatetimeEncoder)


def pring_json(v):
    print "-------------------------------------"
    print to_json(v)


path = os.path.dirname(os.path.realpath(__file__))

xml_path = path + "/data/sample.xml"
f2 = open( xml_path, 'r' )
xmlString = f2.read()

# ------------------------------------------------------------------------------
# Here mapping is list, described by '/path/to/element' and list-item-description.
# List item here is a 'string' scalar.
# Scalar definition contains type name and optional default value.
mapping = '["/person/phone", "string"]'
# mapping = '["/person/phone", "string|optionalDefaultValue"]'

builder = Xml2VarBuilder(mapping)
builder.feed(xmlString)
result = builder.end()

etalon = [ '+122233344550',
           '+122233344551',
           '+122233344553',
           '+122233344554']

if result != etalon:
    pring_json(result)
    pring_json(etalon)
    print "Error #1"
    sys.exit(1)

# ------------------------------------------------------------------------------
#  build simple object from xml string (last 'person' element will be used)
# 
#  Here mapping is object, described by set of mappings, each containing
#  key definition and scalar definition.˛
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

# ------------------------------------------------------------------------------
#  build list-of-lists-of-strings from xml string
# 
#  Here mapping is list, described by /path/to/element and list item
#  description. List item is described as 'list' sub-mapping, described 
#  by sub-path and'string' scalar definition
mapping = '["/person", ["/phone", "string"]]'

builder = Xml2VarBuilder(mapping)
builder.feed(xmlString) # can be more than one call to feed(xmlChunk) method
result = builder.end()

etalon = [ [ '+122233344550', '+122233344551'],
    [ '+122233344553', '+122233344554']]

if result != etalon:
    pring_json(result)
    pring_json(etalon)
    print "Error #2"
    sys.exit(1)

# ------------------------------------------------------------------------------
#  build list-of-objects-with-lists from xml string
# 
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
        "/birthday": "datetime|Fri, 22 Aug 2014 13:59:06 +0000|%a, %d %b %Y %H:%M:%S %z",
        "/phone -> phones": ["/", "string"],
        "/address -> cities": ["/city", "string"],
            // same as "/address/city -> cities": ["/", "string"]
        "/photos": ["/*", "string"],
        "/married/@firstTime -> isMerriedFirstTime": "boolean"
    }
]"""

builder = Xml2VarBuilder(mapping)
builder.feed(xmlString) # can be more than one call to feed(xmlChunk) method
result = builder.end()

etalon = [
    {
        "isMerriedFirstTime": False,
        "phones": [ '+122233344550', '+122233344551' ],
        "photos": ["img1","img2","img3"],
        "birthday": datetime(1979, 3, 28, 12, 13, 14),
        "cities": [ 'New York', 'Boston' ],
    },
    {
        "isMerriedFirstTime": True,
        "phones": [ '+122233344553', '+122233344554' ],
        "photos": ["img3","img4"],
        "birthday": datetime(1970, 8, 31, 2, 3, 4),
        "cities": [ 'Moscow', 'Tula' ],
    }
]

if result != etalon:
    pring_json(result)
    pring_json(etalon)
    print "Error #3"
    sys.exit(1)

# ------------------------------------------------------------------------------
mapping = """["/person",
    {
        "/*": "string"
    }
]"""

builder = Xml2VarBuilder(mapping)
builder.feed(xmlString) # can be more than one call to feed(xmlChunk) method
result = builder.end()

etalon = [
  {
    "name": "Jack",
    "photos": "\n\t\t\t\n\t\t\t\n\t\t\t\n\t\t",
    "age": "33",
    "married": "Yes",
    "phone": "+122233344551",
    "birthday": "Wed, 28 Mar 1979 12:13:14 +0300",
    "address": "\n\t\t\t\n\t\t\t\n\t\t\t\n\t\t\t\n\t\t",
    "empty": ""
  },
  {
    "name": "Boris",
    "photos": "\n\t\t\t\n\t\t\t\n\t\t",
    "age": "34",
    "married": "Yes",
    "phone": "+122233344554",
    "birthday": "Mon, 31 Aug 1970 02:03:04 +0300",
    "address": "\n\t\t\t\n\t\t\t\n\t\t\t\n\t\t\t\n\t\t",
    "empty": ""
  }
]

if result != etalon:
    pring_json(result)
    pring_json(etalon)
    print "Error #4"
    sys.exit(1)


# ------------------------------------------------------------------------------
mapping = """["/person", {
   	"/key_for_default_value": "string|default_value",
    "/non_existing_key": "string"
    }
]"""

builder = Xml2VarBuilder(mapping)
builder.feed(xmlString) # can be more than one call to feed(xmlChunk) method
result = builder.end()

etalon = [
  {
    "key_for_default_value": "default_value"
  },
  {
    "key_for_default_value": "default_value"
  }
]

if result != etalon:
    pring_json(result)
    pring_json(etalon)
    print "Error #5"
    sys.exit(1)


# ------------------------------------------------------------------------------
mapping = """["/person", {
    "/empty": "string"
    }
]"""

builder = Xml2VarBuilder(mapping)
builder.feed(xmlString) # can be more than one call to feed(xmlChunk) method
result = builder.end()

etalon = [
  {
    "empty": ""
  },
  {
    "empty": ""
  }
]

if result != etalon:
    pring_json(result)
    pring_json(etalon)
    print "Error #6"
    sys.exit(1)


print "ok"
sys.exit(0)

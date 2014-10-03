#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import os
import sys
from datetime import *
from nkit4py import Xml2VarBuilder, DatetimeJSONEncoder


# ------------------------------------------------------------------------------
def to_json(v):
    return json.dumps(v, indent=2, ensure_ascii=False, cls=DatetimeJSONEncoder)


# ------------------------------------------------------------------------------
def pring_json(v):
    print "-------------------------------------"
    print to_json(v)


# ------------------------------------------------------------------------------
def make_pairs(context):
    MAPPING_SUFFIX = "_mapping"
    ETALON_SUFFIX = "_etalon"
    context.update(globals())
    etalons = {}
    mappings = {}
    for mapping_key, mapping in context.items():
        if mapping_key.endswith(MAPPING_SUFFIX):
            structure = mapping_key[: -1 * len(MAPPING_SUFFIX)]
            etalon = context.get(structure + ETALON_SUFFIX)
            if etalon:
                etalons[structure] = etalon
                mappings[structure] = mapping
    return mappings, etalons


# ------------------------------------------------------------------------------
path = os.path.dirname(os.path.realpath(__file__))

xml_path = path + "/data/sample.xml"
f = open(xml_path, 'r')
xml_string = f.read()

# ------------------------------------------------------------------------------
# Building a lists-of-strings from xml string
#
# Here mapping is list, described by '/path/to/element' and list-item-description.
# List item here is a 'string' scalar.
# Scalar definition contains type name and optional default value.

list_of_strings_mapping = ["/person/phone", "string"]
list_of_strings_etalon = [
    '+122233344550',
    '+122233344551',
    '+122233344553',
    '+122233344554']

# ------------------------------------------------------------------------------
# Building a list-of-lists-of-strings from xml string
#
# Here mapping is list, described by /path/to/element and list item
# description. List item is described as 'list' sub-mapping, described
# by sub-path and'string' scalar definition
list_of_list_of_strings_mapping = ["/person", ["/phone", "string"]]
list_of_list_of_strings_etalon = [['+122233344550', '+122233344551'],
                                  ['+122233344553', '+122233344554']]

# ------------------------------------------------------------------------------
# Building a simple object from xml string (last 'person' element will be used)
#
# Here mapping is object, described by set of mappings, each containing
# key definition and scalar definition.˛
# Keys are described by "/sub/path -> optionalKeyName".
# If optionalKeyName doesn't provided, then last element name in /sub/path
# will be used for key name.
# Scalar definition may have optional "...|defaultValue"

single_object_mapping = {
    "/person/name -> lastPersonName": "string|Captain Nemo",
    "/person/married/@firstTime -> lastPersonIsMarriedFirstTime":
        "boolean|true",
    "/person/age": "integer"
}

single_object_etalon = {
    "age": 34,
    "lastPersonName": "Boris",
    "lastPersonIsMarriedFirstTime": True
}

# ------------------------------------------------------------------------------
# Building list-of-objects-with-lists from xml string
#
# Here mapping is list, described by /path/to/element and list item description.
#  List item is described as 'object' sub-mapping.
#  This 'object' sub-mapping described by set of mappings, each containing
#  key definition and sub-mapping or scalar.
#  Keys are described by "/sub/path -> optionalKeyName".
#  If optionalKeyName doesn't provided, then last element name in "/sub/path"
#  will be used for key name
#  Scalar definition may have optional "...|defaultValue"
#  'datetime' scalar definition MUST contain default value and formatting string

list_of_objects_with_lists_mapping = ["/person",
                                      {
                                          "/birthday": "datetime|Fri, 22 Aug 2014 13:59:06 +0000|%a, %d %b %Y %H:%M:%S %z",
                                          "/phone -> phones": ["/", "string"],
                                          "/address -> cities": ["/city",
                                                                 "string"],
                                          # same as "/address/city -> cities": ["/", "string"]
                                          "/photos": ["/*", "string"],
                                          "/married/@firstTime -> isMerriedFirstTime": "boolean"
                                      }
]

list_of_objects_with_lists_etalon = [
    {
        "isMerriedFirstTime": False,
        "phones": ['+122233344550', '+122233344551'],
        "photos": ["img1", "img2", "img3"],
        "birthday": datetime(1979, 3, 28, 12, 13, 14),
        "cities": ['New York', 'Boston'],
    },
    {
        "isMerriedFirstTime": True,
        "phones": ['+122233344553', '+122233344554'],
        "photos": ["img3", "img4"],
        "birthday": datetime(1970, 8, 31, 2, 3, 4),
        "cities": ['Moscow', 'Tula'],
    }
]

# ------------------------------------------------------------------------------
# Testing default values

testing_default_values_mapping = ["/person", {
    "/key_for_default_value": "string|default_value",
    "/non_existing_key": "string"
}
]

testing_default_values_etalon = [
    {
        "key_for_default_value": "default_value"
    },
    {
        "key_for_default_value": "default_value"
    }
]

# ------------------------------------------------------------------------------
# Testing empty values

testing_empty_values_mapping = ["/person", {
    "/empty": "string"
}
]

testing_empty_values_etalon = [
    {
        "empty": ""
    },
    {
        "empty": ""
    }
]

# ------------------------------------------------------------------------------
# Testing new object creation

testing_new_object_creation_mapping = [
    "/person", {
        "/empty": {
            "/ -> k1": "string",
            "/ -> k2": "string"
        }
    }
]

testing_new_object_creation_etalon = [
    {
        "empty": {
            "k2": "",
            "k1": ""
        }
    },
    {
        "empty": {
            "k2": "",
            "k1": ""
        }
    }
]
# ------------------------------------------------------------------------------
mappings, etalons = make_pairs({})

builder = Xml2VarBuilder(mappings)
builder.feed(xml_string)
res = builder.end()

counter = 0
for mapping_name, etalon in etalons.items():
    counter += 1
    if res[mapping_name] != etalon:
        print "ETALON of %s:" % mapping_name
        pring_json(etalon)
        print "RESULT of %s:" % mapping_name
        pring_json(res[mapping_name])
        print "Error #1.%d" % counter
        sys.exit(1)

# ------------------------------------------------------------------------------
# ------------------------------------------------------------------------------
# Testing paths with '*'

mapping = ["/person",
           {
               "/*": "string"
           }
]

mapping_name = "testing_paths_with_star"

options = {"trim": True}

builder = Xml2VarBuilder(options, {mapping_name: mapping})
builder.feed(xml_string)
result = builder.end()
result = result[mapping_name]

etalon = [
    {
        "name": "Jack",
        "photos": "",
        "age": "33",
        "married": "Yes",
        "phone": "+122233344551",
        "birthday": "Wed, 28 Mar 1979 12:13:14 +0300",
        "address": "",
        "empty": ""
    },
    {
        "name": "Boris",
        "photos": "",
        "age": "34",
        "married": "Yes",
        "phone": "+122233344554",
        "birthday": "Mon, 31 Aug 1970 02:03:04 +0300",
        "address": "",
        "empty": ""
    }
]

if etalon != result:
    pring_json(etalon)
    pring_json(result)
    print "Error #2.1"
    sys.exit(1)

# ------------------------------------------------------------------------------
multi_mapping = {
    "academy": {
        "/academy/title": "string",
        "/academy/link": "string"
    },
    "persons": ["/person", {
        "/*": "string"
    }]
}

options = {"trim": True, "unicode": False}

builder = Xml2VarBuilder(options, multi_mapping)
builder.feed(xml_string)
result = builder.end()
academy = result["academy"]
persons = result["persons"]

academy_etalon = {
    "link": "http://www.damsdelhi.com/dams.php",
    "title": "Delhi Academy Of Medical Sciences"
}

persons_etalon = [
    {
        "name": "Jack",
        "photos": "",
        "age": "33",
        "married": "Yes",
        "phone": "+122233344551",
        "birthday": "Wed, 28 Mar 1979 12:13:14 +0300",
        "address": "",
        "empty": ""
    },
    {
        "name": "Boris",
        "photos": "",
        "age": "34",
        "married": "Yes",
        "phone": "+122233344554",
        "birthday": "Mon, 31 Aug 1970 02:03:04 +0300",
        "address": "",
        "empty": ""
    }
]

if academy_etalon != academy:
    pring_json(academy)
    pring_json(academy_etalon)
    print "Error #2.2"
    sys.exit(1)

if persons_etalon != persons:
    pring_json(persons)
    pring_json(persons_etalon)
    print "Error #2.3"
    sys.exit(1)

print "ok"
sys.exit(0)

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json, datetime, os

from nkit4py import Xml2VarBuilder
from datetime_json_encoder import DatetimeEncoder

path = os.path.dirname(os.path.realpath(__file__))

spec_file = path + "/data/list_of_lists.json";
xml_path = path + "/data/sample.xml";

f1 = open( spec_file, 'r' )
f2 = open( xml_path, 'r' )

spec = f1.read()
xml = f2.read()

b = Xml2VarBuilder(spec)
b.feed(xml)
o = b.end()

j = json.dumps( o, indent=2, ensure_ascii=False, cls=DatetimeEncoder )

print(j)

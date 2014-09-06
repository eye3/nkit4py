#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json, datetime, os

from datetime_json_encoder import DatetimeEncoder
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from nkit4py import Xml2VarBuilder

path = os.path.dirname(os.path.realpath(__file__))

xml_path = path + "/data/sample.xml";
f2 = open( xml_path, 'r' )
xml = f2.read()

mapping = """["/person",
    {
        "/birthday": "datetime|1970-01-01|%Y-%m-%d",
        "/phone -> phones": ["/", "string"],
        "/address -> cities": ["/city", "string"],
            // same as "/address/city -> cities": ["/", "string"]
        "/married/@firstTime -> isMerriedFirstTime": "boolean"
    }
]"""

class WebServer(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('content-type', 'application/json; charset=utf-8')
        self.end_headers()
        gen = Xml2VarBuilder(mapping)
        gen.feed(xml)
        target = gen.end()
        j = json.dumps( target, indent=2,
                        ensure_ascii=False, cls=DatetimeEncoder )
        self.wfile.write(j)

webserver = HTTPServer(('0.0.0.0', 8080 ), WebServer)
webserver.serve_forever()

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json, datetime, os

from datetime_json_encoder import DatetimeEncoder
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from nkit4py import Xml2VarBuilder

from consts import spec_str, xml_str

path = os.path.dirname(os.path.realpath(__file__))

spec_file = path + "/data/list_of_lists.json";
xml_path = path + "/data/sample.xml";

f1 = open( spec_file, 'r' )
f2 = open( xml_path, 'r' )

spec = f1.read()
xml = f2.read()

class WebServer(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('content-type', 'application/json; charset=utf-8')
        self.end_headers()
        gen = Xml2VarBuilder(spec)
        gen.feed(xml)
        target = gen.end()
        j = json.dumps( target, indent=2,
                        ensure_ascii=False, cls=DatetimeEncoder )
        self.wfile.write(j)

webserver = HTTPServer(('127.0.0.1', 8080 ), WebServer)
webserver.serve_forever()

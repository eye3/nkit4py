#!/usr/bin/env python
# -*- coding: utf-8 -*-

import tornado
from tornado.ioloop import IOLoop
from tornado.web import RequestHandler, Application
from tornado.httpserver import HTTPServer
from nkit4py import Xml2VarBuilder, AnyXml2VarBuilder,\
    DatetimeJSONEncoder, var2xml
import json
import os
import sys
if sys.version_info[0] == 2:
    import BaseHTTPServer
    SERVER = BaseHTTPServer
else:
    import http.server
    SERVER = http.server
from datetime import *


def read_file_text(file_path):
    f = open(file_path, 'r')
    text = f.read()
    f.close()
    return text


path = os.path.dirname(os.path.realpath(__file__))
NKIT_TEST_DATA_PATH = os.path.join(path, '../deps/nkit/test/data')
NKIT_TEST_DATA_PATH = os.path.normpath(NKIT_TEST_DATA_PATH)

attribute_as_key_sample = os.path.join(NKIT_TEST_DATA_PATH, 'attribute_as_key_sample.xml')
attribute_as_key_sample = read_file_text(attribute_as_key_sample)

attribute_as_key_mapping = os.path.join(NKIT_TEST_DATA_PATH, 'attribute_as_key_mapping.json')
attribute_as_key_mapping = read_file_text(attribute_as_key_mapping)
attribute_as_key_mapping = json.loads(attribute_as_key_mapping)
attribute_as_key_mapping = ["/Record", {"/Field -> @name": {
                                            "/ -> value": {
                                                "/ -> value": "string"
                                                }
                                            }
                                       }
                           ]
attribute_as_key_mappings = {"": attribute_as_key_mapping}

xml = read_file_text(path + "/data/sample.xml")

mapping = ["/person",
    {
        "/birthday": "datetime|Fri, 22 Aug 2014 13:59:06 +0000|%a, %d %b %Y %H:%M:%S %z",
        "/phone -> phones": ["/", "string"],
        "/address -> cities": ["/city", "string"],
        "/married/@firstTime -> isMerriedFirstTime": "boolean"
    }
]

mappings = {"": mapping}


def xml2var():
    gen = Xml2VarBuilder(mappings)
    gen.feed(xml)
    target = gen.end()[""]
    return json.dumps(target
                      , indent=2
                      , ensure_ascii=False
                      , cls= DatetimeJSONEncoder
    )


def xml2var_attribute_as_key():
    gen = Xml2VarBuilder(attribute_as_key_mappings)
    gen.feed(attribute_as_key_sample)
    target = gen.end()[""]
    return json.dumps(target
                      , indent=2
                      , ensure_ascii=False
                      , cls= DatetimeJSONEncoder
    )

def anyxml2var():
    options = {
        "trim": True,
        "ordered_dict": True
    }

    gen = AnyXml2VarBuilder(options)
    gen.feed(xml)
    target = gen.end()
    return json.dumps(target
                      , indent=2
                      , ensure_ascii=False
                      , cls= DatetimeJSONEncoder
    )


class Xml2VarHandler(RequestHandler):
    def get(self):
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write(xml2var())


class Xml2VarAttributeAsKeyHandler(RequestHandler):
    def get(self):
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write(xml2var_attribute_as_key())


class AnyXml2VarHandler(RequestHandler):
    def get(self):
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write(anyxml2var())


class Var2XmlHandler(RequestHandler):
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
        "bool_false": "No",
        "unicode": True
    }

    def get(self):
        self.set_header("Content-Type", "text/xml; charset=" + Var2XmlHandler.ENCODING)
        bytes = var2xml(Var2XmlHandler.DATA, Var2XmlHandler.OPTIONS)
#         print(type(bytes))
        self.write(bytes)


class RedirectHandler(RequestHandler):
    urls = ["http://localhost:8888/1/", "http://localhost:8888/2/"]
    counter = 0

    def get(self):
        RedirectHandler.counter += 1
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.set_header("X-Accel-Redirect", "/reproxy")
        self.set_header("X-Reproxy-URL",
            RedirectHandler.urls[RedirectHandler.counter % len(RedirectHandler.urls)])


class Handler1(RequestHandler):
    def get(self):
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write("{}")


class Handler2(RequestHandler):
    def get(self):
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write("[]")


def run_tornado():
    app = Application([
        tornado.web.url(r"/xml2var", Xml2VarHandler),
        tornado.web.url(r"/anyxml2var", AnyXml2VarHandler),
        tornado.web.url(r"/var2xml", Var2XmlHandler),
        tornado.web.url(r"/xml2var_attribute_as_key", Xml2VarAttributeAsKeyHandler),
        tornado.web.url(r"/1/", Handler1),
        tornado.web.url(r"/2/", Handler2),
        tornado.web.url(r"/redirect/", RedirectHandler)
    ])
    server = HTTPServer(app)
    server.bind(8888)
    server.start(3)  # 0 - forks one process per cpu
    IOLoop.current().start()


class WebServer(SERVER.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('content-type', 'application/json; charset=utf-8')
        self.end_headers()
        self.wfile.write(xml2var())

    def log_message(self, format, *args):
        return


def run_simple_server():
    webserver = SERVER.HTTPServer(('0.0.0.0', 8888), WebServer)
    webserver.serve_forever()


def get_date():
    import datetime
    return datetime.datetime(2014, 1, 1)


if __name__ == "__main__":
    # for i in range(1000):
    #     do_mapping()
    # print "ok"
    run_tornado()
    # run_simple_server()
    # print json.dumps([get_date()], indent=2,
    #                  ensure_ascii=False, cls=DatetimeJSONEncoder)

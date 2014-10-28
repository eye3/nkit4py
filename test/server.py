#!/usr/bin/env python
# -*- coding: utf-8 -*-

import tornado
from tornado.ioloop import IOLoop
from tornado.web import RequestHandler, Application
from tornado.httpserver import HTTPServer
from nkit4py import Xml2VarBuilder, DatetimeJSONEncoder
import json
import os
import BaseHTTPServer

path = os.path.dirname(os.path.realpath(__file__))

xml_path = path + "/data/sample.xml"
f2 = open(xml_path, 'r')
xml = f2.read()

mapping = ["/person",
    {
        "/birthday": "datetime|Fri, 22 Aug 2014 13:59:06 +0000|%a, %d %b %Y %H:%M:%S %z",
        "/phone -> phones": ["/", "string"],
        "/address -> cities": ["/city", "string"],
        "/married/@firstTime -> isMerriedFirstTime": "boolean"
    }
]

mappings = {"1": mapping}


def do_mapping():
    gen = Xml2VarBuilder(mappings)
    gen.feed(xml)
    target = gen.end()
    return json.dumps(target
                      , indent=2
                      , ensure_ascii=False
                      , cls= DatetimeJSONEncoder
    )


class MainHandler(RequestHandler):
    def get(self):
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write(do_mapping())


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
        tornado.web.url(r"/", MainHandler),
        tornado.web.url(r"/1/", Handler1),
        tornado.web.url(r"/2/", Handler2),
        tornado.web.url(r"/redirect/", RedirectHandler)
    ])
    server = HTTPServer(app)
    server.bind(8888)
    server.start(1)  # 0 - forks one process per cpu
    IOLoop.current().start()


class WebServer(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('content-type', 'application/json; charset=utf-8')
        self.end_headers()
        self.wfile.write(do_mapping())

    def log_message(self, format, *args):
        return


def run_simple_server():
    webserver = BaseHTTPServer.HTTPServer(('0.0.0.0', 8888), WebServer)
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

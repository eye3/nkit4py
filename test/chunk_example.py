#!/usr/bin/env python
# -*- coding: utf-8 -*-

import nkit4py, json
import tornado, tornado.ioloop
from tornado.web import RequestHandler, Application
from tornado.httpclient import HTTPRequest, AsyncHTTPClient

class XmlDownloader:
    def __init__(self, ):
        self.http = AsyncHTTPClient()

    @tornado.gen.coroutine
    def run(self, url, mapping):
        builder = nkit4py.Xml2VarBuilder(mapping)
        def on_chunk(chunk):
            builder.feed(chunk)
        yield self.http.fetch(HTTPRequest(url, streaming_callback=on_chunk))
        raise tornado.gen.Return(builder.end())

class MainHandler(RequestHandler):
    @tornado.gen.coroutine
    def get(self):
        downloader = XmlDownloader()
        result = yield downloader.run("http://rt.com/rss/", """
            ["/channel/item", {
                "/title": "string",
                "/content:encoded": "string",
                "/description": "string"
            }]
        """)
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write(json.dumps(result, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    app = Application([tornado.web.url(r"/", MainHandler),])
    app.listen(8888)
    tornado.ioloop.IOLoop.current().start()


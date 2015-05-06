import nkit4py
import json
import pycurl
import tornado
import tornado.ioloop
from tornado.web import RequestHandler, Application
from tornado.httpclient import HTTPRequest, AsyncHTTPClient


class XmlDownloader:
    def __init__(self):
        self.http = AsyncHTTPClient()

    def _set_read_timeout_callback(self, curl):
        curl.setopt(pycurl.LOW_SPEED_LIMIT, 1)
        curl.setopt(pycurl.LOW_SPEED_TIME, 10)

    @tornado.gen.coroutine
    def run(self, url, mapping):
        builder = nkit4py.Xml2VarBuilder({"any_mapping_name": mapping})

        def on_chunk(chunk): # <------this callback will be called many times
            builder.feed(chunk)
            current_data = builder.get("any_mapping_name")
#             print(len(current_data))

        yield self.http.fetch(HTTPRequest(url,
                        connect_timeout=40,
                        request_timeout=40,
                        streaming_callback=on_chunk,
                        prepare_curl_callback=self._set_read_timeout_callback
        ))

        raise tornado.gen.Return(builder.end()["any_mapping_name"])


class MainHandler(RequestHandler):
    URLS = [
        "http://www.1tv.ru/rss_item/id=216",
        "http://www.1tv.ru/rss_item/id=220"
    ]

    counter = 0

    @tornado.gen.coroutine
    def get(self):
        MainHandler.counter += 1
        print(MainHandler.counter)
        downloader = XmlDownloader()
        result = yield downloader.run(
            MainHandler.URLS[MainHandler.counter % 2],
            ["/channel/item", {
                "/title": "string",
                "/description": "string"
            }])
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write(json.dumps(result, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    app = Application([tornado.web.url(r"/", MainHandler),])
    app.listen(8888)
    tornado.ioloop.IOLoop.current().start()

import nkit4py, json
import tornado, tornado.ioloop
from tornado.web import RequestHandler, Application
from tornado.httpclient import HTTPRequest, AsyncHTTPClient


SPEC = [
    "/flats/offer", {
        "/id": "string",

        "/deal_type": "string",

        "/address": {
            "/region": "string",
            "/region/@type_attr -> region_type": "string",
            "/district": "string|",
            "/metro": "string",
            "/street": "string",
            "/house": "string",
            "/range": "string",
            "/range/@type_attr -> range_type": "string",
            "/highway": "string"
        }
    }
]


class XmlDownloader:
    def __init__(self):
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
            "http://www.naprostore.ru/foto/RBC/naprostore_rbc.xml", 
            SPEC)
        self.set_header("Content-Type", "application/json; charset=utf-8")
        self.write(json.dumps(result, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    app = Application([tornado.web.url(r"/", MainHandler),])
    app.listen(8888)
    tornado.ioloop.IOLoop.current().start()

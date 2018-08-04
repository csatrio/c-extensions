import uvmodule
import threading

CRLF = '\r\n'

def generate_header(my_dict):
    val = ''
    for k, v in my_dict.items():
        val += f"{k}: {v}{CRLF}"
    return val

class Response (dict):

    def __init__(self):
        self.body = None
        self.status = 200
        self.keep_alive = False

    def setHeader(self, key, value):
        self[key] = value

    def setStatus(self, status):
        self.status = status

    def generate(self, arg):
        self['Content-Length'] = len(arg)
        return f"HTTP/1.1 {self.status} OK{CRLF}" \
               f"Connection: {'keep-alive' if self.keep_alive else 'close'}{CRLF}" \
               f"{generate_header(self)}{CRLF}{arg}"

class Request(dict):

    def __init__(self):
        self.method = 'GET'
        self.url = ''
        self.host = ''
        self.port = ''

    def generate(self):
        return f"{self.method} {self.url} HTTP/1.1{CRLF}" \
               f"Host: {self.host}{CRLF}"\
               f"Connection: keep-alive{CRLF}{CRLF}" \
               #f"Pragma: no-cache{CRLF}"\
               #f"Cache-Control: no-cache{CRLF}" \
               # f"Upgrade-Insecure-Requests: 1{CRLF}"\
               # f"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/67.0.3396.99 Safari/537.36{CRLF}"\
               # f"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8{CRLF}"\
               # f"Accept-Encoding: gzip, deflate, br{CRLF}"\
               # f"Accept-Language: en-US,en;q=0.9,id;q=0.8{CRLF}"\

# perform work on thread pool
def on_work(name):
    print(threading.current_thread())
    print(f"on async work {name}")


# marshall result back to main thread
def on_finish_work(name):
    print(threading.current_thread())
    print(f"on finish async work {name}")



submit = False
def server_cb(req, handle):
    _response = Response()
    response = _response.generate('Custom response method')
    uvmodule.send_response(_response.generate('ABC'), handle)
    print(threading.current_thread())

def client_cb(req, handle):
    print(req['body'])
    # for k,v in req.items():
    #     print(f"{k}=>{v}")

my_timer = None
r = Request()
r.method = 'GET'
r.url = ''
r.host = 'tokokiranajaya.com'
r.port = '80'
def timer_cb(param):
    print(threading.current_thread())
    print(f"timer callback to trigger task : {str(param)}")
    #uvmodule.submit_task(on_work, on_finish_work, ('hello-work',))
    uvmodule.send_request(r.generate(), r.host, r.port, client_cb)
    #uvmodule.stop_timer(my_timer)

print('starting server from python...')
uvmodule.set_buffer_size(1024*1024)
uvmodule.set_callback(server_cb)
uvmodule.set_async(False)
my_timer = uvmodule.start_timer(timer_cb, ('Http Client',), 0, 1000)
uvmodule.listen_request('0.0.0.0', 8000)
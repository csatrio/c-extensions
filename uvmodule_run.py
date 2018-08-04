import uvmodule

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


res_str = ''
with open('/home/csatrio/Desktop/tes.txt', 'r', encoding='utf-8') as myfile:
    for line in myfile:
        res_str += line

#print(type(res_str))

_response = Response()
def server_cb(req, handle):
    _response.clear()
    _response['Content-Type'] = 'text/html'
    response = _response.generate(res_str)
    # for k,v in req.items():
    #     print(f"{k}=>{v}")
    uvmodule.send_response(response, handle)

print("starting server from python...")
uvmodule.set_callback(server_cb)
uvmodule.set_async(True)
uvmodule.set_buffer_size(1024*1024)
uvmodule.listen_request("0.0.0.0", 8000)
import hello

hello.hello_world()
hello.hello("ABC")


def my_cb(arg):
    print("Callback called from c")
    print(f"{str(arg)}")

hello.set_callback(my_cb)
hello.call_cb()

def set_str(my_str):
    my_str += "kucing"

c = ""
set_str(c)
print(c)

a = {}
a[1] = "b"
hello.set_dict(a)
print(a["response"])
print("finish")




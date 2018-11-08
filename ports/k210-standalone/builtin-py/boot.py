import gc
import platform
import uos
import os
import machine
import common
import app
import socket
#pin_init=common.pin_init()
#pin_init.init()
#test_gpio_pin_num=15
#fpioa=machine.fpioa()
#fpioa.set_function(test_gpio_pin_num,63)
#test_pin=machine.pin(7,2,0)
#if test_pin.value() == 0:
#    print('test')
#    machine.test()
# run usr init.py file
file_list = os.ls()
for i in range(len(file_list)):
    if file_list[i] == '/init.py':
        import init
wifi=machine.esp8285()
wifi.init("Sipeed_2.4G","Sipeed123.")
addr=socket.getaddrinfo("www.baidu.com",80)[0][-1]

/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "stdlib.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "esp8285.h"
#include "sleep.h"
#include "uart.h"

#include "plic.h"
#include "sysctl.h"
#include "utils.h"
#include "atomic.h"
#include "fpioa.h"

#include "py/obj.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "py/objtype.h"
#include "py/objstr.h"
#include "py/objint.h"

#include "modmachine.h"
#include "mphalport.h"
#include "plic.h"
#include "sysctl.h"
#include "py/objtype.h"


typedef struct _machine_esp8285_obj_t {
    mp_obj_base_t base;
    //mp_uint_t repeat;//timer mode
} machine_esp8285_obj_t;

const mp_obj_type_t machine_esp8285_type;

#define K210_DEBUG 0
#if K210_DEBUG==1
#define debug_print(x,arg...) printf("[lichee]"x,##arg)
#else 
#define debug_print(x,arg...) 
#endif

STATIC void machine_esp8285_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
	return mp_const_none;
}
STATIC mp_obj_t machine_esp8285_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    machine_esp8285_obj_t *self = m_new_obj(machine_esp8285_obj_t);
    self->base.type = &machine_esp8285_type;

    return self;
}

STATIC mp_obj_t machine_esp8285_init_helper(machine_esp8285_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
		ARG_ssid,
		ARG_passwd,
    };
    static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_ssid,		 MP_ARG_OBJ, {.u_obj = mp_const_none} },
		{ MP_QSTR_passwd,	 MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	fpioa_set_function(6,64);
    fpioa_set_function(7,65);
	esp8285_init();
	if(mp_const_none == args[ARG_ssid].u_obj || mp_const_none == args[ARG_passwd].u_obj)
	{
		mp_raise_ValueError("Please enter ssid and password\n");
	}
	mp_buffer_info_t buf_id;
	mp_buffer_info_t buf_pswd;
    mp_obj_str_get_buffer(args[ARG_ssid].u_obj, &buf_id, MP_BUFFER_READ);
	mp_obj_str_get_buffer(args[ARG_passwd].u_obj, &buf_pswd, MP_BUFFER_READ);
	unsigned char *id_ptr =buf_id.buf;
	unsigned char *pswd_ptr =buf_pswd.buf;
	printf("id is %s\n",id_ptr);
	printf("passwd is %s\n",pswd_ptr);
	/*
	if(buf_id.len == 0 || buf_pswd.len == 0)
	{
		mp_raise_ValueError("parameter is empty\n");
	}
	*/

	esp8285_wifista_config(id_ptr,pswd_ptr);
	/*
	int res = 1;
	res = esp8285_send_cmd("AT+CIPDOMAIN=\"www.baidu.com\"","OK",0);
	unsigned char* ret_ptr = rev_buf_addr();
	printf("%s\n",ret_ptr);
	*/
    return mp_const_none;
}



STATIC mp_obj_t machine_esp8285_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_esp8285_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_esp8285_init_obj, 1, machine_esp8285_init);


STATIC mp_obj_t machine_esp8285_send_cmd(machine_esp8285_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
		ARG_cmd,
		ARG_ack,
    };
    static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_cmd,		 MP_ARG_OBJ, {.u_obj = mp_const_none} },
		{ MP_QSTR_ack,	 MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	if(mp_const_none == args[ARG_cmd].u_obj || mp_const_none == args[ARG_ack].u_obj)
	{
		mp_raise_ValueError("Please enter cmd and check_ack\n");
	}
	mp_buffer_info_t buf_cmd;
	mp_buffer_info_t buf_ack;
    mp_obj_str_get_buffer(args[ARG_cmd].u_obj, &buf_cmd, MP_BUFFER_READ);
	mp_obj_str_get_buffer(args[ARG_ack].u_obj, &buf_ack, MP_BUFFER_READ);
	unsigned char *cmd_ptr =buf_cmd.buf;
	unsigned char *buf_ptr =buf_ack.buf;
	printf("cmd is %s\n",cmd_ptr);
	printf("ack is %s\n",buf_ptr);
	if(buf_cmd.len == 0 || buf_ack.len == 0)
	{
		mp_raise_ValueError("parameter is empty\n");
	}
	esp8285_send_cmd(cmd_ptr,buf_ptr,0);
    return mp_const_none;
}


STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_esp8285_send_cmd_obj, 1, machine_esp8285_send_cmd);



/*
STATIC mp_obj_t mod_usocket_getaddrinfo(mp_obj_t host_in, mp_obj_t port_in) {
    size_t hlen;
    const char *host = mp_obj_str_get_data(host_in, &hlen);
    mp_int_t port = mp_obj_get_int(port_in);
    uint8_t out_ip[MOD_NETWORK_IPADDR_BUF_SIZE];
    bool have_ip = false;

    if (hlen > 0) {
		esp8285_send_cmd("AT+CIPDOMAIN=<domain	name>");
        netutils_parse_ipv4_addr(host_in, out_ip, NETUTILS_BIG);
        have_ip = true;
    }

    if (!have_ip) {
        // find a NIC that can do a name lookup
        for (mp_uint_t i = 0; i < MP_STATE_PORT(mod_network_nic_list).len; i++) {
            mp_obj_t nic = MP_STATE_PORT(mod_network_nic_list).items[i];
            mod_network_nic_type_t *nic_type = (mod_network_nic_type_t*)mp_obj_get_type(nic);
            if (nic_type->gethostbyname != NULL) {
                int ret = nic_type->gethostbyname(nic, host, hlen, out_ip);
                if (ret != 0) {
                    mp_raise_OSError(ret);
                }
                have_ip = true;
                break;
            }
        }
    }

    if (!have_ip) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "no available NIC"));
    }

    mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(mp_obj_new_tuple(5, NULL));
    tuple->items[0] = MP_OBJ_NEW_SMALL_INT(MOD_NETWORK_AF_INET);
    tuple->items[1] = MP_OBJ_NEW_SMALL_INT(MOD_NETWORK_SOCK_STREAM);
    tuple->items[2] = MP_OBJ_NEW_SMALL_INT(0);
    tuple->items[3] = MP_OBJ_NEW_QSTR(MP_QSTR_);
    tuple->items[4] = netutils_format_inet_addr(out_ip, port, NETUTILS_BIG);
    return mp_obj_new_list(1, (mp_obj_t*)&tuple);
}
*/
STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_esp8285_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&machine_esp8285_send_cmd_obj) },
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(false) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(true) },
};
STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

const mp_obj_type_t machine_esp8285_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = machine_esp8285_print,
    .make_new = machine_esp8285_make_new,
    .locals_dict = (mp_obj_t)&machine_timer_locals_dict,
};

/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//#include <stdint.h>
#include <stdio.h>
//#include <string.h>

#include "sleep.h"
#include "encoding.h"

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"
#include "fpioa.h"
#include "gpio.h"
#include "lib/mp-readline/readline.h"
#include "timer.h"
#include "sysctl.h"
#include "w25qxx.h"
#include "plic.h"
#include "uarths.h"
#include "lcd.h"
#include "spiffs-port.h"
#include <malloc.h>
#define UART_BUF_LENGTH_MAX 269
#define MPY_HEAP_SIZE 1 * 1024 * 1024
extern int mp_hal_stdin_rx_chr(void);


static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[MPY_HEAP_SIZE];
#endif

void do_str(const char *src, mp_parse_input_kind_t input_kind);
/*
void test_board_driver(void)
{
    uint8_t ip_str[100];
    uint8_t send_buf[200];
    uint8_t rev_buf[200];
    fpioa_set_function(6,64);
    fpioa_set_function(7,65);
    //fpioa_set_function(8,56);
    //gpio_set_pin(0, GPIO_PV_HIGH);
    //msleep(1);
    //gpio_set_pin(0, GPIO_PV_LOW);
    //msleep(1);
    esp8285_init();
    esp8285_wifista_config("Sipeed_2.4G","Sipeed123.");
    //esp8285_get_wanip(ip_str);
    //printf("\nlocal ip:%s",ip_str);
    esp8285_send_cmd("AT+PING=\"www.baidu.com\"","OK",1);

    sprintf(send_buf,"AT+CIPSTART=\"TCP\",\"%s\",%s","api.seniverse.com","80");//116.62.81.138
	esp8285_send_cmd(send_buf,"OK",1);
    sprintf(send_buf,"GET http://api.seniverse.com/v3/weather/now.json?key=x3owc7bndhbvi8oq&location=shenzhen&language=zh-Hans&unit=c\n\n");
    esp8285_send_cmd("AT+CIPMODE=0","OK",1);
    uint8_t temp_code_buf[200];
    sprintf(temp_code_buf,"AT+CIPSEND=%d",strlen(send_buf));
    esp8285_send_cmd(temp_code_buf,"OK",1);
    
    esp8285_send_data(send_buf,strlen(send_buf));

    //wait res
    
    esp8285_check_cmd("OK");
    esp8285_check_cmd("+IPD,");
    printf("data earch\n");
    //esp8285_quit_trans();
    
    uint32_t temp_len = 0;
    temp_len = 300;
    temp_len = esp8285_rev_buflen(":");
    uint16_t len;
    uint8_t *temp_rev_buf_p;
    temp_rev_buf_p = rev_buf;
    uint32_t timeout = 10000;
    uint32_t temp_timeout = timeout;
    while(temp_len > 0 && temp_timeout > 0)
    {
        len = esp8285_rev_data(temp_rev_buf_p,temp_len,10000);
        usleep(100);
        temp_len -= len;temp_rev_buf_p += len;
        temp_timeout--;
        if(len > 0)
            temp_timeout = timeout;
        if(strstr(rev_buf,"}]}"))
            break;
        if(len >0)
            printf("len %d rev_data %x %x %x %s\n",len,rev_buf[0],rev_buf[1],rev_buf[2],rev_buf);
    }

    printf("res len %d\n",temp_len);
    printf("rev_data %s\n",rev_buf);
    //esp8285_quit_trans();
}
*/
int main()
{
    uint64_t core_id = current_coreid();
    plic_init();
	set_csr(mie, MIP_MEIP);
	set_csr(mstatus, MSTATUS_MIE);
    if (core_id == 0)
    {
        sysctl_pll_set_freq(SYSCTL_PLL0,320000000);
		sysctl_pll_enable(SYSCTL_PLL1);
		sysctl_pll_set_freq(SYSCTL_PLL1,160000000);
		uarths_init();
		printf("[lichee]:pll0 freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));
		printf("[lichee]:pll1 freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL1));
		sysctl->power_sel.power_mode_sel6 = 1;
		sysctl->power_sel.power_mode_sel7 = 1;
		uarths_set_irq(UARTHS_RECEIVE,on_irq_uarths_recv,NULL,1);
		uarths_config(115200,UARTHS_STOP_1);
		uarths_init();
        uint8_t manuf_id, device_id;
		while (1) {
			w25qxx_init(3);
			w25qxx_read_id(&manuf_id, &device_id);
			if (manuf_id != 0xFF && manuf_id != 0x00 && device_id != 0xFF && device_id != 0x00)
			    break;
		}
		w25qxx_enable_quad_mode();
        printf("manuf_id:0x%02x,device_id:0x%02x\n", manuf_id, device_id);
		my_spiffs_init();
	    int stack_dummy;
	    stack_top = (char*)&stack_dummy;
	    #if MICROPY_ENABLE_GC
	    gc_init(heap, heap + sizeof(heap));
	    #endif
	    mp_init();
	    readline_init0();
	    readline_process_char(27);
	    pyexec_frozen_module("boot.py");
	    #if MICROPY_REPL_EVENT_DRIVEN
            pyexec_event_repl_init();
            char c = 0;
            for (;;) {
                int cnt = read_ringbuff(&c,1);
                if(cnt==0){continue;}
                if(pyexec_event_repl_process_char(c)) {
                    break;
                }
            }
	    #else
	        pyexec_friendly_repl();
	    #endif
	    mp_deinit();
	    msleep(1);
	    printf("prower off\n");
	    return 0;
    }
    while (1);
}
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}

void nlr_jump_fail(void *val) {
    while (1);
}

void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info();
}

#if !MICROPY_DEBUG_PRINTERS
// With MICROPY_DEBUG_PRINTERS disabled DEBUG_printf is not defined but it
// is still needed by esp-open-lwip for debugging output, so define it here.
#include <stdarg.h>
int mp_vprintf(const mp_print_t *print, const char *fmt, va_list args);
int DEBUG_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(MICROPY_DEBUG_PRINTER, fmt, ap);
    va_end(ap);
    return ret;
}
#endif



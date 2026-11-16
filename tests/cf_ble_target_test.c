/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#define BLE_SCAN_N 8
#define BLE_SCAN_RING 4
#define BLE_CS_IDLE 0
#define BLE_CF_NEED_PAIR 8
#define BLE_ERR_LOCAL_HOST 22
struct ble_found { uint8_t used,midi,addr_rand,addr[6]; char name[17]; uint32_t seen_ms; };
struct ble_scan_tab { struct ble_found e[8]; uint32_t reports; };
struct ble_peer { uint8_t addr[6],addr_rand; };
static uint8_t cb_started=1,cb_ready,cb_panic_pos,cb_panic,cb_lost;
static uint32_t fm1_ms,cb_tr,cb_tw;
static int enabled,scanning,connected,state,fail,held,attempts;
static struct ble_peer selected;
static void fm1_ble_irqs_hold(int x){held=x;}
static int ble_central_fail(void){return fail;}
static int ble_central_state(void){return state;}
static void ble_enable(int x){assert(held);enabled=x;}
static int ble_connected(void){return connected;}
static void ll_close(int x){(void)x;assert(held);connected=state=0;}
static void ble_ll_scan(int x){assert(held);scanning=x;}
static void ble_scan_clear(struct ble_scan_tab *t){memset(t,0,sizeof *t);}
static void ble_scan_age(struct ble_scan_tab *t,uint32_t n){(void)t;(void)n;}
static uint8_t ble_ll_scan_take(uint8_t *p,uint16_t *r){(void)p;(void)r;return 0;}
static void ble_scan_add(struct ble_scan_tab*t,uint8_t*p,uint8_t n,uint16_t r,uint32_t m){(void)t;(void)p;(void)n;(void)r;(void)m;}
static void ble_central_cancel(void){assert(held);state=0;}
static void ble_cpy(void*d,const void*s,uint32_t n){memcpy(d,s,n);}
static void ble_zero(uint8_t*p,uint32_t n){memset(p,0,n);}
static int ble_central_connect(const struct ble_peer*p){assert(held);selected=*p;attempts++;state=1;return 1;}
#include "../firmware/src/cf_ble_target.c"
int main(void){
 cf_ble_target_poll(0);assert(enabled&&!scanning&&cb_target==0);
 cb_ready=connected=1;cb_tw=5;
 cf_ble_target_poll(1);assert(cb_switching&&cb_panic&&connected);
 fm1_ms=249;cf_ble_target_poll(1);assert(connected);
 fm1_ms=250;cf_ble_target_poll(1);assert(!connected&&enabled&&scanning&&cb_lost&&cb_tr==5);
 struct ble_found*f=&cb_scan.e[0];f->used=f->midi=1;strcpy(f->name,"WIDI Bud Pro");f->seen_ms=2000;
 fm1_ms=2000;cf_ble_target_poll(1);assert(!attempts);
 strcpy(f->name,"OP-1");f->addr[0]=42;f->addr_rand=1;
 cf_ble_target_poll(1);assert(attempts==1&&selected.addr[0]==42&&selected.addr_rand);
 fm1_ms=15001;cf_ble_target_poll(1);assert(!state);
 cf_ble_target_poll(2);assert(!enabled&&!scanning&&cb_target==2);
 cf_ble_target_poll(0);assert(enabled&&!scanning);
 cb_ready=1;cf_ble_target_poll(1);assert(cb_switching);
 cf_ble_target_poll(0);assert(!cb_switching);
 puts("BLE target: WIDI/OP-1/off, panic grace, name filter, address type, timeout and cancelled switch passed");
}

#include "ethernet.h"
#include "utils.h"
#include "driver.h"
#include "arp.h"
#include "ip.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 处理一个收到的数据包
 *        你需要判断以太网数据帧的协议类型，注意大小端转换
 *        如果是ARP协议数据包，则去掉以太网包头，发送到arp层处理arp_in()
 *        如果是IP协议数据包，则去掉以太网包头，发送到IP层处理ip_in()
 * 
 * @param buf 要处理的数据包
 */
void ethernet_in(buf_t *buf)
{
    // TODO
    uint16_t protocol;
    protocol = (buf->data[12])<<8;
    protocol += buf->data[13];
    buf_remove_header(buf, 14);
    if (protocol == NET_PROTOCOL_ARP)
    {
        arp_in(buf);
    }
    else if(protocol == NET_PROTOCOL_IP)
    {
        ip_in(buf);
    }
}

/**
 * @brief 处理一个要发送的数据包
 *        你需添加以太网包头，填写目的MAC地址、源MAC地址、协议类型
 *        添加完成后将以太网数据帧发送到驱动层
 * 
 * @param buf 要处理的数据包
 * @param mac 目标ip地址
 * @param protocol 上层协议
 */
void ethernet_out(buf_t *buf, const uint8_t *mac, net_protocol_t protocol)
{
    // TODO
    // DRIVER_IF_MAC = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    buf_add_header(buf, 14);
    uint8_t *p = buf->data;
    for (int i = 0; i < 6; i++)
    {
        p[i] = mac[i];
    }
    p[6] = 0x11;
    p[7] = 0x22;
    p[8] = 0x33;
    p[9] = 0x44;
    p[10] = 0x55;
    p[11] = 0x66;
    p[12] = (protocol >> 8);
    p[13] = (protocol & 0x00ff);
    driver_send(buf);
}

/**
 * @brief 初始化以太网协议
 * 
 * @return int 成功为0，失败为-1
 */
int ethernet_init()
{
    buf_init(&rxbuf, ETHERNET_MTU + sizeof(ether_hdr_t));
    return driver_open();
}

/**
 * @brief 一次以太网轮询
 * 
 */
void ethernet_poll()
{
    if (driver_recv(&rxbuf) > 0)
        ethernet_in(&rxbuf);
}

#include "udp.h"
#include "ip.h"
#include "icmp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief udp处理程序表
 * 
 */
static udp_entry_t udp_table[UDP_MAX_HANDLER];

/**
 * @brief udp伪校验和计算
 *        1. 你首先调用buf_add_header()添加UDP伪头部
 *        2. 将IP头部拷贝出来，暂存被UDP伪头部覆盖的IP头部
 *        3. 填写UDP伪头部的12字节字段
 *        4. 计算UDP校验和，注意：UDP校验和覆盖了UDP头部、UDP数据和UDP伪头部
 *        5. 再将暂存的IP头部拷贝回来
 *        6. 调用buf_remove_header()函数去掉UDP伪头部
 *        7. 返回计算后的校验和。  
 * 
 * @param buf 要计算的包
 * @param src_ip 源ip地址
 * @param dest_ip 目的ip地址
 * @return uint16_t 伪校验和
 */
static uint16_t udp_checksum(buf_t *buf, uint8_t *src_ip, uint8_t *dest_ip)
{
    // TODO
    int length = (buf->data[4])*256 + buf->data[5];
    buf_add_header(buf, 12);
    uint8_t *p = buf->data;
    uint16_t checksum;
    uint8_t tempiphead[12];
    for (int i = 0; i < 12; i++){ tempiphead[i] = p[i]; };    
    for (int i = 0; i < 4; i++){ p[i] = src_ip[i]; }
    for (int i = 0; i < 4; i++){ p[i+4] = dest_ip[i]; }
    p[8] = 0;
    p[9] = 17;
    p[10] = p[16];
    p[11] = p[17];
    checksum = checksum16(buf->data, length+12);
    for (int i = 0; i < 12; i++){ p[i] = tempiphead[i]; }
    buf_remove_header(buf,12);
    return checksum;
}

/**
 * @brief 处理一个收到的udp数据包
 *        你首先需要检查UDP报头长度
 *        接着计算checksum，步骤如下：
 *          （1）先将UDP首部的checksum缓存起来
 *          （2）再将UDP首都的checksum字段清零
 *          （3）调用udp_checksum()计算UDP校验和
 *          （4）比较计算后的校验和与之前缓存的checksum进行比较，如不相等，则不处理该数据报。
 *       然后，根据该数据报目的端口号查找udp_table，查看是否有对应的处理函数（回调函数）
 *       
 *       如果没有找到，则调用buf_add_header()函数增加IP数据报头部(想一想，此处为什么要增加IP头部？？)
 *       然后调用icmp_unreachable()函数发送一个端口不可达的ICMP差错报文。
 * 
 *       如果能找到，则去掉UDP报头，调用处理函数（回调函数）来做相应处理。
 * 
 * @param buf 要处理的包
 * @param src_ip 源ip地址
 */
void udp_in(buf_t *buf, uint8_t *src_ip)
{
    // TODO
    int i;
    uint16_t checksum1, checksum2;
    udp_hdr_t *uh = (udp_hdr_t *)buf->data;
    if (buf->len >= 8)
    {
        checksum1 = swap16(uh->checksum);
        uh->checksum = 0;
        checksum2 = udp_checksum(buf, src_ip, net_if_ip);
        uh->checksum = swap16(checksum1);
        if (checksum1 == checksum2)
        {
            for (i = 0; i < UDP_MAX_HANDLER; i++)
            {
                if (udp_table[i].port == swap16(uh->dest_port) && udp_table[i].valid == 1)
                {
                    buf_remove_header(buf, 8);
                    udp_table[i].handler(udp_table, src_ip, swap16(uh->src_port), buf);
                    break;
                }                
            }
            if (i == UDP_MAX_HANDLER)
            {
                buf_add_header(buf, 20);
                icmp_unreachable(buf, src_ip, ICMP_CODE_PORT_UNREACH);
            }
        }
    }
}

/**
 * @brief 处理一个要发送的数据包
 *        你首先需要调用buf_add_header()函数增加UDP头部长度空间
 *        填充UDP首部字段
 *        调用udp_checksum()函数计算UDP校验和
 *        将封装的UDP数据报发送到IP层。    
 * 
 * @param buf 要处理的包
 * @param src_port 源端口号
 * @param dest_ip 目的ip地址
 * @param dest_port 目的端口号
 */
void udp_out(buf_t *buf, uint16_t src_port, uint8_t *dest_ip, uint16_t dest_port)
{
    // TODO
    uint16_t checksum;
    buf_add_header(buf, 8);
    udp_hdr_t *uh = (udp_hdr_t *)buf->data;
    uh->checksum = 0;
    uh->src_port = swap16(src_port);
    uh->dest_port = swap16(dest_port);
    uh->total_len = swap16(buf->len);
    checksum = udp_checksum(buf, net_if_ip, dest_ip);
    uh->checksum = swap16(checksum);
    ip_out(buf, dest_ip, NET_PROTOCOL_UDP);
}

/**
 * @brief 初始化udp协议
 * 
 */
void udp_init()
{
    for (int i = 0; i < UDP_MAX_HANDLER; i++)
        udp_table[i].valid = 0;
}

/**
 * @brief 打开一个udp端口并注册处理程序
 * 
 * @param port 端口号
 * @param handler 处理程序
 * @return int 成功为0，失败为-1
 */
int udp_open(uint16_t port, udp_handler_t handler)
{
    for (int i = 0; i < UDP_MAX_HANDLER; i++) //试图更新
        if (udp_table[i].port == port)
        {
            udp_table[i].handler = handler;
            udp_table[i].valid = 1;
            return 0;
        }

    for (int i = 0; i < UDP_MAX_HANDLER; i++) //试图插入
        if (udp_table[i].valid == 0)
        {
            udp_table[i].handler = handler;
            udp_table[i].port = port;
            udp_table[i].valid = 1;
            return 0;
        }
    return -1;
}

/**
 * @brief 关闭一个udp端口
 * 
 * @param port 端口号
 */
void udp_close(uint16_t port)
{
    for (int i = 0; i < UDP_MAX_HANDLER; i++)
        if (udp_table[i].port == port)
            udp_table[i].valid = 0;
}

/**
 * @brief 发送一个udp包
 * 
 * @param data 要发送的数据
 * @param len 数据长度
 * @param src_port 源端口号
 * @param dest_ip 目的ip地址
 * @param dest_port 目的端口号
 */
void udp_send(uint8_t *data, uint16_t len, uint16_t src_port, uint8_t *dest_ip, uint16_t dest_port)
{
    buf_init(&txbuf, len);
    memcpy(txbuf.data, data, len);
    udp_out(&txbuf, src_port, dest_ip, dest_port);
}

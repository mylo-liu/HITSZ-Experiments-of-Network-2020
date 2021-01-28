#include "ip.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"
#include <string.h>
#include <stdio.h>

int my_id = 0;
int cmp_ip1(uint8_t *ip1, uint8_t *ip2);
/**
 * @brief 处理一个收到的数据包
 *        你首先需要做报头检查，检查项包括：版本号、总长度、首部长度等。
 * 
 *        接着，计算头部校验和，注意：需要先把头部校验和字段缓存起来，再将校验和字段清零，
 *        调用checksum16()函数计算头部检验和，比较计算的结果与之前缓存的校验和是否一致，
 *        如果不一致，则不处理该数据报。
 * 
 *        检查收到的数据包的目的IP地址是否为本机的IP地址，只处理目的IP为本机的数据报。
 * 
 *        检查IP报头的协议字段：
 *        如果是ICMP协议，则去掉IP头部，发送给ICMP协议层处理
 *        如果是UDP协议，则去掉IP头部，发送给UDP协议层处理
 *        如果是本实验中不支持的其他协议，则需要调用icmp_unreachable()函数回送一个ICMP协议不可达的报文。
 *          
 * @param buf 要处理的包
 */
void ip_in(buf_t *buf)
{
    // TODO 
    uint8_t p10;
    uint8_t p11;
    ip_hdr_t ih;
    uint8_t *p = buf->data;
    uint8_t net_if_ip_broadcast[4];
    for (int i = 0; i < 4; i++)
    {
        net_if_ip_broadcast[i] = net_if_ip[i];
    }
    net_if_ip_broadcast[3] = 255;
    
    ih.hdr_len = (p[0]) & 0x0f;
    ih.version = (p[0]) >> 4;
    ih.tos = p[1];
    ih.total_len = p[2]*256 + p[3];
    ih.id = p[4]*256 + p[5];
    ih.flags_fragment = p[6]*256 + p[7];
    ih.ttl = p[8];
    ih.protocol = p[9];
    ih.hdr_checksum = p[10]*256 + p[11];
    uint16_t checksum = 0;
    for (int i = 0; i < 4; i++){ih.src_ip[i] = p[12+i];}
    for (int i = 0; i < 4; i++){ih.dest_ip[i] = p[16+i];}
    if ((ih.version == IP_VERSION_4) && (ih.hdr_len >= 5) && (ih.total_len >= 20))
    {
        p10 = p[10]; p11 = p[11];
        p[10] = 0;   p[11] = 0;
        checksum = checksum16(buf->data, 4*(ih.hdr_len));
        p[10] = p10; p[11] = p11;
        if (checksum == ih.hdr_checksum && (cmp_ip1(ih.dest_ip, net_if_ip) || cmp_ip1(ih.dest_ip, net_if_ip_broadcast)))
        {            
            if (ih.protocol == 1)
            {                
                buf_remove_header(buf, 4*(ih.hdr_len));
                icmp_in(buf, ih.src_ip);
            }
            else if (ih.protocol == 17)
            {
                buf_remove_header(buf, 4*(ih.hdr_len));
                udp_in(buf, ih.src_ip);
            }
            else
            {
                icmp_unreachable(buf, ih.src_ip, ICMP_CODE_PROTOCOL_UNREACH);
            }            
        }
    }
}

/**
 * @brief 处理一个要发送的ip分片
 *        你需要调用buf_add_header增加IP数据报头部缓存空间。
 *        填写IP数据报头部字段。
 *        将checksum字段填0，再调用checksum16()函数计算校验和，并将计算后的结果填写到checksum字段中。
 *        将封装后的IP数据报发送到arp层。
 * 
 * @param buf 要发送的分片
 * @param ip 目标ip地址
 * @param protocol 上层协议
 * @param id 数据包id
 * @param offset 分片offset，必须被8整除
 * @param mf 分片mf标志，是否有下一个分片
 */
void ip_fragment_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol, int id, uint16_t offset, int mf)
{
    // TODO
    buf_add_header(buf, 20);
    uint8_t *p = buf->data;
    p[0] = 0b01000101;
    p[1] = 0;
    p[2] = (buf->len) >> 8;
    p[3] = (buf->len) & 0x00ff;
    p[4] = id >> 8;
    p[5] = id & 0x00ff;
    p[6] = offset >> 8;
    if(mf == 1) { p[6] = p[6] | 0b00100000; }
    p[7] = offset & 0x00ff;
    p[8] = 64;
    p[9] = protocol;
    p[10] = 0;
    p[11] = 0;
    for (int i = 0; i < 4; i++){p[12+i] = net_if_ip[i];}
    for (int i = 0; i < 4; i++){p[16+i] = ip[i];}
    uint16_t checksum = 0;
    checksum = checksum16(p, 20);
    p[10] = checksum >> 8;
    p[11] = checksum & 0x00ff;
    arp_out(buf, ip, NET_PROTOCOL_IP);
}

/**
 * @brief 处理一个要发送的ip数据包
 *        你首先需要检查需要发送的IP数据报是否大于以太网帧的最大包长（1500字节 - 以太网报头长度）。
 *        
 *        如果超过，则需要分片发送。 
 *        分片步骤：
 *        （1）调用buf_init()函数初始化buf，长度为以太网帧的最大包长（1500字节 - 以太网报头长度）
 *        （2）将数据报截断，每个截断后的包长度 = 以太网帧的最大包长，调用ip_fragment_out()函数发送出去
 *        （3）如果截断后最后的一个分片小于或等于以太网帧的最大包长，
 *             调用buf_init()函数初始化buf，长度为该分片大小，再调用ip_fragment_out()函数发送出去
 *             注意：id为IP数据报的分片标识，从0开始编号，每增加一个分片，自加1。最后一个分片的MF = 0
 *    
 *        如果没有超过以太网帧的最大包长，则直接调用调用ip_fragment_out()函数发送出去。
 * 
 * @param buf 要处理的包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{
    // TODO 
    int n = 0;
    int l = 0;
    buf_t buf0;
    if (buf->len < 1500-20)
    {
        ip_fragment_out(buf, ip, protocol, my_id, 0, 0);
        my_id++;
    }
    else
    {
        n = buf->len / (1500-20);
        l = buf->len % (1500-20);
        for (int i = 0; i < n; i++)
        {
            buf_init(&buf0, 1500-20);
            for(int j = 0; j < 1500-20; j++) { buf0.data[j] = buf->data[i*(1500-20)+j]; }
            ip_fragment_out(&buf0, ip, protocol, my_id, i*185, 1);
        }
        if (l != 0)
        {
            buf_init(&buf0, l);
            for(int j = 0; j < l; j++) { buf0.data[j] = buf->data[n*(1500-20)+j]; }
            ip_fragment_out(&buf0, ip, protocol, my_id, n*185, 0);
        }
    }    
}

int cmp_ip1(uint8_t *ip1, uint8_t *ip2)
{
    int i = 0;
    for (i = 0; i < NET_IP_LEN; i++)
    {
        if (ip1[i] != ip2[i])
        {
            break;
        }
    }
    if (i == NET_IP_LEN)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    
}
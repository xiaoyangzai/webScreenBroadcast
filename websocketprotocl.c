#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <openssl/sha.h>  
#include <openssl/buffer.h>  
#include <openssl/bio.h>  
#include <openssl/evp.h>
#include "robust_io.h"
#include "websocketprotocl.h"

void send_websocket_response(int clientfd,char *websocket_key)
{
	char response[MAXLINE];
	printf("websocket Connection requestion.\n");
	printf("Sec-WebSocket-Key: %s\n",websocket_key);
	base64_sha_encode(websocket_key, response);  
	//发送websocket握手协议响应报
	writen(clientfd,response,strlen(response));
	return;
}
//获取数据长度
int get_websocket_data_len(uint8_t *data,uint32_t *size)
{
	if(data == NULL)
		return -1;
	uint8_t frist = (data[1]) & 0x7F;
	if(frist < 126)
		*size = frist;
	else if(frist == 126)
		*size = ntohs(*((uint16_t *)(data + 1)));	
	else if(frist == 127)
		*size = ntohll(*((uint16_t *)(data + 1)));
#ifdef __PROTOCL_DEBUG__
	printf("data size: %d\n",*size);
#endif
	return 0;
}

//获取掩码值，并且存储到数组中
int get_websocket_maskKey(uint8_t *data,uint8_t maskKey[])
{
	int i = 0;
	uint8_t *mask = data + 2;
	for(i = 0;i < 4;i++)
		maskKey[i] = mask[i];
	
#ifdef __PROTOCL_DEBUG__
	printf("Mask Key: ");
	for(i = 0;i < 4;i++)
		printf(" %2x ",maskKey[i]);
	puts("\n");
#endif
	return 0;
}

//解掩码或掩码
//转换后的结果存储在data中
int decode_websocke_data(uint8_t *data,uint32_t len,uint8_t maskKey[],uint8_t *outBuff)
{
	uint32_t i = 0,
		j = 0;
	data = data + 6;

	for(i = 0;i < len;i++)
	{
		j = i % 4;
		outBuff[i] = data[i] ^ maskKey[j];
	}
#ifdef __PROTOCL_DEBUG__
	printf("transate %d bytes data finish!!\n",i);
#endif
	return len;
}

int get_websocket_data(uint8_t *data,uint8_t *outBuff)
{
	//if mask??
	if(!IS_SETMASK(data[1]))
		return -1;
	if(OPCODE(data[0]) == CLOSE_CODE)
		return -1;
	uint32_t size = 0;
	get_websocket_data_len(data,&size);
	uint8_t maskKey[4] = {0};
	get_websocket_maskKey(data,maskKey);
	return decode_websocke_data(data,size,maskKey,outBuff);
}

int send_websocket_header(int clientfd,uint8_t type,uint64_t len)
{
	printf("send websocket header......\n");
	uint8_t pockHeader[MAXLINE] = {0};	
	int index = 0;
	pockHeader[index++] = 0x80 | type;
	if(len < 126)
		pockHeader[index++] = (uint8_t)len;
	else if(len <= 0xFFFF)
	{
		pockHeader[index++] = 126;
		*((uint16_t *)(pockHeader + index)) = htons((uint16_t)len);	
		index += 2;
	}
	else
	{
		pockHeader[index++] = 127;
		*((uint64_t *)(pockHeader + index)) = htonll(len);	
		index += 8;
	}
	if(writen(clientfd,pockHeader,index) < 0)
		sys_err("write failed");
	printf("send websocket header finish!!!\n");
	return 0;
	
}
//server send data pointed by data to client 
int send_websocket_data(int clientfd,uint8_t *data,uint64_t len,int flag)
{
	uint8_t pocket[MAXLINE] = {0};
	int index = 0;
	if(flag)
		pocket[index++] = 0x80 | CLOSE_CODE;	//FIN : 1,OPCODE : 0x8,CLOSE STATUES
	else
		pocket[index++] = 0x80 | TEXT_CODE;	//FIN : 1,OPCODE : 0x8,CLOSE STATUES


	if(len < 126)
		pocket[index++] = (uint8_t)len;
	else if(len <= 0xFFFF)
	{
		pocket[index++] = 126;
		pocket[index++] = htons((uint16_t)len);
	}
	else
	{
		pocket[index++] = 127;
		pocket[index++] = htonll(len);
	}
	memcpy(pocket + index,data,len);
	index = index + len;
	if(writen(clientfd,pocket,index) < 0)
		sys_err("send pocket failed");
#ifdef __PROTOCL_DEBUG__
	printf("====== %d Bytes has sent to client =======\n",index);
	int i = 0;
	for(i = 0;i < index;i++)
		printf("%hhx ",pocket[i]);
	puts("\n");
#endif
	return index;
}

//64位长整型网络字节序到本地字节序的转换
//若本地为大端存储模式，无序转换
//若本地为小端存储模式,则需要从大端到小端的转换
uint64_t ntohll(uint64_t nl)
{
	uint64_t ret = nl;
	union{
		unsigned int i;
		unsigned char low;
	}a;
	a.i = 0x01020304;
	if(a.low == 0x04)
	{
		//本地存储模式为小端存储模式
		uint32_t low = nl & 0xFFFFFFFF;
		low = ntohl(low);
		uint32_t high = (nl>>32) & 0xFFFFFFFF;
		high = ntohl(high);
		ret = low;
		ret <<=32;
		ret |= high;
	}
	return ret;
}

//64位长整型本地字节序到网络字节序的转换
//若本地为大端存储模式，无序转换
//若本地为小端存储模式,则需要从大端到小端的转换
uint64_t htonll(uint64_t hl)
{
	uint64_t ret = hl;
	union{
		unsigned int i;
		unsigned char low;
	}a;
	a.i = 0x01020304;
	if(a.low == 0x04)
	{
		//本地存储模式为小端存储模式
		uint32_t low = hl & 0xFFFFFFFF;
		low = htonl(low);
		uint32_t high = (hl>>32) & 0xFFFFFFFF;
		high = htonl(high);
		ret = low;
		ret <<=32;
		ret |= high;
	}
	return ret;
}

//base64加密算法
//@bindata: 需要加密的数据
//@base64: 加密后的数据存储缓冲区
//@binlength: 需要加密数据的长度
//返回值：返回加密后的数据存储缓冲区
char * base64_encode( const unsigned char * bindata, char * base64, int binlength )
{
    int i, j;
    unsigned char current;
	const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (unsigned char)0x3F;
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';
    return base64;
}

int base64_decode( const char * base64, unsigned char * bindata )
{
    int i, j;
	const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char k;
    unsigned char temp[4];
    for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
    {
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+3] )
                temp[3]= k;
        }

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
                ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
                ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
                ((unsigned char)(temp[3]&0x3F));
    }
    return j;
}

int base64_sha_encode(const char* key, char* response)  
{
	uint8_t val[256] = {0};  
	memset(val, 0, 256);  
	strcat((char *)val,(char *)key);
	strcat((char *)val,"258EAFA5-E914-47DA-95CA-C5AB0DC85B11");  
	printf("val = %s\n",val);
	uint8_t mt[SHA_DIGEST_LENGTH] = {0};  
	char accept[256] = {0};  
	SHA1(val, strlen((char *)val), mt);  
	memset(accept, 0, 256);  
	base64_encode(mt,accept,20);  
	memset(response, 0, 1024);  
	sprintf(response, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nSec-webSocket-Version: 13\r\nServer: Bottle-websocket-server\r\n\r\n",accept);  
	return 0;  
}

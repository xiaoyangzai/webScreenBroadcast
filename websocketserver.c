#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <ulimit.h> 

#include "robust_io.h"
#include "capture.h"
#include "jpeg_util.h"
#include "websocketprotocl.h"
#include "websocketserver.h"

void do_websocket_response(int clientfd, char *key)
{
	//发送websocket握手响应报文,建立websocket连接
	send_websocket_response(clientfd,key);
	
	//初始化资源
	global_resource res;
	res.clientfd = clientfd;
	res.disconnect_flag = 0;
	pthread_mutex_init(&res.mutex,NULL);

	//创建接收客户端数据线程
	pthread_t pid_recv,pid_send;
	pthread_create(&pid_recv,NULL,pthread_recv,(void *)&res);
	pthread_create(&pid_send,NULL,pthread_send,(void *)&res);

	pthread_join(pid_recv,NULL);
	pthread_join(pid_send,NULL);
	pthread_mutex_destroy(&res.mutex);
	return;
}
void *pthread_send(void *arg)
{
	global_resource *pres = (global_resource *)arg;
	uint8_t *outbuf = (uint8_t *)malloc(5 * 1024*1024);
	uint64_t outlen = 0;
		
	uint32_t width,height;
	get_window_size(&width,&height);
	uint8_t *rgb24 = (uint8_t *)malloc(3 * width * height);
	while(1)
	{
		pthread_mutex_lock(&pres->mutex);
		if(pres->disconnect_flag)
		{
			pthread_mutex_unlock(&pres->mutex);
			break;
		}
		pthread_mutex_unlock(&pres->mutex);
		//获取屏幕图像并发送到客户端
		
		//获取屏幕图像数据RGB24
		CaptureDesktop(rgb24);
		//对图像数据进行JPEG编码
		encode_jpeg(rgb24,width,height,outbuf,&outlen);	

		//发送websocket数据帧帧头,无需掩码处理
		send_websocket_header(pres->clientfd,BINARY_CODE,outlen);

		//发送图像的JPEG格式的数据到客户端
		writen(pres->clientfd,outbuf,outlen);
		//100ms发送一次
		usleep(100*1000);
	}
	free(rgb24);
	free(outbuf);
	pthread_exit(NULL);
}
void *pthread_recv(void *arg)
{
	//该线程函数仅处理客户端的断开连接请求
	global_resource *pres = (global_resource *)arg;
	char buf[256];
	int n = 0;
	while(1)
	{
		n = readn(pres->clientfd,buf,1);
		if(n == 0)
		{
			printf("client offline\n");
			pthread_mutex_lock(&pres->mutex);	
			pres->disconnect_flag = 1;
			pthread_mutex_unlock(&pres->mutex);	
			break;
		}
		if(n < 0)
		{
			perror("read failed");
			exit(-1);
		}
		if((buf[0] & 0xf0) == 0x80)
		{
			printf("client disconnect\n");
			pthread_mutex_lock(&pres->mutex);	
			pres->disconnect_flag = 1;
			pthread_mutex_unlock(&pres->mutex);	
			break;
		}
		//获取客户端有效数据
		//解析websocket协议数据帧,获取客户端发送的有效数据
		//your code
	}
	pthread_exit(NULL);
}

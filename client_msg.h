
/************************************************************
 * FileName: client_msg.h
 * Description: ͨ�Ű���ʽ
 * History:
 * <author>     <time>      <version >   <desc>
 * milowang		2010-06-18	1.1			add new cmd(3001-3004)
 * bakerchen    2010-03-01  1.0			build this moudle  
 ************************************************************/
#ifndef _CLIENT_MSG_H_
#define _CLIENT_MSG_H_

#include "common.h"

/*
 * �԰汾3.2.4��ʼ�������°���������֣���Ҫ֧��ָ��category����
 * �°�����ṹ�����£�
 *		int total_len;
 *		int cmd_word;
 *		int sequence_no;
 *		int result_num;
 *		int category:8;
 *		int unused1:24;
 *		int unused2;
 *		char *buf;
 * ����ԭ����ṹ��û��Ԥ����չ�ֶΣ�������ͬʱ��Ҫ������ǰ�Ľṹ
 * ��ˣ�Լ������:
 *	��cmd_wordΪ�ɰ�������ʱ��buf�����岻��
 *	��cmd_wordΪ�°�������ʱ��buf��ǰ8���ֽ�Ϊ(category, unused1, unused2)
 */
//���������
struct SClientReq
{
	int total_len;		//�ܳ���
	int cmd_word;		//������
	int sequence_no;	//������
	int result_num;		//�����
	char * buf;			//����
};

//Ӧ�������
struct SClientResp
{
	int total_len;		//�ܳ���
	int ret_code;		//������
	int sequence_no;	//������
	int result_num;		//�����
	char * buf;		//�����
};

enum eSearchCmd
{
	E_MIN_SEARCH_CMD		= 1000,		//��С����������
	E_ONLY_WORD_CMD			= 1001,		//ֻ������ʾ��
	E_TIMES_WORD_CMD		= 1002,		//���ؼ�����������ʾ��
	E_COUNT_WORD_CMD		= 1003,		//���ؽ��������ʾ��
	E_ALL_DATA_CMD			= 1004,		//����������Ϣ
	E_MAX_SEARCH_CMD		= 1005,		//������������
	
	E_MIN_CONTROL_CMD		= 2000,		//��С����������
	E_ADD_NORMAL_WORD_CMD	= 2001,		//���һ����ͨ����
	E_ADD_MASK_WORD_CMD		= 2002,		//���һ�����δ�
	E_DEL_MASK_WORD_CMD		= 2003,		//ɾ��һ�����δ�
	E_REQ_MASK_LIST_CMD		= 2004,		//�������δ��б�
	E_MAX_CONTROL_CMD		= 2005,		//������������

	E_MIN_SEARCH2_CMD		= 3000,		// �°���������Сֵ 
	E_SEARCH_ONLY_WORD		= 3001,		// �°�������ֻ������ʾ��
	E_SEARCH_TIMES_WORD		= 3002,		// �°����������ؼ�����������ʾ��
	E_SEARCH_COUNT_WORD		= 3003,		// �°����������ؽ��������ʾ��
	E_SEARCH_ALL_DATA		= 3004,		// �°�����������������Ϣ
	E_MAX_SEARCH2_CMD		= 3005		// �°����������ֵ

};

enum eErrorMsg
{
	E_RECV_ERROR = -1, 		//���մ���
	E_MSG_ILLEGAL = -2,		//ͨ�Ű��Ƿ�
	E_SEARCH_ERROR = -3		//��������

};

int recv_data(int sock_fd, char * buf, SClientReq & req, struct sockaddr_in & addr)
{
	int recv_len;
	char *p;
	int tmp;
	socklen_t addr_len;

	addr_len = sizeof(addr);
	recv_len = recvfrom(sock_fd, buf, MAX_BUF_LEN, 0, (struct sockaddr*) &addr, &addr_len);
	if(recv_len <= 0 || recv_len >= MAX_BUF_LEN-1)
	{
		return E_RECV_ERROR;
	}
	buf[recv_len] = '\0';

	p = buf;
	//�ܳ���
	tmp = * (int*) p;
	p += 4;
	req.total_len = ntohl(tmp);
	if(req.total_len != recv_len)
	{
		return E_MSG_ILLEGAL;
	}

	//������
	tmp = * (int*) p;
	p += 4;
	req.cmd_word = ntohl(tmp);
	if( ! ((req.cmd_word > E_MIN_SEARCH_CMD && req.cmd_word < E_MAX_SEARCH_CMD)
				|| (req.cmd_word > E_MIN_CONTROL_CMD && req.cmd_word < E_MAX_CONTROL_CMD)
				|| (req.cmd_word > E_MIN_SEARCH2_CMD && req.cmd_word < E_MAX_SEARCH2_CMD)) )
	{
		return E_MSG_ILLEGAL;
	}

	//������
	tmp = * (int*) p;
	p += 4;
	req.sequence_no = ntohl(tmp);

	//�����
	tmp = * (int*) p;
	p += 4;
	req.result_num = ntohl(tmp);
	if(req.result_num <= 0)
	{
		return E_MSG_ILLEGAL;
	}

	//����
	req.buf = p;

	return 0;
}

int send_data(int sock_fd, char *buf, const SClientReq &req, int ret_code, SResultData*result_data, int result_count, 
		const struct sockaddr_in & addr)
{
	int total_len;
	int tmp_len;
	char *p;

	total_len = 16;
	if(ret_code == 0)
	{
		p = buf + 16;
		for(int i=0; i < result_count; i++)
		{
			if(req.cmd_word == E_ONLY_WORD_CMD)
			{
				tmp_len = sprintf(p, "%s\n", result_data[i].szWord);
				p += tmp_len;
			}
			else if(req.cmd_word == E_TIMES_WORD_CMD)
			{
				tmp_len = sprintf(p, "%d\t%s\n", result_data[i].dwSearchTimes, result_data[i].szWord);
				p += tmp_len;
			}
			else if(req.cmd_word == E_COUNT_WORD_CMD)
			{
				tmp_len = sprintf(p, "%d\t%s\n", result_data[i].dwResultCount, result_data[i].szWord);
				p += tmp_len;
			}
			else
			{
				tmp_len = sprintf(p, "%d\t%d\t%s\n", result_data[i].dwSearchTimes, result_data[i].dwResultCount, 
						result_data[i].szWord);
				p += tmp_len;
			}
		}
		total_len = p - buf;
	}

	p = buf;
	*(int*) p = htonl(total_len);
	p += 4;
	*(int*) p = htonl(ret_code);
	p += 4;
	*(int*) p = htonl(req.sequence_no);
	p += 4;
	*(int*) p = htonl(result_count);

	sendto(sock_fd, buf, total_len, 0, (struct sockaddr*)&addr, sizeof(addr)); // ���Ͳ�ѯ���

	return 0;
}

int send_mask_data(int sock_fd, char *buf, const SClientReq &req, int ret_code, char*result_data, int result_count, 
		const struct sockaddr_in & addr)
{
	int total_len;
	char *p;

	total_len = 16;
	if(ret_code == 0)
	{
		p = buf + 16;
		strcpy(p, result_data);
		total_len = 16 + strlen(result_data);
	}

	p = buf;
	*(int*) p = htonl(total_len);
	p += 4;
	*(int*) p = htonl(ret_code);
	p += 4;
	*(int*) p = htonl(req.sequence_no);
	p += 4;
	*(int*) p = htonl(result_count);

	sendto(sock_fd, buf, total_len, 0, (struct sockaddr*)&addr, sizeof(addr)); // ���Ͳ�ѯ���

	return 0;
}

int client_recv_data(int sock_fd, char *buf, SClientResp &resp, char **result, 
		struct sockaddr_in & addr)
{
	int recv_len;
	char *p;
	int tmp;
	socklen_t addr_len = sizeof(addr);

	recv_len = recvfrom(sock_fd, buf, MAX_BUF_LEN, 0, (struct sockaddr*) &addr, &addr_len);
	if(recv_len <= 0)
	{
		return E_RECV_ERROR;
	}
	buf[recv_len] = '\0';

	p = buf;
	//�ܳ���
	tmp = * (int*) p;
	p += 4;
	resp.total_len = ntohl(tmp);
	if(resp.total_len != recv_len)
	{
		return E_MSG_ILLEGAL;
	}
	//������
	tmp = * (int*) p;
	p += 4;
	resp.ret_code = ntohl(tmp);
	if(resp.ret_code < 0)
	{
		return E_SEARCH_ERROR;
	}
	//������
	tmp = * (int*) p;
	p += 4;
	resp.sequence_no = ntohl(tmp);
	//�����
	tmp = * (int*) p;
	p += 4;
	resp.result_num = ntohl(tmp);
	if(resp.result_num <= 0)
	{
		return 0;
	}
	//�����
	 tmp = 1;
	 result[0] = p;
	 while(p < buf+recv_len)
	 {
		if(*p != '\n')
		{
			p++;
		}
		else
		{
			*p = '\0';
			if(p+1 >= buf+recv_len)
			{
				break;
			}
			p += 1;
			result[tmp++] = p;
		}
	 }

	return 0;
}

int client_send_data(int sock_fd, char * buf, int cmd_word, int seq_no, int result_num, char* query, 
		const struct sockaddr_in & addr)
{
	int total_len;
	char *p;

	p = buf;

	//�ܳ���
	total_len = 16 + strlen(query);
	* (int*) p = htonl(total_len);
	p += 4;
	//������
	* (int*) p = htonl(cmd_word);
	p += 4;
	//���к�
	* (int*) p = htonl(seq_no);
	p += 4;
	//�����
	* (int*) p = htonl(result_num);
	p += 4;
	//�ַ���
	strcpy(p, query);

    return sendto(sock_fd, buf, total_len, 0, (struct sockaddr*)&addr, sizeof(addr));
}

#endif


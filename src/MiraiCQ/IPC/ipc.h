#pragma once

/*
 * ����������Api
 * ����`to_pid`:��Ŀ����̺�
 * ����`msg`��Ҫ���͵�����
 * ����ֵ�����ص��ý��(C�����ַ����������Լ��ͷ�)������ʱ(15s)�򷵻�`""`
*/
const char* IPC_ApiSend(int to_pid, const char* msg);

/*
 * �������㲥�¼�
 * ����`msg`��Ҫ���͵�����
*/
void IPC_SendEvent(const char* msg);

/*
 * �����������¼�
 * ����`pid`:���¼��㲥�ŵĽ��̺�
 * ����ֵ�������¼����(C���Էǿ��ַ����������Լ��ͷ�)�������¼��򷵻�`""`
*/
const char* IPC_GetEvent(int pid);

/*
 * ����������api����
 * ����`fun`:����api����ʱ�Ļص�����������Ϊapi�����߷��͵�msg,����ֵΪҪ���ص��ַ�����ʹ��IPC_Malloc�������ڴ�
*/
void IPC_ApiRecv(char* ((*fun)(const char*)));


/*
 * �����������ڴ棬��`IPC_ApiRecv`���ʹ��
 * ����`sz`:��Ҫ������ڴ��С��ͬmalloc
*/
void* IPC_Malloc(size_t sz);
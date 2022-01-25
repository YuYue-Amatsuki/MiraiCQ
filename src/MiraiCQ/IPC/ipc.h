#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

/* ����const char * �����ᣬҲ����ΪNULL��������Ϊδ���� */

/* 
������������ǵ�һ�����õ�IPC������
mainprocess�� ��uuid��`""`, 
plusproceess�У�uuid��plus_uuid
�ɹ�����0��ʧ�ܷ��ط�0
*/
int IPC_Init(const char* uuid);


/* �����������̵�api��tmΪ��ʱ����λΪms */
const char* IPC_ApiSend(const char* remote_uuid, const char* msg,int tm);

/* mainproess�����¼� */
void IPC_SendEvent(const char* msg);

/* plusprocess�����¼����˺����������������¼�Ϊֹ */
const char* IPC_GetEvent(const char* flag);

/* ����api����,�˺���������������api����ΪֹΪֹ */
void IPC_ApiRecv(void((*fun)(const char* sender_uuid, const char* flag, const char* msg)));

/* ���ڽ���api���ú�ķ��� */
void IPC_ApiReply(const char* sender_uuid, const char* flag, const char* msg);

/* ����mainprocess��ȡmain_uuid��Ȼ�󴫸��ӽ���*/
const char* IPC_GetFlag();

#ifdef  __cplusplus
}
#endif


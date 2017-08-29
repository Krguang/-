#include "modbusToAndroid.h"
#include "usart.h"

unsigned char SlaveAdd = 1;
unsigned char alarmStatus[7] = { 1,2,3,4,5,6,7 };
unsigned char LocalStatusArray[50];

unsigned int GetCRC16(unsigned char *arr_buff, unsigned char len) {  //CRCУ�����
	unsigned int crc = 0xFFFF;
	unsigned char i, j;
	for (j = 0; j < len; j++) {
		crc = crc ^*arr_buff++;
		for (i = 0; i < 8; i++) {
			if ((crc & 0x0001) > 0) {
				crc = crc >> 1;
				crc = crc ^ 0xa001;
			}
			else
				crc = crc >> 1;
		}
	}
	return (crc);
}


void ModbusDecode(unsigned char *MDbuf, unsigned char len) {

	unsigned char i;
	unsigned char cnt;
	unsigned int  crc;
	unsigned char crch, crcl;

	if (MDbuf[0] != SlaveAdd) return;								//��ַ���ʱ���ٶԱ�֡���ݽ���У��
	crc = GetCRC16(MDbuf, len - 2);								//����CRCУ��ֵ
	crch = crc >> 8;
	crcl = crc & 0xFF;
	if ((MDbuf[len - 1] != crch) || (MDbuf[len - 2] != crcl)) return;	//��CRCУ�鲻��ʱֱ���˳�

	switch (MDbuf[1]) {											//��ַ��У���־�����󣬽��������룬ִ����ز���

	case 0x03:											//��ȡһ���������ļĴ���
		if ((MDbuf[2] == 0x00) && (MDbuf[3] <= 0x20)) {			//ֻ֧��0x0000��0x0020
			i = MDbuf[3];									//��ȡ�Ĵ�����ַ
			cnt = MDbuf[5];									//��ȡ����ȡ�ļĴ�������
			MDbuf[2] = cnt * 2;								//��ȡ���ݵ��ֽ�����Ϊ�Ĵ�����*2
			len = 3;										//֡ǰ�����е�ַ�������롢�ֽ�����3���ֽ�
			while (cnt--) {
				unsigned int LocalStatusArrayTemp = LocalStatusArray[i++];	//��ȡ����16λ���飬ת��Ϊ2��8λ���ݴ��뷢������
				MDbuf[len++] = LocalStatusArrayTemp >> 8;
				MDbuf[len++] = LocalStatusArrayTemp & 0xff;
			}
		}
		else {					//�Ĵ�����ַ����֧��ʱ�����ش�����
			MDbuf[1] = 0x83;	//���������λ��1
			MDbuf[2] = 0x02;	//�����쳣��Ϊ02-��Ч��ַ
			len = 3;
		}
		break;

	case 0x06:											//д�뵥���Ĵ���
		if ((MDbuf[2] == 0x00) && (MDbuf[3] <= 0x20)) {	//�Ĵ�����ַ֧��0x0000��0x0020
			i = MDbuf[3];								//��ȡ�Ĵ�����ַ
			LocalStatusArray[i] = MDbuf[5];				//����Ĵ�������
			len -= 2;									//����-2�����¼���CRC������ԭ֡
		}
		else {					//�Ĵ�����ַ����֧��ʱ�����ش�����{
			MDbuf[1] = 0x86;	//���������λ��1
			MDbuf[2] = 0x02;	//�����쳣��Ϊ02-��Ч��ַ
			len = 3;
		}
		break;

	case 0x10:
		if ((MDbuf[2] == 0x00) && (MDbuf[3] <= 0x20)) {		//�Ĵ�����ַ֧��0x0000��0x0020
			i = MDbuf[3];									//��ȡ�Ĵ�����ַ
			cnt = MDbuf[5];									//��ȡ��д��ļĴ�������
			unsigned char startNum = 7;						//���ö�ȡ��Ҫд������ݵĵ�ַ
			unsigned int writeTemp = 0;						//д����8λת16λ�Ļ���
			while (cnt--) {									//дcnt��
				writeTemp = MDbuf[startNum];					//��һλ����д�뻺��
				LocalStatusArray[i] = (writeTemp << 8) + MDbuf[startNum + 1];	//����Ĵ�������
				i++;
				startNum = startNum + 2;
			}

			len = 6;			//����6֡���¼���CRC������ԭ֡
		}
		else {					//�Ĵ�����ַ����֧��ʱ�����ش�����{
			MDbuf[1] = 0x86;	//���������λ��1
			MDbuf[2] = 0x02;	//�����쳣��Ϊ02-��Ч��ַ
			len = 3;
		}
		break;

	default:					//������֧�ֵĹ�����
		MDbuf[1] |= 0x80;		//���������λ��1
		MDbuf[2] = 0x01;		//�����쳣��Ϊ01-��Ч����
		len = 3;
		break;
	}
	crc = GetCRC16(MDbuf, len);		//���㷵��֡��CRCУ��ֵ
	MDbuf[len++] = crc & 0xFF;		//CRC���ֽ�
	MDbuf[len++] = crc >> 8;		//CRC���ֽ�
									//UsartTransmitArray(MDbuf, len);	//���ͷ���֡
	HAL_UART_Transmit(&huart1, MDbuf, len, 1000);
}

void UsartRxMonitor() {
	static unsigned char ArrayLenTemp;					//�洢���ջ���������һ�γ��ȣ������ж����鳤���Ƿ���ֱ仯
	static unsigned char BusIdleCount;					//���߿��м���
	
	if (Usart1ReceiveBuffer.BufferLen > 0) {
		if (ArrayLenTemp != Usart1ReceiveBuffer.BufferLen) {		//����˴����鳤�����ϴβ�ͬ
			ArrayLenTemp = Usart1ReceiveBuffer.BufferLen;		//�˴����鳤�ȸ�ֵ���ϴ�
			BusIdleCount = 0;								//������м�ʱ
		}
		else {											//����˴����鳤�����ϴ���ͬ
			if (BusIdleCount < 4) {
				BusIdleCount++;							//���߿��м�ʱ+1
				if (BusIdleCount >= 4) {					//������м�ʱ>=4
					BusIdleCount = 0;						//���м�ʱ����
					ModbusDecode(Usart1ReceiveBuffer.BufferArray, Usart1ReceiveBuffer.BufferLen);	//����
					Usart1ReceiveBuffer.BufferLen = 0;											//����󣬻������鳤������
				}
			}
		}
	}
	else ArrayLenTemp = 0;
}

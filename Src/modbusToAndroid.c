#include "modbusToAndroid.h"
#include "usart.h"

uint8_t SlaveAdd = 1;

uint8_t txBuf[50];
uint8_t txCount = 0;

uint16_t localData[50];


uint16_t GetCRC16(uint8_t *arr_buff, uint8_t len) {  //CRCУ�����
	uint16_t crc = 0xFFFF;
	uint8_t i, j;
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

void sendDataMaster03() {
	uint16_t temp;
	txBuf[0] = SlaveAdd;
	txBuf[1] = 0x03;
	txBuf[2] = 0x00;
	txBuf[3] = 0x00;
	txBuf[4] = 0x00;
	txBuf[5] = 0x06;//��6λ
	temp = GetCRC16(txBuf, 6);
	txBuf[6] = (uint8_t)(temp & 0xff);
	txBuf[7] = (uint8_t)(temp >> 8);
	txCount = 8;
	HAL_UART_Transmit(&huart1, txBuf, txCount, 0xff);
}

void sendDataMaster16() {

	uint16_t temp;
	uint8_t i;

	txBuf[0] = SlaveAdd;
	txBuf[1] = 0x10;
	txBuf[2] = 0x00;         //���ݵ���ʼ��ַ��
	txBuf[3] = 0x06;
	txBuf[4] = 0x00;         //���ݵĸ�����
	txBuf[5] = 0x07;
	txBuf[6] = 0x0e;         //���ݵ��ֽ�����
	for (i = 0; i<txBuf[5]; i++) {
		txBuf[7 + 2 * i] = (uint8_t)(localData[i+ txBuf[3]] >> 8);
		txBuf[8 + 2 * i] = (uint8_t)(localData[i+ txBuf[3]] & 0xff);
	}
	temp = GetCRC16(txBuf, 2 * txBuf[5] + 7);
	txBuf[7 + 2 * txBuf[5]] = (uint8_t)(temp & 0xff);
	txBuf[8 + 2 * txBuf[5]] = (uint8_t)((temp >> 8) & 0xff);
	txCount = 9 + 2 * txBuf[5];
	HAL_UART_Transmit(&huart1, txBuf, txCount, 0xff);
}

void ModbusDecode(uint8_t *MDbuf, uint8_t len) {

	uint16_t  crc;
	uint8_t crch, crcl;
	uint16_t temp;

	if (MDbuf[0] != SlaveAdd) return;								//��ַ���ʱ���ٶԱ�֡���ݽ���У��
	crc = GetCRC16(MDbuf, len - 2);								//����CRCУ��ֵ
	crch = crc >> 8;
	crcl = crc & 0xFF;
	if ((MDbuf[len - 1] != crch) || (MDbuf[len - 2] != crcl)) return;	//��CRCУ�鲻��ʱֱ���˳�
	if (MDbuf[1] != 0x03) return;									//���鹦����
	if (MDbuf[2] > 0x20) return;					//�Ĵ�����ַ֧��0x0000��0x0020
	for (uint8_t i = 0; i < MDbuf[2]/2; i++)
	{
		localData[i] = (uint16_t)(MDbuf[3 + 2*i] << 8) + MDbuf[4 + 2*i];
	}
}

void UsartRxMonitor() {
	static uint8_t ArrayLenTemp;					//�洢���ջ���������һ�γ��ȣ������ж����鳤���Ƿ���ֱ仯
	static uint8_t BusIdleCount;					//���߿��м���
	
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
					HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_1);
					ModbusDecode(Usart1ReceiveBuffer.BufferArray, Usart1ReceiveBuffer.BufferLen);	//����
					Usart1ReceiveBuffer.BufferLen = 0;											//����󣬻������鳤������
				}
			}
		}
	}
	else ArrayLenTemp = 0;
}

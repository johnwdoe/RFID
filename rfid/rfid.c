/*
 * rfid.c
 *
 *  Created on: May 13, 2013
 *      Author: william
 */

#include "rfid.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define CARD_DATA (PB0)
#define ANT_SHORT (PB1)
//#define ANT_SHORT_2 (PB2)
#define CARD_CLK (PB3)

#define B_EDGEDETECTED (0)
#define B_EDGEDIRECTION (1)
#define B_SYNCHRONISED (2)

#define R_EQ (0)
#define R_GT (1)
#define R_LE (2)

//bit operations
#define F_(X) (1<<X)
#define NF_(X) (~(1<<X))
#define SBIT_(f, X) (f|=(1<<X))
#define CBIT_(f, X) (f&=(~(1<<X)))
#define TBIT_(f, X) (f^=(1<<X))
#define GBIT_(f, X) ((f&(1<<X))>>X)


//don't modify this defines!!!
#define _OCR2 (F_CPU/RFID_MAIN_FREQ/2 - 1)

uint8_t unpackedData[5]; //только "полезные" данные
uint8_t packedData[8]; //данные, готовые к отсылке (со всеми чексамами и прочей херней)
uint8_t tPos[1];
volatile uint8_t flag = 0x00;

volatile uint16_t pT;
volatile uint16_t cT;
volatile uint16_t synT;
volatile uint8_t currentPos; //позиция курсора в битовом шаблоне
volatile uint8_t preparedBit; //предрасчитанное значение бита

uint8_t tCompare(uint16_t t1, uint16_t t2)
{
	/**compare values of t1 and t2.
	 * returns:	R_EQ - t1 = t2
	 * 			R_GT - t1 = 2t2 (t1>t2)
	 * 			R_LE - t2 = 2t1 (t1<t2)
	 * 			R_ERR - else
	*/
	//TODO переосмыслить коэффициенты!!!
	uint32_t tmp;
	tmp = (((uint32_t)(t1)) * 100) / (t2);
	if(tmp>30 && tmp<70)
	{
		return R_LE; //t1 < t2 (2t1 = t2)
	}else if(tmp>70 && tmp<130)
	{
		return R_EQ; //t1 = t2
	}else if(tmp>170 && tmp<230)
	{
		return R_GT; //t1 > t2 (t1 = 2t2)
	}else return RFID_RET_ERR; //what da fuck ?!1
}

uint8_t sync (void)
{
	//TODO предусмотреть принудительный выход!
	do
	{
		while(!GBIT_(flag, B_EDGEDETECTED));
		CBIT_(flag, B_EDGEDETECTED);
	}while (tCompare(pT, cT) != R_LE); //до тех пор, пока не наступит cT = 2pT
	synT = pT; //вот и период нашелся!
	SBIT_(flag, B_SYNCHRONISED); //бит синхронизации
	return RFID_RET_OK;
}

uint8_t getCurrentBit(void)
{
	uint8_t tmp;
	while (!GBIT_(flag, B_EDGEDETECTED));
	CBIT_(flag, B_EDGEDETECTED);
	tmp = tCompare(cT, synT);
	if (tmp == R_EQ)
	{
		//wait for next edge
		while (!GBIT_(flag, B_EDGEDETECTED));
		CBIT_(flag, B_EDGEDETECTED);
		tmp = tCompare(cT,synT);
		if (tmp != R_EQ) return RFID_RET_ERR;
	}else if (tmp != R_GT)
	{
		return RFID_RET_ERR;
	}
	return GBIT_(flag, B_EDGEDIRECTION);
}

uint8_t waitForStartSeq(void)
{
	uint8_t tmp, i;
	i=9;
	while(i != 0)
	{
		tmp = getCurrentBit();
		//Проверка факта синхронизации. Иначе рискуем зависнуть к хуям!
		if (!GBIT_(flag, B_SYNCHRONISED)) return RFID_RET_ERR;
		if(tmp == 1)
		{
			i--;
		}else if(tmp == 0)
		{
			i = 9; //reset
		}else return RFID_RET_ERR;
	}
	return RFID_RET_OK;
}

void setBits(uint8_t *dst, uint8_t *sPos, uint8_t val, uint8_t len) //tested OK
{

	uint16_t mask;  //маска для наложения, так сказать
	uint8_t cellAddr = (*sPos)>>3; //номер байта в массиве
	uint8_t bitN = (*sPos) & 0x07; //позиция бита (слева)
	uint16_t tmp = *(dst+cellAddr);
	tmp <<= 8; //shift it
	if (bitN + len > 8) tmp |= *(dst+cellAddr+1); //дергаем соседа

	//fix value (optional)
	val &= (0xFF >> (8 - len));

	//calculate 16-bit mask
	mask = (0xFF >> (8 - len));
	mask <<= (16 - bitN - len);
	mask = ~mask;

	tmp = ((tmp) & mask) | (val << (16 - bitN - len));
	*(dst+cellAddr) = (tmp>>8);
	if (bitN + len > 8) *(dst+cellAddr+1) = tmp; //снова к соседу :)
	(*sPos) += len; //сдвигаем позицию
}

uint8_t getBits(uint8_t *src, uint8_t *sPos, uint8_t len) //tested OK
{
	uint8_t res;

	uint8_t cellAddr = (*sPos)>>3; //номер байта в массиве
	uint8_t bitN = (*sPos) & 0x07; //позиция бита (слева)
	res = *(src + cellAddr) & (0xFF >> bitN);
	if(bitN + len <= 8) //конец данных не выходит за пределы ячейки
	{
		res >>= (8 - bitN - len);
	}else //остатки стоит забрать в след. ячейке
	{
		res <<= (bitN + len - 8); //двигаем результат, дабы освободить место для остального добра
		res |= *(src + cellAddr + 1) >> (16 - bitN - len); //8 - (bitN + len - 8) //акуеть
	}
	*sPos += len; //увеличиваем позицию
	return res;
}

uint8_t xorEx(uint8_t par) //tested OK
{
	uint8_t res = 0;
	while (par != 0x00)
	{
		res ^= par;
		par >>= 1;
	}
	return (res & 0x01);
}

void pack(uint8_t *src, uint8_t *dst) //tested OK
{
	uint8_t i;
	uint8_t checkSumV = 0;
	uint8_t cPos[1] = {0}; //current pos to add bits
	uint8_t tmp;
	//суть в том, чтобы из 5 байт ID карты сделать 64-битный пакет для "отправки" ридеру:
	//9 стартовых единичных бит + 54 бита данных с контрольными суммами + 1 нулевой стоп-бит.
	//в манчестерский код будем уже переводить на лету в прерывании!
	setBits(dst, cPos, 0xFF, 8);
	setBits(dst, cPos, 0x01, 1); //9 бит стартовая посылка
	for (i=0; i<5; i++)
	{
		//проходим по всем 5 байтам

		tmp = *(src+i) >> 4; //берем 4 старших
		setBits(dst, cPos, tmp, 4);//пишем как есть
		setBits(dst, cPos, xorEx(tmp), 1); //чексам строки
		checkSumV ^= tmp; //обновляем верт. контр. сумму, мать ее!

		tmp = *(src+i) & 0x0F; //4 младших
		setBits(dst, cPos, tmp, 4);//пишем как есть
		setBits(dst, cPos, xorEx(tmp), 1); //чексам строки
		checkSumV ^= tmp;
	}
	//последняя строка (5 бит) - наш вертикальный чексам с нулем на конце
	checkSumV <<= 1;
	setBits(dst, cPos, checkSumV, 5);
}

uint8_t unpack(uint8_t *src, uint8_t *dst) //tested OK
{
	uint8_t i;
	uint8_t cPosSrc[1] = {0}; //current source position
	uint8_t cPosDst[1] = {0}; //current dest position
	uint8_t checkSumV = 0;
	uint8_t str;
	//Из 64-битного пакета получаем 5-байтовый ID карты.
	//если все ок, то возвращаем RET_OK
	//в противном случае (не сошлись контр. суммы, как правило) вернем RET_ERR
	//(*pos) = 0; //initial position
	//PORTC = getBits2(src, pos, 8);
	if (getBits(src, cPosSrc, 8) != 0xFF) return RFID_RET_ERR;
	if (getBits(src, cPosSrc, 1) != 0x01) return RFID_RET_ERR; //check start seq
	for (i=0; i<10; i++)
	{
		str = getBits(src, cPosSrc, 4); //берем очередные 4 бита
		checkSumV ^= str; //обновляем верт. чексам
		if (getBits(src, cPosSrc, 1) != xorEx(str)) return RFID_RET_ERR; //отваливаемся, если строка невалидна
		//строка прошла, так что можно добавлять ее в результат
		setBits(dst, cPosDst, str, 4);
	}

	str = getBits(src, cPosSrc, 5); //выдергиваем оставщиея 5 бит (верт. чексам и стоповый 0)
	if (str != (checkSumV<<1)) return RFID_RET_ERR; //сумма не совпала либо стоп ненулевой

	return RFID_RET_OK;
}

void release(void)
{
	/*отключает таймеры, разгружает необходимые пины.
	 * пригодится перед выходами из основных (внешних) процедур*/
	TCCR1B = 0x00; //stop T1
	TCCR2 = 0x00; //stop T2
	TIMSK &= ~(1<<TICIE1 | 1<<OCIE1A); //запрещаем прерывания

	//DDRB &= ~(1<<ANT_SHORT_1 | 1<<ANT_SHORT_2);
	PORTB &= ~(1<<CARD_CLK | 1<<ANT_SHORT); //оставили антенну в покое
	//PORTB &= ~(1<<CARD_CLK); //оставили антенну в покое

}

void rfid_ioinit(void)
{
	/*initialize io ports direction
	* PB0(ICP) - input. data from detector
	* PB1(OC1A) - output. emulate card (ANT. short). set to 0 while receive
	* PB3(OC2) - output. card clock. set to 0 while transmit
	*/
	DDRB |= (1<<ANT_SHORT | 1<<CARD_CLK);
	//DDRB |= (1<<CARD_CLK);
	//ant short будем менять с выхода на вход при постоянном значении PORTB

}

uint8_t rfid_read(uint8_t *data, volatile uint8_t *ctl, uint8_t abortmask) //tested OK
{
	cli();
	// настроить таймер2, чтоб дрыгал ногой CARD_CLK с частотой 250кГц
	OCR2 = _OCR2; //выставляем период (для несущей)
	TCCR2 |= (1<<FOC2 | 1<<WGM21 | 1<<COM20 | 1<<CS20); //запускаем таймер

	//конфигурация и запуск основного таймера
	TCCR1B |= (1<<ICES1 | 1<<CS10);
	TIMSK |= (1<<TICIE1); //прерывание по захвату
	sei();

	while((*ctl & abortmask) != abortmask) //пока не отменили вручную
	{
		packedData[0] = 0xFF;
		packedData[1] = 0x80; //имитируем стар. посылку заранее (понадобится "распаковщику")
		sync();
		if(waitForStartSeq() == RFID_RET_OK)
		{
			*tPos = 9; //начнем заполнять с 9-й позиции
			while(*tPos < 64) //по 63
			{
				setBits(packedData, tPos, getCurrentBit(), 1);
			}
			/*пытаемся распаковать то, что получили,
			 * но распаковываем, начиная со 2-й позиции,
			 * так как в 0 по 1 будет записан период синхронизации,
			 * если распакуется удачно :)*/
			if(unpack(packedData, data + 2) == RFID_RET_OK)
			{
				//пишем период
				*data = (uint8_t)(synT>>8);
				*(data+1) = (uint8_t) synT;
				release();
				return RFID_RET_OK;
			}
		}
	}
	release();
	//*ctl &= ~(abortmask); //reset abort mask
	return RFID_RET_ABORTED;
}

void rfid_transmit(uint8_t *data, volatile uint8_t *ctl, uint8_t abortmask)
{
	cli();
	TCCR2 = 0x00; //останавливаем таймер2, чтоб не дергал ногу CARD_CLK :)
	//prepare ports
	PORTB &= ~(1<<ANT_SHORT | 1<<CARD_CLK); //один "конец" антенны на земле, второй пока "висит в воздухе"
	//PORTB &= ~(1<<CARD_CLK);
	//DDRB |= (1<<ANT_SHORT_2); //as2 делаем выходом. теперь край антенны притянут к земле
	//ANT_SHORT на данный момент - вход без подтяжки (т.е. "висит в воздухе")
	//2 байта в младших адресах содержат период синхронизации (T/2). будем генерить прерывания
	//с таким периодом
	TCCR1A = 0x00;
	TCCR1B = (1<<WGM12); //таймер в режим CTC, заодно и останавливаем
	TCNT1 = 0x0000; //обнуляем счетчик
	OCR1AH = *(data++);
	OCR1AL = *(data++);//установка времени полупериода

	pack(data, packedData); //упаковали оставшиеся 5 байт в супер-мега-пакет
	currentPos = 1;
	preparedBit = 0;
	//порт ant_short выставлен в положение 0 (контур разомкнут и ридер не нагружен)
	TIMSK |= (1<<OCIE1A); //разрешаем прерывание по совпадению с OCR1A
	TCCR1B |= (1<<CS10); //поехали, епт!
	sei();

	while ((*ctl & abortmask) != abortmask); //тупим, пока нет команды останова
	release();
	//*ctl &= ~(abortmask); //reset abort mask
	//return RFID_RET_OK;
}

ISR(TIMER1_COMPA_vect)
{
	//T/2 happens!
	//TCNT1= 0x0000;
	if(currentPos++ & 0x01) //нечетный - просто инвертируем выход
	{
		PORTB ^= (1<<ANT_SHORT);
		//DDRB ^= (1<<ANT_SHORT_1);
		//currentPos++;
	}else
	{
		PORTB = (PORTB & ~(1<<ANT_SHORT)) | (preparedBit<<ANT_SHORT); //выставляем посчитанное ранее
		//DDRB = (DDRB & ~(1<<ANT_SHORT_1)) | (preparedBit<<ANT_SHORT_1); //выставляем посчитанное ранее
		//currentPos++;
		currentPos &= 0x7F; //127
		preparedBit = ((*(packedData + (currentPos>>4))) >> (7 - ((currentPos>>1)&0x07))) & 0x01; //очень хитро о_О
		//preparedBit ^= 0x01; //invert
	}
}

ISR(TIMER1_CAPT_vect)
{
	//cli();
	TCNT1 = 0x0000; //самое время скинуть таймер
	pT = cT; //сохраняем последний (предыдущий фронт) период
	cT = ICR1; //сохраняем новый период (текущий фронт (виновник прерывания, собсно))
	//а не отвалилась ли синхронизация на этом фронте?
	if(GBIT_(flag, B_SYNCHRONISED))
	{
		//текущий период не может быть меньше синхр. полупериода,
		//иначе скидываем флаг синхронизации к ебеням!
		if(tCompare(cT, synT) > R_GT)
		{
			CBIT_(flag, B_SYNCHRONISED);
		}
	}
	SBIT_(flag, B_EDGEDETECTED);
	flag = (flag & NF_(B_EDGEDIRECTION)) | (GBIT_(TCCR1B, ICES1) << B_EDGEDIRECTION);
	TBIT_(TCCR1B, ICES1); //invert edgr detector
}

/*
 * Описание регистров контроллера КСК.
 *
 * Автор: Дмитрий Подхватилин, НПП "Дозор" 2012
 */
 
#ifndef _KSK_MON_REG_H_
#define _KSK_MON_REG_H_

#include <dozor/ksk-reg.h>

#define KSKM_MTR            (*((vu32 *) KSK_MEM_BASE + 512))
#define KSKM_SYNC           (*((vu32 *) KSK_MEM_BASE + 513))
#define KSKM_SYNC_TIMES     (*((vu32 *) KSK_MEM_BASE + 514))
#define KSKM_MCR            (*((vu32 *) KSK_MEM_BASE + 515))

#define KSKM_RUN_WD(n)      (*((vu32 *) KSK_MEM_BASE + 528 + (n)))
#define KSKM_END_WD(n)      (*((vu32 *) KSK_MEM_BASE + 544 + (n)))

/*******
 * MTR *
 *******/
#define KSKM_CYCLE_LEN(n)       ((n) & 0x7FFFF)     /* Длина цикла в тактах шины */
#define KSKM_NOM_CYCLE_OS(n)    (((n) & 0x7) << 19) /* Количество циклов, которое ждёт основное синхронизирующее устройство в случае коллизии перед повторной выдачей слова типа 1 */
#define KSKM_NOM_CYCLE_RS(n)    (((n) & 0x7) << 22) /* Количество циклов, которое ждёт резервное синхронизирующее устройство в случае коллизии перед повторной выдачей слова типа 1 */
#define KSKM_PLACE(n)           (((n) & 0xF) << 25) /* Собственный номер */

/***************************************************************
 * Макросы для задания номеров устройств, выдающих синхрослова *
 ***************************************************************/
#define KSKM_SYNC_MAIN_CH1(n)   ((n) & 0xF)		/* Номер узла, выдающего основное синхрослово в первом канале */
#define KSKM_SYNC_RSRV_CH1(n)   (((n) & 0xF) << 4)	/* Номер узла, выдающего резерное синхрослово в первом канале */
#define KSKM_SYNC_MAIN_CH2(n)   (((n) & 0xF) << 8)	/* Номер узла, выдающего основное синхрослово во втором канале */
#define KSKM_SYNC_RSRV_CH2(n)   (((n) & 0xF) << 12)	/* Номер узла, выдающего резерное синхрослово во втором канале */
#define KSKM_SYNC_MAIN_CH3(n)   (((n) & 0xF) << 16)	/* Номер узла, выдающего основное синхрослово в третьем канале */
#define KSKM_SYNC_RSRV_CH3(n)   (((n) & 0xF) << 20)	/* Номер узла, выдающего резерное синхрослово в третьем канале */

/**********************************************
 * Макросы для задания окон выдачи синхрослов *
 **********************************************/
#define KSKM_SYNC_MAIN_BEGIN(t) ((t) & 0xFF)            /* Начало выдачи основного синхрослова */
#define KSKM_SYNC_RSRV_BEGIN(t) (((t) & 0xFF) << 8)     /* Начало выдачи резервного синхрослова */
#define KSKM_SYNC_MAIN_END(t)   (((t) & 0xFF) << 16)    /* Конец выдачи основного синхрослова */
#define KSKM_SYNC_RSRV_END(t)   (((t) & 0xFF) << 24)    /* Конец выдачи резервного синхрослова */

/*******
 * MCR *
 *******/
#define KSKM_MCR_ERROR_CHAN(n)  (1 << ((n) - 1))        /* Номер канала, в котором инжектируются ошибки */
#define KSKM_MCR_INJECT_ECS     (0 << 3)	            /* Тип инжектируемой ошибки: ошибка контрольной суммы */
#define KSKM_MCR_INJECT_EE      (1 << 3)	            /* Тип инжектируемой ошибки: ошибка эхо-контроля */
#define KSKM_MCR_INJECT_ELEN    (2 << 3)	            /* Тип инжектируемой ошибки: ошибка длины пакета */
#define KSKM_MCR_INJECT_ESN     (3 << 3)	            /* Тип инжектируемой ошибки: неправильный номер */
#define KSKM_MCR_ERR_PLACE(n)   (((n) & 0xF) << 5)      /* Номер узла, которому необходимо сымитировать ошибку эхо-контроля */
#define KSKM_MCR_BLOCK_TX(n)    ((1 << (n-1)) << 9)     /* Блокировка передатчиков, параметр - номер канала */
#define KSKM_MCR_BLOCK_RX(n)    ((1 << (n-1)) << 12)    /* Блокировка записи в ОЗУ-2Д, параметр - номер канала */

/******************************
 * Слово управления монитором *
 ******************************/
#define KSKM_CW1_CHAN1          (1 << 0)	/* Прослушивание канала 1 */
#define KSKM_CW1_CHAN2          (1 << 1)	/* Прослушивание канала 2 */
#define KSKM_CW1_CHAN3          (1 << 2)	/* Прослушивание канала 3 */
#define KSKM_CW1_NODE(n)        ((n) << 3)	/* Прослушивание узла с указанным номером; если 0, то прослушивание всех узлов */
#define KSKM_CW1_ADDR(n)        ((n) << 7)	/* Прослушивание слов типа 3 с указанным адресом; если 0, то прослушивание всех адресов */
#define KSKM_CW1_TYPE1          (1 << 18)	/* Включение регистрации слов типа 1 */
#define KSKM_CW1_TYPE2          (1 << 19)	/* Включение регистрации слов типа 2 */
#define KSKM_CW1_TYPE3          (1 << 20)	/* Включение регистрации слов типа 3 */

/* Ключ запуска в режиме монитора */
#define KSKM_START_MON          KSK_KEY(0x0A)

/*********************
 * Слово состояния 1 *
 *********************/
#define KSKM_SW1_GRUN           (1 << 28)

#define KSKM_EXT_RAM_BASE   0xC0000000
#define KSKM_EXT_RAM_SIZE   0x00080000
struct mon_word {
    volatile unsigned        data        :   32;
    volatile unsigned        addr        :   9;
    volatile unsigned        parity_err  :   1;
    volatile unsigned        channel     :   2;
    volatile unsigned        time        :   19;
    volatile unsigned        valid       :   1;
} __attribute__((packed));


#define KSKM_LAMP *((volatile unsigned *) 0xA0000000)
#define KSKM_ABONENT(n)         (1 << ((n)-1)*4)
#define KSKM_RESERVE(n)         (2 << ((n)-1)*4)
#define KSKM_MAIN(n)            (4 << ((n)-1)*4)
#define KSKM_MONITOR(n)         (8 << ((n)-1)*4)
#define KSKM_OK                 (1 << 12)


#endif /*_KSKM_MON_REG_H_*/


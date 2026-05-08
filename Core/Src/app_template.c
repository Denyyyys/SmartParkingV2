/*
 / _____)             _              | |
 ( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
 (______/|_____)_|_|_| \__)_____)\____)_| |_|
 (C)2013 Semtech

 Description: Ping-Pong implementation

 License: Revised BSD License, see LICENSE.TXT file include in the project

 Maintainer: Miguel Luis and Gregory Cristian
 */
#include <string.h>
#include "board.h"
#include "radio.h"
#include "app_template.h"
#include "lcd.h"
#include "stm32u5xx_hal.h"
#include "stdbool.h"
#include "tim.h"

#define RF_FREQUENCY                                868000000 // Hz
#define TX_OUTPUT_POWER                             0         // dBm

#if defined( USE_MODEM_LORA )

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#elif defined( USE_MODEM_FSK )

#define FSK_FDEV                                    25e3      // Hz
#define FSK_DATARATE                                50e3      // bps
#define FSK_BANDWIDTH                               50e3      // Hz
#define FSK_AFC_BANDWIDTH                           83.333e3  // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

#else
    #error "Please define a modem in the compiler options."
#endif

typedef enum {
	LOWPOWER, RX, RX_DONE, RX_TIMEOUT, RX_ERROR, TX, TX_TIMEOUT,
} States_t;

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 13

States_t State = LOWPOWER;

volatile int8_t RssiValue = 0;
volatile int8_t SnrValue = 0;

uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];

typedef struct {
	int rxdone;
	int rxtimeout;
	int rxerror;
	int txdone;
	int txtimeout;
} trx_events_cnt_t;

trx_events_cnt_t trx_events_cnt;

int rx_cnt = 0;
int txdone_cnt = 0;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(void);

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout(void);

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout(void);

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError(void);

volatile bool b1Pressed = false;
bool gateIsMoving = false;
bool gateIsClosed = true;

volatile uint32_t rise_time = 0;
volatile uint32_t fall_time = 0;
volatile uint32_t pulse_width = 0;
volatile uint8_t edge_state = 0; // 0 - waiting for rise, 1 - waiting for fall
volatile bool pa8_output = true;
volatile bool pa0PulseActive = false;
// TODO - change based on layout of objects
volatile float threshold = 10.0f;
volatile float average_distance = 10.0f;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) {
        if (edge_state == 0) { // should be rising edge
            rise_time = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            edge_state = 1;
        }
        else { // should be falling edge
            fall_time = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

            // handle overflow (if counter wrapped around)
            if (fall_time >= rise_time) {
                pulse_width = fall_time - rise_time;
            } else {
                pulse_width = (htim->Instance->ARR - rise_time) + fall_time;
            }

            edge_state = 0;
        }
    }
}

static void PA0_Pulse_StartUs(uint16_t pulseWidthUs)
{
	if (pulseWidthUs == 0 || pa0PulseActive) {
		return;
	}

	pa0PulseActive = true;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);

	__HAL_TIM_SET_COUNTER(&htim16, 0);
	__HAL_TIM_SET_AUTORELOAD(&htim16, (uint32_t)pulseWidthUs - 1U);
	__HAL_TIM_CLEAR_FLAG(&htim16, TIM_FLAG_UPDATE);
	HAL_TIM_Base_Start_IT(&htim16);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM16) {
		HAL_TIM_Base_Stop_IT(&htim16);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
		pa0PulseActive = false;
	}
}

/**
 * Main application entry point.
 */
void app_main(void) {
	// Target board initialisation
	BoardInitMcu();
	BoardInitPeriph();

	// Radio initialization
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;

	Radio.Init(&RadioEvents);

	Radio.SetChannel( RF_FREQUENCY);

#if defined( USE_MODEM_LORA )

    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
    
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

#elif defined( USE_MODEM_FSK )

	Radio.SetTxConfig(MODEM_FSK, /* Radio modem to be used [0: FSK, 1: LoRa] */
	TX_OUTPUT_POWER, /* Sets the output power [dBm] */
	FSK_FDEV, /* Sets the frequency deviation (FSK only) [Hz] */
	0, /* Sets the bandwidth (LoRa only); 0 for FSK */
	FSK_DATARATE, /* Sets the Datarate. FSK: 600..300000 bits/s */
	0, /* Sets the coding rate (LoRa only) FSK: N/A ( set to 0 ) */
	FSK_PREAMBLE_LENGTH, /* Sets the preamble length. FSK: Number of bytes */
	FSK_FIX_LENGTH_PAYLOAD_ON, /* Fixed length packets [0: variable, 1: fixed] */
	true, /* Enables disables the CRC [0: OFF, 1: ON] */
	0, /* Enables disables the intra-packet frequency hopping. FSK: N/A ( set to 0 ) */
	0, /* Number of symbols bewteen each hop. FSK: N/A ( set to 0 ) */
	0, /* Inverts IQ signals (LoRa only). FSK: N/A ( set to 0 ) */
	3000 /* Transmission timeout [ms] */
	);

	Radio.SetRxConfig(MODEM_FSK, /* Radio modem to be used [0: FSK, 1: LoRa] */
	FSK_BANDWIDTH, /* Sets the bandwidth. FSK: >= 2600 and <= 250000 Hz. (CAUTION: This is "single side bandwidth") */
	FSK_DATARATE, /* Sets the Datarate. FSK: 600..300000 bits/s */
	0, /* Sets the coding rate (LoRa only) FSK: N/A ( set to 0 ) */
	FSK_AFC_BANDWIDTH, /* Sets the AFC Bandwidth (FSK only). FSK: >= 2600 and <= 250000 Hz */
	FSK_PREAMBLE_LENGTH, /* Sets the Preamble length. FSK: Number of bytes */
	0, /* Sets the RxSingle timeout value (LoRa only). FSK: N/A ( set to 0 ) */
	FSK_FIX_LENGTH_PAYLOAD_ON, /* Fixed length packets [0: variable, 1: fixed] */
	0, /* Sets payload length when fixed lenght is used. */
	true, /* Enables/Disables the CRC [0: OFF, 1: ON] */
	0, /* Enables disables the intra-packet frequency hopping. FSK: N/A ( set to 0 ) */
	0, /* Number of symbols bewteen each hop. FSK: N/A ( set to 0 ) */
	false, /* Inverts IQ signals (LoRa only). FSK: N/A ( set to 0 ) */
	true /* Sets the reception in continuous mode. [false: single mode, true: continuous mode] */
	);

#else
    #error "Please define a frequency band in the compiler options."
#endif

	rx_loop();

	while (1) {
		printf("Infinite loop. This should never happen!\r\n");
	}
}

void openGate()
{
	gateIsMoving = true;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 140);
	gateIsMoving = false;
	gateIsClosed = false;
}

void closeGate()
{
	gateIsMoving = true;
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 80);
	gateIsMoving = false;
	gateIsClosed = true;
}

void delay_us()
{

}

void rx_loop(void) {
	char buf[50];
	int loop_cnt = 0;

	printf("\r\n\r\nRX loop start\r\n");
	int time_on_air;
	int payload_size = BUFFER_SIZE;
	time_on_air = Radio.TimeOnAir(MODEM_FSK, payload_size);
	printf("Time on air: %d us for payload_size: %d bytes\r\n", time_on_air, payload_size);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);

	lcd_init();
	closeGate();
	DelayMs(100);
	lcd_clear();

	Radio.Rx(0);

	while (1) {
		if (b1Pressed) {
			float distances[5];
			float sum = 0.f;
			for (int i = 0; i < 5; i++)
			{
				PA0_Pulse_StartUs(15);
				HAL_Delay(100);
				float distance = (float)pulse_width * 0.01715f;
				distances[i] = distance;
				sum += distance;
			}
			average_distance = sum / 5.0f;
				//			Radio.Sleep( );

			b1Pressed = false;
//			Radio.Sleep( );
			openGate();
			HAL_Delay(1000);
			closeGate();
			HAL_Delay(1000);

		}

		DelayMs(25);

		snprintf(buf, sizeof(buf), "%d %d %d %d %d ", RssiValue, trx_events_cnt.rxdone, trx_events_cnt.rxerror, trx_events_cnt.rxtimeout, loop_cnt);

		if (State == RX_TIMEOUT)
		{
			Radio.Rx(0);
			State = RX;
		}

		if (State == RX_DONE)
		{
			lcd_clear();

			printf("%s  \t", buf);
			RtcGetTimeStr((uint8_t*)buf);
			printf("Local time: %s, received: %s\r\n", buf, Buffer);
			if (Buffer[0] == 'N')
			{
				// response is good - open gate
				openGate();

				// wait for car to move
				HAL_Delay(5000);

				// check if szlaban can be closed
				do {
					float distances[5];
					float sum = 0.f;
					for (int i = 0; i < 5; i++)
					{
						// send trig signal to HC-SR04 to start measuring
						PA0_Pulse_StartUs(15);

						// wait for sound - it goes brrrrr
						HAL_Delay(100);

						// calculate distance
						float distance = (float)pulse_width * 0.01715f;
						distances[i] = distance;
						sum += distance;
					}

					// take average distance based on 5 measurments to increase reliability (i hope it does help)
					average_distance = sum / 5.0f;
				} while (average_distance <= threshold);

				// if here it means that average_distance > threshold - car is not there - we can close szlaban, but
				// just in case wait a bit more - 5s
				HAL_Delay(5000);

				closeGate();
				Radio.Sleep( );

				// need some time to set radio in sleep mode - maybe is possible with less value - have to check
				HAL_Delay(5000);

				uint8_t txbuf[50];
				const char *txt = "NICK-xxxxxxxx-Pozdrowienia";
				size_t len = strlen(txt);
				memcpy(txbuf, txt, len);
				Radio.Send(txbuf, (uint8_t)len);

			}

			lcd_set_cursor(0,0);
			lcd_write_string(Buffer);
			State = RX;
		}

		loop_cnt++;
	}

}

void OnTxDone(void) {
//	Radio.Sleep();
	State = TX;
	trx_events_cnt.txdone++;
	Radio.Rx(0);
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
	BufferSize = size;
	memcpy(Buffer, payload, BufferSize);
	RssiValue = rssi;
	SnrValue = snr;
//    State = RX;
	State = RX_DONE;
	trx_events_cnt.rxdone++;
//    Radio.Rx(0);
}

void OnTxTimeout(void) {
	Radio.Sleep();
	State = TX_TIMEOUT;
	trx_events_cnt.txtimeout++;
	Radio.Rx(0);
}

void OnRxTimeout(void) {
	State = RX_TIMEOUT;
	trx_events_cnt.rxtimeout++;
}

void OnRxError(void) {
	State = RX_ERROR;
	trx_events_cnt.rxerror++;
	Radio.Rx(0);
}

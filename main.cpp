#include <stdio.h>
#include "STM32F7xx.h"
#include <string.h>
#include <math.h>

int contador = 0;
int	tiempo = 0;
bool banderaTiempo = false;
double distancia;
int servo=405;
int banderaServo=0;
void conversion();


//ULTRASONIDO
void conversion (){
	distancia= 0.00001*pow(tiempo,2.0) + 0.167*tiempo + 0.8649;
}

extern"C"
{
	
	//ultrasonido
	void SysTick_Handler(void)
	{
		if(banderaTiempo){
			tiempo++;
		}
		else
		{
			if(contador == 0){
				GPIOF->ODR = 0;
			}
			contador ++;
			if(contador == 6800){
				GPIOF->ODR |= 1;
				contador=0;
			}
		}
	}

	void EXTI1_IRQHandler(void){
		if(GPIOF->IDR &= 0x2){
			tiempo = 0;
			banderaTiempo = true;			
		}
		else{
			banderaServo=1;
			banderaTiempo = false;
			conversion();
		}
		EXTI->PR |= 0;
	}
}
		
	
int main(void)
{

	RCC->AHB1ENR |= 0xFF;
	RCC->APB2ENR |= 0x4000;
	RCC->APB1ENR |= 0xFF;    //HABILITA CLOCK TIM3
	
	GPIOF->MODER |= 0x1; //pin B0 como salida y B1 como entrada
	GPIOF->OSPEEDR |= 0x3; //Velocidad muy alta
	GPIOF->OTYPER |= 0;
	GPIOF->PUPDR |= 0x5;
	
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/100000);
	
	
	SYSCFG->EXTICR[0] |= 0x50;
	EXTI->IMR |= 0x2;
	EXTI->EMR |= 0x2;
	EXTI->RTSR |= 0x2;
	EXTI->FTSR |= 0x2;
	NVIC_EnableIRQ(EXTI1_IRQn); 


		//********************************
	//CONFIGURACION PTA6 -> TIM3_CH1
	GPIOD->MODER &= ~(3UL<<2*12);     //PTA6 en MODO ALTERNO
	GPIOD->MODER |= (2UL<<2*12);      
	GPIOD->OTYPER = 0;               //PUSH PULL -> PTA6
	GPIOD->OSPEEDR &= ~(3UL<<2*12);     //MEDIUM SPEEED -> PTA6
	GPIOD->OSPEEDR |= (1UL<<2*12);
	GPIOD->PUPDR &= ~(3UL<<2*12);       //PULL-UP -> PTA6
	GPIOD->PUPDR |= (1UL<<2*12);
	GPIOD->AFR[1] = 0x20000;       //PTA6 funcion alterna  AF2= TIM3_CH1
		
	//CONFIGURACION DEL TIM3_CH1
	TIM4->EGR |= (1UL<<0);           //UG = 1 ,  RE-inicializar el contador
	TIM4->PSC = 15;                  //señal de reloj HSI=16Mhz, se necesita generar 1Mhz por lo tanto PSC=15
	TIM4->ARR = 20000;               //con una frecuencia de 1Mhz -> T=1uS :  
	TIM4->DIER |= (1UL<<0);          //UIE = 1, update interrupt enable
	                                //conteo hasta 20000 significa 20000*1uS = 20ms //periodo de la señal de control del servo
	TIM4->CR1 |= (1UL<<0);           //Enable counter
  TIM4->CCMR1 =	0x60;              //PWM modo 1, preload del CCR1 deshabilitado, CH1 configurado como salida
	TIM4->CCER |= (1UL<<0);          //OC1 signal is output on the corresponding output pin
	TIM4->CCR1 = 510;                //conteo hasta 510 significa 510*1uS = 0,51ms

	//********************
		//bucle infinito
		
	while(true)
		{	
			if(banderaServo==1){		
			servo=servo+105;
			  if(servo>2400){               //limite del PMW en este caso para un servo de referecia 
					servo=510;                  //MG996R
				}
				TIM4->CCR1 = servo;            //Actualiza el valor del PWM
				
				for(int i=0;i<200000;i++);      //anti-rebote por software
				banderaServo=0;	
			}
	}

	}
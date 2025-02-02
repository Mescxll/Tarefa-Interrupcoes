// Tarefa U4C4 feita por Maria Eduarda Santos Campos

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "interrupcoes.pio.h"

#define LEDS 25
#define PINO_LEDS 7
#define BOTAO_A 5
#define BOTAO_B 6
#define LED_VERMELHO 13
#define LED_AZUL 12

//Sm e outras variaveis definidas como estáticas para não estourar o limite de Sm por PIO
static PIO pio = pio0;
static uint sm;
static uint offset;

//Variáveis que definem a cor usada e sua intensidade -> 1 = 100%
double r = 0.0, g = 0.0, b = 1.0;

static volatile uint a = 1; //Contador para contar as pressionadas no botão
static volatile uint32_t ultimo_tempo = 0; // Váriavel que armazena o tempo do último evento
static volatile uint numero = 0; //Variável que armazena o número que está sendo mostrado

//Protótipo da funçao
static void gpio_irq_handler(uint gpio, uint32_t events);

//Matrizes dos números
double numeroZero[25] = {   0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0
};

double numeroUm[25] = {     0.0, 0.0, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.8, 0.0
};

double numeroDois[25] = {   0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.0, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0
};

double numeroTres[25] = {   0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0
};

double numeroQuatro[25] = { 0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.8, 0.0
};

double numeroCinco[25] = {  0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.0, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0
};

double numeroSeis[25] = {   0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.0, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0
};

double numeroSete[25] = {   0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 0.8, 0.0
};

double numeroOito[25] = {   0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0
};

double numeroNove[25] = {   0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.8, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0,
                            0.0, 0.8, 0.0, 0.0, 0.0,
                            0.0, 0.8, 0.8, 0.8, 0.0
};

//Matriz com as matrizes principais
double *numeros [10] = {
    numeroZero, numeroUm, numeroDois, numeroTres, numeroQuatro,
    numeroCinco, numeroSeis, numeroSete, numeroOito, numeroNove
};

//Função para configurar o PIO
void configurar_pio() {
    sm = pio_claim_unused_sm(pio, true);
    offset = pio_add_program(pio, &interrupcoes_program);
    interrupcoes_program_init(pio, sm, offset, PINO_LEDS);
}

//Função para definir a intensidade dos leds
uint32_t definirLeds(double intVermelho, double intVerde, double intAzul){
    unsigned char vermelho, verde, azul;

    vermelho = intVermelho*255;
    verde = intVerde*255;
    azul = intAzul*255;

    return (verde << 24) | (vermelho << 16) | (azul << 8);
}

//Função para fazer com que a matriz seja ativada, em uma cor especificada
void ligarMatriz(double r, double g, double b, double *desenho){
    uint32_t valorLed;
    for (int16_t i = 0; i < LEDS; i++) { 
        if(r != 0.0){
            valorLed = definirLeds(desenho[24-i], g, b);           
        }else if(g != 0.0){
            valorLed = definirLeds(r, desenho[24-i], b);    
        }else if(b != 0.0){
            valorLed = definirLeds(r, g, desenho[24-i]);           
        }
        pio_sm_put_blocking(pio, sm, valorLed);
    }
}

//Função principal
int main() {
    
    //Chama a função que configura o PIO
    configurar_pio();

    //Inicializações e definições (botões e leds)
    stdio_init_all();

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, false);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, false);
    gpio_pull_up(BOTAO_B);

    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, true);

    // Configuração da interrupção com callback
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    //Loop infinito
    while (true) {
        gpio_put(LED_VERMELHO, 1); //Acende o led vermelho 5 vezes por segundo 
        sleep_ms(200);
        gpio_put(LED_VERMELHO, 0);
        sleep_ms(200);  
    }
}

//Função de interrupção com debouncing
void gpio_irq_handler(uint gpio, uint32_t events){
    uint32_t tempo_real = to_us_since_boot(get_absolute_time()); //Obtém o tempo real

    //Debouncing
    if (tempo_real - ultimo_tempo > 200000) { //Verifica se passou 200ms desde o último evento
        if (gpio == BOTAO_A){
            numero = (numero + 1) % 10; //Incrementa 1 ao numero mostrado e obriga que ele fique entre 0 e 9 (10%)
            a++; 
        }
        if (gpio == BOTAO_B){ //Decrementa 1 ao numero mostrado e obriga que ele fique entre 0 e 9 (10%)
            numero = (numero - 1 + 10 )%10;
            //+10 obriga que ele não fique negativoro 
            a++;
        }  
        ultimo_tempo = tempo_real; //Atualiza o tempo
    }
    //Chama a função que aciona a matriz no valor especificado
    ligarMatriz(r, g, b, numeros[numero]);
}

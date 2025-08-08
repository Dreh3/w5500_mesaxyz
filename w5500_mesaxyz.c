#include <stdio.h>
#include "pico/stdlib.h"

//biblioteca para comunicação SPI
#include "hardware/spi.h"

//incluir bibliotecas do chip
#include "wizchip_conf.h"
#include "socket.h"
#include "w5500.h"

//definir pinos de comunicação SPI
#define W5500_SPI_PORT  spi0     //selecionar caso acha mais de um dispositivo por comunicação SPI
#define W5500_SPI_MISO  16   //modificar para portas definidas
#define W5500_SPI_MOSI  19
#define W5500_SPI_SCK   18
#define W5500_SPI_CS    17
#define W5500_RESET     20

//ativa pino CS
void wizchip_select(){             //linha 83 de wizchip_conf.c
    gpio_put(W5500_SPI_CS, 0);      //seleciona o dispositivo w5500
};
//desativa pino CS
void wizchip_deselect(){           //linha 91 de wizchip_conf.c
    gpio_put(W5500_SPI_CS, 1);      //deseleciona o dispositivo w5500
};
//função para receber 1 byte de dados
uint8_t wizchip_read(){                  //linha 168 de wizchip_conf.c
    uint8_t receive_data;   //para armazenar dados recebidos
    printf("Lendo dados do chip W5500 por SPI");
    spi_read_blocking (W5500_SPI_PORT, 0xFF, &receive_data, 1); //lê os dados 
    return receive_data;
};
//função para enviar 1 byte de dados
void wizchip_write(uint8_t send_data){   //linha 176 de wizchip_conf.h
    spi_write_blocking(W5500_SPI_PORT, &send_data, 1);
};
//funções para receber e enviar blocos de dados
void wizchip_readburst(uint8_t *buf, uint16_t len){             //linha 186 de wizchip_conf.c
    spi_read_blocking (W5500_SPI_PORT, 0xFF, buf, len);
};
void wizchip_writeburst(uint8_t *buf, uint16_t len){             //linha 202 de wizchip_conf.c
    spi_write_blocking(W5500_SPI_PORT, buf, len);
};

//função para resetar chip
void w5500_reset(){
    gpio_put(W5500_RESET, 0);
    sleep_ms(100);
    gpio_put(W5500_RESET, 1);
    sleep_ms(100);
};

int main()
{
    stdio_init_all();

    //chamar funções que linkam as funções definidas acima com as das bibliotecas
    // Registra callbacks para o driver
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);   //linha 302
    reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);       //linha 397
    reg_wizchip_spiburst_cbfunc(wizchip_readburst, wizchip_writeburst); //linha 415

    //inicializando os pinos SPI
    spi_init(W5500_SPI_PORT, 1000 * 1000); // 1 MHz
    gpio_set_function(W5500_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(W5500_SPI_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(W5500_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_init(W5500_SPI_CS);
    gpio_set_dir(W5500_SPI_CS, GPIO_OUT);   //definindo como gpio para ser possível selecionar ou não
    wizchip_select();                        //deixa o w5500 em estado dormente

    //Resetando o w5500
    gpio_init(W5500_RESET);
    gpio_set_dir(W5500_RESET, GPIO_OUT);
    w5500_reset();                          //reseta chip ao iniciar

    while (true) {
        printf("Teste!\n");
        sleep_ms(1000);
    }
}

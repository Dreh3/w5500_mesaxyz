#include <stdio.h>
#include "pico/stdlib.h"
#include "string.h"
//biblioteca para comunicação SPI
#include "hardware/spi.h"

//incluir bibliotecas do chip
#include "wizchip_conf.h"
#include "socket.h"
//#include "w5500.h"
#include "dhcp.h"

//para dns
//#include "dns.h"

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

wiz_NetInfo netif;

//inicialização do módulo e configuração de rede
void w5500_initialize(bool use_dhcp) {

    // Buffers para TX e RX do W5500
    //W5500 tem 16 KB para TX e 16 KB para RX
    uint8_t txbuf[8] = {2,2,2,2,2,2,2,2};  //
    uint8_t rxbuf[8] = {2,2,2,2,2,2,2,2};

    // Inicializa o chip W5500
    if (wizchip_init(txbuf, rxbuf) != 0) {
        printf("Falha ao iniciar W5500\n");
        while (1) {} // Loop infinito em caso de falha
    }

    // Estrutura para armazenar dados de rede

    //precisa inicializar o mac

    if (use_dhcp) {
        netif.dhcp = NETINFO_DHCP; //indica que será usado o dhcp
        // Configuração dinâmica via DHCP
        uint8_t dhcp_buffer[548];
        DHCP_init(0, dhcp_buffer);  //socket 0
        while (DHCP_run() != DHCP_IP_LEASED) {} // Aguarda IP
        //dhcp_get_ip_assign(&netif);
        getIPfromDHCP(netif.ip);
        getGWfromDHCP(netif.gw);
        getSNfromDHCP(netif.sn);
        getDNSfromDHCP(netif.dns);  //endereço do servidor dns
        printf("IP obtido via DHCP\n");
    } else {

        //configuração estática
        uint8_t mac[6]  = {0x00, 0x08, 0xDC, 0x01, 0x02, 0x03};
        uint8_t ip[4]   = {192, 168, 1, 123};
        uint8_t sn[4]   = {255, 255, 255, 0};
        uint8_t gw[4]   = {192, 168, 1, 1};
        uint8_t dns[4]  = {8, 8, 8, 8};

        memcpy(netif.mac, mac, sizeof(mac));
        memcpy(netif.ip,  ip,  sizeof(ip));
        memcpy(netif.sn,  sn,  sizeof(sn));
        memcpy(netif.gw,  gw,  sizeof(gw));
        memcpy(netif.dns, dns, sizeof(dns));
        
        netif.dhcp = NETINFO_STATIC;                            // Modo estático
        wizchip_setnetinfo(&netif);                             // Aplica configuração
    }

}

//para dns
uint8_t dest_ip[4]; // Vai guardar o IP convertido
uint8_t dns_server[4] = {8,8,8,8}; // Servidor DNS do Google  -- tem o armazenado em netif.dns
/*
void resolve_hostname() {
    int8_t ret;

    // Inicializa o DNS no socket 0
    DNS_init(0, (uint8_t *)""); 

    printf("Resolvendo host...\n");

    // Faz a consulta de DNS
    ret = gethostbyname((uint8_t *)"meuservidor.com", dest_ip, dns_server);

    if(ret == 1) {
        printf("IP resolvido: %d.%d.%d.%d\n", dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3]);
    } else {
        printf("Falha ao resolver host, erro %d\n", ret);
    }
}
*/

// ==== Exibe configuração de rede e status físico ====
void print_network_info(void) {
    //wiz_NetInfo netif;
    wizchip_getnetinfo(&netif);

    // Exibe IP, gateway e máscara
    printf("IP: %d.%d.%d.%d\n", netif.ip[0], netif.ip[1], netif.ip[2], netif.ip[3]);
    printf("GW: %d.%d.%d.%d\n", netif.gw[0], netif.gw[1], netif.gw[2], netif.gw[3]);
    printf("SN: %d.%d.%d.%d\n", netif.sn[0], netif.sn[1], netif.sn[2], netif.sn[3]);

    // Verifica link físico (cabo conectado)
    uint8_t link = 0;
    ctlwizchip(CW_GET_PHYLINK, (void*)&link);
    printf("Link físico: %s\n", link ? "Sim" : "Não");
}

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
    wizchip_deselect();                        //deixa o w5500 em estado dormente

    //Resetando o w5500
    gpio_init(W5500_RESET);
    gpio_set_dir(W5500_RESET, GPIO_OUT);
    w5500_reset();                          //reseta chip ao iniciar

    printf("Iniciando W5500...\n");

     // Inicializa W5500: false = IP estático, true = DHCP
    w5500_initialize(false);

    printf("Configuração de rede:\n");
    print_network_info();

    // ==== Exemplo simples de comunicação TCP ====
    // Abre socket TCP na porta 5000
    int sock = socket(0, Sn_MR_TCP, 5000, 0);

    // Define IP de destino -- modelo fixo
    uint8_t dest[4] = {192,168,1,100};

    //com dns chama-se a função
    //resolve_hostname();

    // Conecta ao destino
    connect(sock, dest, 5000);

    // Envia mensagem
    send(sock, (uint8_t*)"Olá Pico!", 9);

    // Recebe resposta
    uint8_t buf[64];
    int len = recv(sock, buf, sizeof(buf));
    if (len > 0) {
        buf[len] = 0; // Termina string
        printf("Recebido: %s\n", buf);
    }

    while (true) {
        printf("Teste!\n");
        sleep_ms(1000);
    }
}

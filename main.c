#define _XTAL_FREQ 4000000

#include <xc.h>
#include <pic16f628a.h>

#define HIGH 1
#define LOW 0

// BEGIN CONFIG
#pragma config LVP = OFF
#pragma config CP = OFF
#pragma config BOREN = ON
#pragma config MCLRE = OFF
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config FOSC = INTOSCIO
//END CONFIG

typedef struct {
    char i;
    char j;
} Ponto;

void enviarParaDisplay(char parteAlta, char parteBaixa);
void finalizarEnvio();
void desenharMatriz(char [8]);
void inicializarDisplay();
inline void renderizar(char * mapa, Ponto aux);
//inline void ativarTrigger();
void inicializarJogo(char * mapa, Ponto * ovo, Ponto * kobra);

char tamanho_kobra = 1;
char dir = 0x2;
char inicio = 0;

char check = 0;
char pause = 1;

void __interrupt () echo (void){
    GIE = 0;
    if (RBIF == 1){
        if (RB4 == 1){
            if(pause) 
                pause = !pause;            
            dir = ++dir%4;
            while(RB4 == 1) __delay_us(10000);                
        }        
        RBIF = 0;
    }
    GIE = 1;
}

void main(void) {
    char semente;
    char i;
    Ponto kobra[32];
    Ponto ovo;      
    
    CMCON = 0x07;
    TRISA = 0b00000000;
    TRISB = 0b00010000;
    
    GIE = 1; //Habilitar interrup??es globais
    RBIF = 0; //Limpar a flag de mudan?a em RB
    RBIE = 1; //Habilitar interrup??o por mudan?a na Porta B    
    
    char mapa[] = {
        0b00000000,
        0b00000000,    
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    };
    
    inicializarDisplay();
    
    while(1) {   
        if(!pause) {
            if(kobra[0].i == ovo.i && kobra[0].j == ovo.j){
                tamanho_kobra++;
                semente = ((semente | kobra[0].i) & (semente | ~kobra[0].j))%64;
                ovo.i = semente/8;
                ovo.j = semente%8;            
            }
            
            for(i = 1; i < tamanho_kobra; i++)
                if(kobra[i].j == kobra[0].j && kobra[i].i == kobra[0].i) pause = 1;

            for(i = 0; i < tamanho_kobra; i++) {
                renderizar(mapa, kobra[i]);
            }

            for(i = tamanho_kobra - 1; i > 0; --i) {
                kobra[i].i = kobra[i-1].i;
                kobra[i].j = kobra[i-1].j;
            }

            switch(dir) {
                case 0x00: kobra[0].i = (--kobra[0].i)%8; break;
                case 0x01: kobra[0].j = (++kobra[0].j)%8; break;
                case 0x02: kobra[0].i = (++kobra[0].i)%8; break;
                case 0x03: kobra[0].j = (--kobra[0].j)%8; break;            
            }

            renderizar(mapa, ovo); 
            desenharMatriz(mapa);
            __delay_ms(200);

            semente += kobra[0].i * 8 + kobra[0].j;        
            for(i = 0; i < 8; i++)
                mapa[i] = 0b0;
        } else {
            inicializarJogo(mapa, &ovo, kobra);
            mapa[0] = 0b00000011;
            mapa[1] = 0b00000111;
            mapa[2] = 0b00001110;
            mapa[3] = 0b10011100;
            mapa[4] = 0b10111000;
            mapa[5] = 0b11110000;
            mapa[6] = 0b11100000;
            mapa[7] = 0b11111000;
            desenharMatriz(mapa);
        }
    }
    return;
}

void inicializarJogo(char * mapa, Ponto * ovo, Ponto * kobra){
    mapa[0] = 0b00000000;
    mapa[1] = 0b00000000;
    mapa[2] = 0b00000000;
    mapa[3] = 0b00000000;
    mapa[4] = 0b00000000;
    mapa[5] = 0b00000000;
    mapa[6] = 0b00000000;
    mapa[7] = 0b00000000;
    
    ovo->i = 7;
    ovo->j = 2;
    
    kobra[0].i = 0;
    kobra[0].j = 2; 
    tamanho_kobra = 1;
}


inline void renderizar(char * mapa, Ponto aux){
    mapa[aux.i] |= 0x80 >> aux.j;
}

void inicializarDisplay() {
    char parteAlta = 0b00001100; // shutdown register
    char parteBaixa = 0b00000001; //modo normal de opera??o
    enviarParaDisplay(parteAlta, parteBaixa);
    
    parteAlta = 0b00001001;
    parteBaixa = 0b00000000;
    enviarParaDisplay(parteAlta, parteBaixa); // Sem decode para os bits dos registradores
    
    parteAlta = 0b00001011;
    parteBaixa = 0x07;
    enviarParaDisplay(parteAlta, parteBaixa); // M?ximo de digitos escaneados
}

void desenharMatriz(char vetor[8]){
    char temp[8] = {0x00};
    char indice = 0x80;
    for(char i = 0; i < 8; i++){
        for(char j = 0; j < 8; j++){
            temp[i] |= (vetor[j] & indice) << i >> (7 - j);
        }
        indice = indice >> 1;
    }
    for(char i = 0; i < 8; i++){
        enviarParaDisplay(i + 1, temp[i]);
    }
}

void enviarParaDisplay(char parteAlta, char parteBaixa){
    RA0 = LOW;
    for(int i = 0; i < 8; i++){
        char temp = parteAlta & 128;        
        RA2 = (temp == 128 ? HIGH : LOW);
        RA0 = HIGH;
        __delay_us(100);
        RA0 = LOW;
        parteAlta = parteAlta << 1;
    }
    
    for(int i = 0; i < 8; i++){
        char temp = parteBaixa & 128;        
        RA2 = (temp == 128 ? HIGH : LOW);
        RA0 = HIGH;
        __delay_us(100);
        RA0 = LOW;        
        parteBaixa = parteBaixa << 1;
    }
    finalizarEnvio();
}

void finalizarEnvio(){
    RA1 = HIGH;
    __delay_us(100);
    RA1 = LOW;
}

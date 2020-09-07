#include <main_master.h>
#use rs232(baud=9600, xmit=PIN_C6,rcv=PIN_C7,parity=N, bits=8)
                                                           
#define DISPLAY_SS   PIN_D1    
                                
// Modos de comunicaci�n SPI         
#define SPI_MODE_0  (SPI_L_TO_H | SPI_XMIT_L_TO_H)           
#define SPI_MODE_1  (SPI_L_TO_H) 
#define SPI_MODE_2  (SPI_H_TO_L)                          
#define SPI_MODE_3  (SPI_H_TO_L | SPI_XMIT_L_TO_H)             
                             
#byte PORTA = 0xF80    // Direcci�n del Registro para 18F4520
#byte PORTB = 0xF81    // Direcci�n del Registro para 18F4520     
#byte PORTC = 0xF82    // Direcci�n del Registro para 18F4520
#byte PORTD = 0xF83    // Direcci�n del Registro para 18F4520                                             
#byte SSPBUF = 0xFC9   // Direcci�n del Registro para 18F4520 
                                                                                    
unsigned int8 valor;
         
         
#int_RDA
void RDA_isr(){     

   valor=getc();

} 
 

void write_display(unsigned int8 n){ 

   output_low(DISPLAY_SS);                          
   spi_write(n);                                   
   output_high(DISPLAY_SS);                
   delay_ms(10);          
}
           
void main() {    

   valor = 0;                  
   output_high(DISPLAY_SS); 
   SETUP_SPI(SPI_MASTER | SPI_MODE_0 | SPI_CLK_DIV_4 | SPI_SAMPLE_AT_END);  
   delay_ms(10);
   CLEAR_INTERRUPT(INT_RDA);     
   ENABLE_INTERRUPTS(INT_RDA);                                                        
   ENABLE_INTERRUPTS(GLOBAL);                              
                        
   while(TRUE) {             // Mai Loop.  
                                                             
      switch (valor){                    
         case 'R':write_display(valor);
                  delay_ms(10);  
                  valor = 0;
                  reset_cpu();  
                  break;          
         default:write_display(valor);
                 break;   
      }   
   }                           
}                                                                    

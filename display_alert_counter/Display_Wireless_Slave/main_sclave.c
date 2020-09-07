#include <main_sclave.h> 
                                                   
#define DISPLAY_1 PIN_C0 
#define DISPLAY_2 PIN_C1  
#define nBits 8     

// Modos de comunicación SPI
#define SPI_MODE_0  (SPI_L_TO_H | SPI_XMIT_L_TO_H) 
#define SPI_MODE_1  (SPI_L_TO_H) 
#define SPI_MODE_2  (SPI_H_TO_L) 
#define SPI_MODE_3  (SPI_H_TO_L | SPI_XMIT_L_TO_H)  

#byte PORTA = 0x05    // Dirección del Registro para 16F886 
#byte PORTB = 0x06    // Dirección del Registro para 16F886     
#byte PORTC = 0x07    // Dirección del Registro para 16F886 
#byte SSPBUF = 0x13   // Dirección del Registro para 16F886 
#byte SSPCON = 0x14   // Dirección del Registro para 16F886 

byte display_segments[nBits] = {PIN_B4, PIN_B3, PIN_B2, PIN_B1, PIN_A3, PIN_A2, PIN_A1, PIN_A0};   
byte digitos;           
int1 Enable_Display;                              
unsigned int8 valor;

                             
#int_ssp 
void ssp_isr(void){ 
                       
   valor = SSPBUF; 
                  
}                              

           
void set_segments(byte n) {         // Set bits 7 Segments.                           
                 
   for (byte i = 0; i < nBits; i++) {
      if( n & 1 )
         output_high(display_segments[i]);        
      else                
         output_low(display_segments[i]);        
      n /= 2;               
   }                     
}   
    
    
void dispaly_init(){  
   unsigned int8 shifter = 0b10000000;  
   output_low(DISPLAY_1);                                                                         
   output_high(DISPLAY_2);
   for(unsigned int8 i=0; i<8;i++){
      set_segments(0);              
      set_segments(shifter);   
      shifter>>=1;           
      delay_ms(100);
   }                            
   delay_ms(800); 
   shifter = 0b10000000;  
   output_high(DISPLAY_1);                                                                         
   output_low(DISPLAY_2); 
   for(unsigned int8 j=0; j<8;j++){
      set_segments(0);                                         
      set_segments(shifter);   
      shifter>>=1;           
      delay_ms(100);       
   }   
   delay_ms(800);
   output_high(DISPLAY_1);                                                                         
   output_high(DISPLAY_2);  
   set_segments(0b11111111);
   delay_ms(3000); 
                       
} 
       
      
byte numero(unsigned int v){        // Encoding num Byte to 7 Segments.
     
      switch(v){
         case 0:
                      //ABCDEFGP
               return(0b11111100); // 0
               break;  
         case 1:
                      //ABCDEFGP
               return(0b01100000); // 1   
               break;                                               
         case 2:
                      //ABCDEFGP
               return(0b11011010); // 2  
               break;    
         case 3:                     
                     //ABCDEFGP
               return(0b11110010); // 3 
               break;                               
         case 4:
                      //ABCDEFGP                         
               return(0b01100110); // 4  
               break; 
         case 5:
                      //ABCDEFGP             
               return(0b10110110); // 5  
               break;       
         case 6:           
                      //ABCDEFGP
               return(0b00111110); // 6 
               break;          
         case 7:         
                      //ABCDEFGP       
               return(0b11100000); // 7  
               break;   
         case 8:                          
                      //ABCDEFGP  
               return(0b11111110); // 8 
               break;  
         case 9:
                      //ABCDEFGP                                       
               return(0b11100110); // 9   
               break;  
         case 'Q':    
                      //ABCDEFGP                                       
               return(0b00000010); // -   
               break;   
         default:                             
               break;                                     
      }                                        
                                                                                    
}  
                      
#INT_RTCC                     // Interrupción timer0 Mux Display.
void show_display(){  

   if(Enable_Display){    
      switch(digitos){
         case 0: 
               set_segments(0);                                   
               output_high(display_1);                    
               output_low(display_2);
               if(valor == 'R'){
                  reset_cpu();             
               }else if(valor == 'Q'){        
                  set_segments(numero(valor));
               }else{ 
                  set_segments(numero((valor / 10) % 10));
               }             
               digitos = 1;
               break;                                
         case 1:   
               set_segments(0);                         
               output_low(display_1);                    
               output_high(display_2);
               if(valor == 'R'){
                  reset_cpu();    
               }else if(valor == 'Q'){        
                  set_segments(numero(valor));
               }else{   
                  set_segments(numero(valor % 10));
               }
               digitos = 0;                           
               break;                        
         default: 
               break;         
      }                                   
   }else{
      set_segments(0b00000000);                                   
      output_low(display_1);                    
      output_low(display_2);   
   }                                               
                                                   
   SET_RTCC(236);  
}                        
                                     
                                                
void main(){                       

   output_low(DISPLAY_1);                                                                         
   output_low(DISPLAY_2);        
   set_segments(0);        
   digitos = 0;
   SSPBUF = 0x00; 
   Enable_Display = true;                                          
   valor = 0;  
   dispaly_init();                                                           
   SETUP_SPI(SPI_SLAVE | SPI_MODE_0);   
   CLEAR_INTERRUPT(INT_SSP);          
   ENABLE_INTERRUPTS(INT_SSP);
   SETUP_TIMER_0(RTCC_INTERNAL|RTCC_DIV_256|RTCC_8_bit);                
   ENABLE_INTERRUPTS(INT_RTCC);
   ENABLE_INTERRUPTS(GLOBAL);
   SET_RTCC(236); 
                                                          
                                                                           
   while(TRUE){         // Main Loop.        
                                  
                              
   }                  
}                                              

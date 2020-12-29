#include <16F886.h>
#device ADC=16
#FUSES HS   
#FUSES NOWDT                    
#FUSES NOBROWNOUT               
#FUSES NOLVP                    

#use delay(crystal=20000000)
#use FIXED_IO( A_outputs=PIN_A3,PIN_A2,PIN_A1,PIN_A0 ) 
#use FIXED_IO( B_outputs=PIN_B4,PIN_B3,PIN_B2,PIN_B1 ) 
#use FIXED_IO( C_outputs=PIN_C0,PIN_C1 ) 
                                                    

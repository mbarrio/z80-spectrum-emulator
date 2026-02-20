  gethdr:                               
9c40 21409c   LD   HL,9c40              
9c43 e5       PUSH HL                   
9c44 dd21007f LD   IX,7f00              
9c48 111100   LD   DE,0011              
9c4b af       XOR  A                    
9c4c 37       SCF                       
9c4d cd5605   CALL 0556,ld_bytes        
9c50 d0       RET  NC                   
9c51 3e47     LD   A,47                 
9c53 ddbefe   CP   (IX-02)              
9c56 c0       RET  NZ                   
9c57 3e4c     LD   A,4c                 
9c59 ddbeff   CP   (IX-01)              
9c5c c0       RET  NZ                   
9c5d dd7efc   LD   A,(IX-04)            
9c60 fe08     CP   08                   
9c62 3827     JR   C,9c8b,blk128        
9c64 2816     JR   Z,9c7c,blklow        
9c66 fe0a     CP   0a                   
9c68 3828     JR   C,9c92,blk48         
9c6a 2836     JR   Z,9ca2,endblk        
9c6c dd210040 LD   IX,4000              
9c70 11001b   LD   DE,1b00              
  ldblk:                                
9c73 3eff     LD   A,ff                 
9c75 37       SCF                       
9c76 cd5605   CALL 0556,ld_bytes        
9c79 d8       RET  C                    
9c7a cf       RST  #08                  
9c7b 1a       LD   A,(DE)               
  blklow:                               
9c7c cd929c   CALL 9c92,blk48           
9c7f 2100c0   LD   HL,c000              
9c82 110080   LD   DE,8000              
9c85 010040   LD   BC,4000              
9c88 edb0     LDIR                      
9c8a c9       RET                       
  blk128:                               
9c8b 01fd7f   LD   BC,7ffd              
9c8e f610     OR   10                   
9c90 ed79     OUT  (C),A                
  blk48:                                
9c92 dd2100c0 LD   IX,c000              
9c96 ed5b0b7f LD   DE,(7f0b)            
9c9a cd739c   CALL 9c73,ldblk           
9c9d af       XOR  A                    
9c9e d3fe     OUT  (fe),A               
9ca0 181b     JR   9cbd,unpack          
  endblk:                               
9ca2 dd7efd   LD   A,(IX-03)            
9ca5 ed47     LD   I,A                  
9ca7 37       SCF                       
9ca8 9f       SBC  A,A                  
9ca9 dd210040 LD   IX,4000              
9cad 110040   LD   DE,4000              
9cb0 31e657   LD   SP,57e6              
9cb3 08       EX   AF,AF'               
9cb4 f3       DI                        
9cb5 3e0f     LD   A,0f                 
9cb7 d3fe     OUT  (fe),A               
9cb9 cd6205   CALL 0562                 
9cbc c7       RST  #00                  
  unpack:                               
9cbd ed4b0b7f LD   BC,(7f0b)            
9cc1 05       DEC  B                    
9cc2 c5       PUSH BC                   
9cc3 2100c0   LD   HL,c000              
9cc6 09       ADD  HL,BC                
9cc7 11007f   LD   DE,7f00              
9cca 010001   LD   BC,0100              
9ccd edb0     LDIR                      
9ccf c1       POP  BC                   
9cd0 78       LD   A,B                  
9cd1 fe3f     CP   3f                   
9cd3 307a     JR   NC,9d4f,mov256       
9cd5 21ffbf   LD   HL,bfff              
9cd8 09       ADD  HL,BC                
9cd9 11ffff   LD   DE,ffff              
9cdc edb8     LDDR                      
9cde 210100   LD   HL,0001              
9ce1 19       ADD  HL,DE                
9ce2 1100c0   LD   DE,c000              
9ce5 37       SCF                       
  fetch:                                
9ce6 4e       LD   C,(HL)               
9ce7 23       INC  HL                   
9ce8 cb11     RL   C                    
9cea 1808     JR   9cf4,loop0           
  loop:                                 
9cec 14       INC  D                    
9ced 2860     JR   Z,9d4f,mov256        
9cef 15       DEC  D                    
9cf0 cb21     SLA  C                    
9cf2 28f2     JR   Z,9ce6,fetch         
  loop0:                                
9cf4 3005     JR   NC,9cfb,block        
9cf6 eda0     LDI                       
9cf8 03       INC  BC                   
9cf9 18f1     JR   9cec,loop            
  block:                                
9cfb cb21     SLA  C                    
9cfd 2005     JR   NZ,9d04,bit1         
9cff 4e       LD   C,(HL)               
9d00 23       INC  HL                   
9d01 37       SCF                       
9d02 cb11     RL   C                    
  bit1:                                 
9d04 3026     JR   NC,9d2c,large        
9d06 d5       PUSH DE                   
9d07 d5       PUSH DE                   
9d08 0608     LD   B,08                 
9d0a cd3c9d   CALL 9d3c,getpar          
9d0d e3       EX   (SP),HL              
9d0e ed52     SBC  HL,DE                
9d10 e3       EX   (SP),HL              
9d11 0604     LD   B,04                 
  copybl:                               
9d13 cd3c9d   CALL 9d3c,getpar          
9d16 ed435d9d LD   (9d5d),BC            
9d1a 225b9d   LD   (9d5b),HL            
9d1d 42       LD   B,D                  
9d1e 4b       LD   C,E                  
9d1f e1       POP  HL                   
9d20 d1       POP  DE                   
9d21 edb0     LDIR                      
9d23 2a5b9d   LD   HL,(9d5b)            
9d26 ed4b5d9d LD   BC,(9d5d)            
9d2a 18c0     JR   9cec,loop            
  large:                                
9d2c d5       PUSH DE                   
9d2d d5       PUSH DE                   
9d2e 060e     LD   B,0e                 
9d30 cd3c9d   CALL 9d3c,getpar          
9d33 e3       EX   (SP),HL              
9d34 a7       AND  A                    
9d35 ed52     SBC  HL,DE                
9d37 e3       EX   (SP),HL              
9d38 060a     LD   B,0a                 
9d3a 18d7     JR   9d13,copybl          
  getpar:                               
9d3c 110000   LD   DE,0000              
9d3f eb       EX   DE,HL                
  gpl:                                  
9d40 cb21     SLA  C                    
9d42 2005     JR   NZ,9d49,gp1          
9d44 1a       LD   A,(DE)               
9d45 13       INC  DE                   
9d46 4f       LD   C,A                  
9d47 cb11     RL   C                    
  gp1:                                  
9d49 ed6a     ADC  HL,HL                
9d4b 10f3     DJNZ 9d40,gpl             
9d4d eb       EX   DE,HL                
9d4e c9       RET                       
  mov256:                               
9d4f 21007f   LD   HL,7f00              
9d52 1100ff   LD   DE,ff00              
9d55 010001   LD   BC,0100              
9d58 edb0     LDIR                      
9d5a c9       RET                       
9d5b 00       NOP                       

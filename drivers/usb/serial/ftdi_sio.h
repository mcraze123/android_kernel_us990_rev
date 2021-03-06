/*
                                                                     
                                                                     
  
                                                                  
  
  
                                                                      
                                                                 
                                                                         
                                                              
  
                                                                             
                    
  
                                                                          
                                                                        
                      
  
                                                                            
                           
  
 */

/*          */
#define FTDI_SIO_RESET			0 /*                */
#define FTDI_SIO_MODEM_CTRL		1 /*                                */
#define FTDI_SIO_SET_FLOW_CTRL		2 /*                           */
#define FTDI_SIO_SET_BAUD_RATE		3 /*               */
#define FTDI_SIO_SET_DATA		4 /*                                
                   */
#define FTDI_SIO_GET_MODEM_STATUS	5 /*                                
                          */
#define FTDI_SIO_SET_EVENT_CHAR		6 /*                         */
#define FTDI_SIO_SET_ERROR_CHAR		7 /*                         */
#define FTDI_SIO_SET_LATENCY_TIMER	9 /*                       */
#define FTDI_SIO_GET_LATENCY_TIMER	10 /*                       */

/*                                                           */
#define INTERFACE_A		1
#define INTERFACE_B		2
#define INTERFACE_C		3
#define INTERFACE_D		4


/*
                               
                                 
                      
                                            
                      
                                                              
  
 */

/*                       */
#define PIT_DEFAULT		0 /*      */
#define PIT_SIOA		1 /*      */
/*                                                             */
#define PIT_SIOB		2 /*      */
#define PIT_PARALLEL		3 /*          */

/*                */
#define FTDI_SIO_RESET_REQUEST FTDI_SIO_RESET
#define FTDI_SIO_RESET_REQUEST_TYPE 0x40
#define FTDI_SIO_RESET_SIO 0
#define FTDI_SIO_RESET_PURGE_RX 1
#define FTDI_SIO_RESET_PURGE_TX 2

/*
                             
                                 
                                
                                  
                                        
                                        
                       
                    
                       
  
                                         
  
                                     
                      
                              
                     
                     
               
               
                                    
  
                                                                        
  
   */

/*                       */
#define FTDI_SIO_SET_BAUDRATE_REQUEST_TYPE 0x40
#define FTDI_SIO_SET_BAUDRATE_REQUEST 3

/*
                             
                                        
                                                
                       
                    
                       
                                                    
                                                                      
                                                                              
                                                                
                                
                                                                           
                                                                        
                                                                       
                      
                                                                       
                                                             
                                                                               
                                                      
                                                                              
                                                                       
                   
                                                              
                                                
                                                         
                                                          
                                                           
          
  
                                                                             
                                                                           
                                                                           
             
                              
                              
                              
                              
                              
                              
                              
                              
                                                                             
                                    
  
                                                                            
                                                                               
                                                                              
                                                                              
                                   
 */

enum ftdi_chip_type {
	SIO = 1,
	FT8U232AM = 2,
	FT232BM = 3,
	FT2232C = 4,
	FT232RL = 5,
	FT2232H = 6,
	FT4232H = 7,
	FT232H  = 8,
	FTX     = 9,
};

enum ftdi_sio_baudrate {
	ftdi_sio_b300 = 0,
	ftdi_sio_b600 = 1,
	ftdi_sio_b1200 = 2,
	ftdi_sio_b2400 = 3,
	ftdi_sio_b4800 = 4,
	ftdi_sio_b9600 = 5,
	ftdi_sio_b19200 = 6,
	ftdi_sio_b38400 = 7,
	ftdi_sio_b57600 = 8,
	ftdi_sio_b115200 = 9
};

/*
                                                                               
                                    
 */
#define FTDI_SIO_SET_DATA_REQUEST	FTDI_SIO_SET_DATA
#define FTDI_SIO_SET_DATA_REQUEST_TYPE	0x40
#define FTDI_SIO_SET_DATA_PARITY_NONE	(0x0 << 8)
#define FTDI_SIO_SET_DATA_PARITY_ODD	(0x1 << 8)
#define FTDI_SIO_SET_DATA_PARITY_EVEN	(0x2 << 8)
#define FTDI_SIO_SET_DATA_PARITY_MARK	(0x3 << 8)
#define FTDI_SIO_SET_DATA_PARITY_SPACE	(0x4 << 8)
#define FTDI_SIO_SET_DATA_STOP_BITS_1	(0x0 << 11)
#define FTDI_SIO_SET_DATA_STOP_BITS_15	(0x1 << 11)
#define FTDI_SIO_SET_DATA_STOP_BITS_2	(0x2 << 11)
#define FTDI_SIO_SET_BREAK		(0x1 << 14)
/*                   */

/*
                             
                                    
                                                   
                       
                    
                     
  
                       
  
                                
                   
                     
                    
                     
                     
                      
                      
                  
                    
                  
        
                              
                                      
                 
  
 */



/*                     */
#define FTDI_SIO_SET_MODEM_CTRL_REQUEST_TYPE 0x40
#define FTDI_SIO_SET_MODEM_CTRL_REQUEST FTDI_SIO_MODEM_CTRL

/*
                              
                                       
                                            
                        
                     
                        
  
                                                                      
                                                          
                                                              
 */

#define FTDI_SIO_SET_DTR_MASK 0x1
#define FTDI_SIO_SET_DTR_HIGH (1 | (FTDI_SIO_SET_DTR_MASK  << 8))
#define FTDI_SIO_SET_DTR_LOW  (0 | (FTDI_SIO_SET_DTR_MASK  << 8))
#define FTDI_SIO_SET_RTS_MASK 0x2
#define FTDI_SIO_SET_RTS_HIGH (2 | (FTDI_SIO_SET_RTS_MASK << 8))
#define FTDI_SIO_SET_RTS_LOW (0 | (FTDI_SIO_SET_RTS_MASK << 8))

/*
               
                  
                     
                   
                  
                     
                   
                 
                         
                      
                             
                         
                      
                             
                   
 */

/*                        */
#define FTDI_SIO_SET_FLOW_CTRL_REQUEST_TYPE 0x40
#define FTDI_SIO_SET_FLOW_CTRL_REQUEST FTDI_SIO_SET_FLOW_CTRL
#define FTDI_SIO_DISABLE_FLOW_CTRL 0x0
#define FTDI_SIO_RTS_CTS_HS (0x1 << 8)
#define FTDI_SIO_DTR_DSR_HS (0x2 << 8)
#define FTDI_SIO_XON_XOFF_HS (0x4 << 8)
/*
                               
                                           
                             
                                                                        
                      
                         
  
                      
                                        
                     
                    
                                        
                     
                    
                            
                     
                    
  
                                                           
  
                                                                            
                                                                  
 */

/*
                             
  
                                                                  
                                                                  
                                                                  
                                                                  
                                                                   
                                                  
 */
#define  FTDI_SIO_GET_LATENCY_TIMER_REQUEST FTDI_SIO_GET_LATENCY_TIMER
#define  FTDI_SIO_GET_LATENCY_TIMER_REQUEST_TYPE 0xC0

/*
                               
                                               
                      
                         
                      
                                        
 */

/*
                             
  
                                                                  
                                                                  
                                                                  
                                                                  
                                                                   
                                                  
 */
#define  FTDI_SIO_SET_LATENCY_TIMER_REQUEST FTDI_SIO_SET_LATENCY_TIMER
#define  FTDI_SIO_SET_LATENCY_TIMER_REQUEST_TYPE 0x40

/*
                               
                                               
                                           
                         
                      
                         
  
          
                          
              
  
 */

/*
                          
  
                                                                         
                                                                   
                                                                      
                                  
 */


#define  FTDI_SIO_SET_EVENT_CHAR_REQUEST FTDI_SIO_SET_EVENT_CHAR
#define  FTDI_SIO_SET_EVENT_CHAR_REQUEST_TYPE 0x40


/*
                               
                                            
                              
                         
                      
                         
  
          
                            
                                       
                           
                          
                     
  
 */

/*                         */

/*
                                                                              
       
 */

/*
                              
                                           
                              
                        
                     
                        
  
            
                          
                                     
                         
                        
                   
  
 */

/*                           */
/*                                                         */

#define FTDI_SIO_GET_MODEM_STATUS_REQUEST_TYPE 0xc0
#define FTDI_SIO_GET_MODEM_STATUS_REQUEST FTDI_SIO_GET_MODEM_STATUS
#define FTDI_SIO_CTS_MASK 0x10
#define FTDI_SIO_DSR_MASK 0x20
#define FTDI_SIO_RI_MASK  0x40
#define FTDI_SIO_RLSD_MASK 0x80
/*
                                
                                               
                          
                          
                       
                            
  
                               
          
            
                       
                     
            
                       
                     
                            
                       
                     
                                          
                       
                     
 */



/*                                   
  
                     
  
                                       
                                                
                                                  
                                             
                                   
                                         
                                         
                                                              
                                
                                                  
                                              
                                                    
                                               
                                                          
                                                                    
  
                           
  
                            
                                                 
                                                          
                                                
                                                          
                                                                   
                                                             
                                                              
                                           
  
                       
  
                            
                                                 
                                                      
                                                
                                                            
                                              
                                       
                                            
                                            
                                                             
  
                         
  
                            
                                                 
                                                     
                                                
                                                    
                                                
                                                    
  
                          
  
                            
                                                 
                                                     
                                                
                                                    
                                                
                                                    
  
              
  
              
  
                                                                              
                                                                               
                                                                            
              
  
                       
  
                     
                          
                          
                          
                          
                         
                          
                         
                                       
  
                      
  
                     
                     
                        
                       
                        
                          
                                         
                              
                        
  
 */
#define FTDI_RS0_CTS	(1 << 4)
#define FTDI_RS0_DSR	(1 << 5)
#define FTDI_RS0_RI	(1 << 6)
#define FTDI_RS0_RLSD	(1 << 7)

#define FTDI_RS_DR	1
#define FTDI_RS_OE	(1<<1)
#define FTDI_RS_PE	(1<<2)
#define FTDI_RS_FE	(1<<3)
#define FTDI_RS_BI	(1<<4)
#define FTDI_RS_THRE	(1<<5)
#define FTDI_RS_TEMT	(1<<6)
#define FTDI_RS_FIFO	(1<<7)

/*
               
  
                                                                            
                                                                               
                                   
  
                      
  
                     
                          
                          
                                                   
  
 */

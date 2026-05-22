
// This is the command sequence that initialises the ST7796 driver
//
// This setup information uses simple 8-bit SPI writecommand() and writedata() functions
//
// See ST7735_Setup.h file for an alternative format

#define TFT_INIT_DELAY 0
{

 	writecommand(0x11);     

	delay(120);                //Delay 120ms

	writecommand(0x36);     // Memory Data Access Control MY,MX~~
	writedata(0x48);   

	writecommand(0x3A);     
	writedata(0x55);   

	writecommand(0xF0);     // Command Set Control
	writedata(0xC3);   

	writecommand(0xF0);     
	writedata(0x96);   

	writecommand(0xB4);     
	writedata(0x02);   

	writecommand(0xB7);     
	writedata(0xC6);   

	writecommand(0xC0);     
	writedata(0xC0);   
	writedata(0x00);   

	writecommand(0xC1);     
	writedata(0x13);   

	writecommand(0xC2);     
	writedata(0xA7);   

	writecommand(0xC5);     
	writedata(0x21);   

	writecommand(0xE8);     
	writedata(0x40);   
	writedata(0x8A);   
	writedata(0x1B);   
	writedata(0x1B);   
	writedata(0x23);   
	writedata(0x0A);   
	writedata(0xAC);   
	writedata(0x33);   

	writecommand(0xE0);     
	writedata(0xD2);   
	writedata(0x05);   
	writedata(0x08);   
	writedata(0x06);   
	writedata(0x05);   
	writedata(0x02);   
	writedata(0x2A);   
	writedata(0x44);   
	writedata(0x46);   
	writedata(0x39);   
	writedata(0x15);   
	writedata(0x15);   
	writedata(0x2D);   
	writedata(0x32);   

	writecommand(0xE1);     
	writedata(0x96);   
	writedata(0x08);   
	writedata(0x0C);   
	writedata(0x09);   
	writedata(0x09);   
	writedata(0x25);   
	writedata(0x2E);   
	writedata(0x43);   
	writedata(0x42);   
	writedata(0x35);   
	writedata(0x11);   
	writedata(0x11);   
	writedata(0x28);   
	writedata(0x2E);   

	writecommand(0xF0);     
	writedata(0x3C);   
	writecommand(0xF0);     
	writedata(0x69);  
	
	writecommand(0x21); 
  
 	end_tft_write();
  	delay(120);
  	begin_tft_write();

	writecommand(0x29); //Display on                                          	
}
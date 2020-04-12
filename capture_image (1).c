#include "address_map_arm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define KEY_BASE              0xFF200050
#include "time.h"
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_START		  0xC9000000
#define SW_BASE				  0xFF200040
/* This program demonstrates the use of the D5M camera with the DE1-SoC Board
 * It performs the following: 
 * 	1. Capture one frame of video when any key is pressed.
 * 	2. Display the captured frame when any key is pressed.		  
*/
/* Note: Set the switches SW1 and SW2 to high and rest of the switches to low for correct exposure timing while compiling and the loading the program in the Altera Monitor program.
*/
	
void printMenu(char* textptr, int offset){
	while (*(textptr)){
		*((char*)FPGA_CHAR_START	+offset) = *(textptr);
		textptr++;
		offset++;
	}
}

void updateMenu(int numPics){
	printMenu("                              ",5<<7);
	printMenu("                              ",6<<7);
	
	//char * test = itoa(numPics);
	//char buf[3]= "   ";
	//char buf2[15];

	//sprintf(buf2, "%s",ctime(&now));
	//sprintf(buf, "%d", numPics);
	char str[50];
	sprintf(str,"Number of Pics: %d",numPics);
	
	
	
	printMenu(str, 5<<7);
	//printMenu(str2, 6<<7);
	
}

int main(void)
{	
	volatile int * SW_ptr = (int *) SW_BASE;
	volatile int * KEY_ptr				= (int *) KEY_BASE;
	volatile int * Video_In_DMA_ptr	= (int *) VIDEO_IN_BASE;
	volatile short * Video_Mem_ptr	= (short *) FPGA_ONCHIP_BASE;
	
	

	int x, y;
	int offset = 8<<6 +0;
	char* textptr;
	char arr[][50] = {
						"Flip",
						"Mirror",
						"Rotate",
						"Black and White",
						"Invert",
						"Number of Pics =",
						"Time: ",
		
	};
	
	
	for (x = 0; x < 8;x++){
		strcpy(textptr,arr[x]);
		offset=x<<7+0;
		printMenu(textptr,offset);
		
	}
	updateMenu(9);
	//time_t now;
	//time(&now);
	//char str2[50];
	//sprintf(str2, "Time: %s",ctime(&now));

	
	
	
	
	//array used for mirroring and flipping
	short mirror[320][240];

	//run forever
	while(1){	
		
		
		
		//switch to toggle video/captured image
		if  (*SW_ptr & 0x200) {
			*(Video_In_DMA_ptr + 3)	= 0x4;				// Enable the video
		} else {
			*(Video_In_DMA_ptr + 3) = 0x0;			// Disable the video to capture one frame

			
			//FLIP IMAGE
			if  ((*KEY_ptr & 0x1) && !(*SW_ptr & 0x8)){					//key 1 corresponds to flip
				while (*KEY_ptr & 0x1){}			//runs until key released
				
				int oppositeY = 240;
				
				//store current image in mirror array
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short temp2 = *(Video_Mem_ptr + (y << 9) + x);
						mirror[x][y] = temp2;
					}
				}
				
				//use mirror array to write to pixel buffer flipped
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						*(Video_Mem_ptr + (oppositeY << 9) + x) = mirror[x][y];
						//oppositeX--;
					}
					oppositeY--;
				}
			}
			
			
			
			
			
			
			//MIRROR IMAGE
			if  ((*KEY_ptr & 0x2) && !(*SW_ptr & 0x8)){
				while (*KEY_ptr & 0x2){}
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short temp2 = *(Video_Mem_ptr + (y << 9) + x);
						mirror[x][y] = temp2;
					}
				}
				for (y = 0; y < 240; y++) {
					int oppositeX = 320;
					for (x = 0; x < 320; x++) {
						*(Video_Mem_ptr + (y << 9) + oppositeX) = mirror[x][y];
						oppositeX--;
					}
				}
			}
			
			
			
			
			
			//ROTATE 90 DEGREES
			if  ((*KEY_ptr & 0x4) && !(*SW_ptr & 0x8)){
				while (*KEY_ptr & 0x4){}
				for (y = 0; y < 240; y++) {
					int newX = 240;
					for (x = 0; x < 240; x++) {
						short temp2 = *(Video_Mem_ptr + (y << 9) + x);
						mirror[y][newX] = temp2;
						newX--;
					}
				}
				
				for (y = 240; y > 0; y--) {
					for (x=0; x < 320; x++) {
						if (x >= 240) {
							*(Video_Mem_ptr + (y << 9) + x) = 0x0;
						} else {
							*(Video_Mem_ptr + (y << 9) + x) = mirror[x][y];	
						}
						
					}
				}
			}
			
			
			//BLACK AND WHITE
			if  ((*KEY_ptr & 0x8) && !(*SW_ptr & 0x8)){
				while (*KEY_ptr & 0x8){}
				int thresholdBW = 20;
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short temp2 = *(Video_Mem_ptr + (y << 9) + x);
						int red = temp2 >> 11;
						int blue = temp2 << 11 >> 11;
						int green = temp2 << 5 >> 10;
						int sum = red + blue + green;
						if (sum >= thresholdBW) {
							temp2 = 0x0;	//black
						} else {
							temp2 = 0xFFFF;		//white
						}
						*(Video_Mem_ptr + (y << 9) + x) = temp2;
					}
				}
			}
			
			// Invert B&W
			if  ((*KEY_ptr & 0x8) && (*SW_ptr & 0x8)){		// SWITCH 8 IS MOD MENU
				while (*KEY_ptr & 0x8){}
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short temp2 = *(Video_Mem_ptr + (y << 9) + x);
						if (temp2 == 0x0) {
							temp2 = 0xFFFF;	
						} else {
							temp2 = 0x0;	
						}
						*(Video_Mem_ptr + (y << 9) + x) = temp2;
					}
				}
			}
			
			if  (*SW_ptr & 0x100){					//Switch 8 is B&W
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short temp2 = *(Video_Mem_ptr + (y << 9) + x);
						/*if(){
							
						}*/
						*(Video_Mem_ptr + (y << 9) + x) = temp2;
					}
				}
			}
			
		}
		
		//}

		/*while (1)
		{
			if (*KEY_ptr != 0)						// check if any KEY was pressed
			{
				break;
			}
		}*/

		
	}	
	//here

}

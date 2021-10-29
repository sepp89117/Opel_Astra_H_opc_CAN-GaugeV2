/*
 Name:		bcButton.h
 Created:	08.03.2020 18:13:06
 Author:	Sebastian Balzer
 Editor:	http://www.visualmicro.com
*/

#ifndef _bcButton_h
#define _bcButton_h

#ifdef __cplusplus
#include "Arduino.h"
#include "ILI9486_t3n.h"
#include "ili9486_t3n_font_Arial.h"

class Button {
public:
	char* Text;
	uint16_t TextColor;
	int X, Y, W, H;
	bool Enabled = false;
	bool Visible = false;
	const short unsigned int* Image;

	Button(int x, int y, int w, int h, char *text, uint16_t color, const short unsigned int *image) {
		
		
		Text = text;
		
		TextColor = color;
		X = x;
		Y = y;
		W = w;
		H = h;
		Enabled = true;
		Visible = false;
		Image = image;
	};

	Button() {
	};

	void drawButton(bool enabled, ILI9486_t3n &Tft) { //Button&
		Tft.setFont(Arial_16);
		int tXsize = Tft.strPixelLen(Text);

		//Background
		Tft.writeRect(X, Y, W, H, Image);

		//Text		
		if(sizeof(Text) > 1){
			Tft.setCursor(X + (W / 2) - (tXsize / 2), Y + ((H - 18) / 2));
			if(enabled) Tft.setTextColor(TextColor);
			else Tft.setTextColor(0xA534);		
			Tft.print(Text);
		}

		Visible = true;
		Enabled = enabled;

//    return *this;
	}

  void makeInvisible(){
    Visible = false;
  }

	bool isTouched(int touchX, int touchY) {
		if (Visible && Enabled && touchX >= X && touchX <= X + W && touchY >= Y && touchY <= Y + H) return true;
		else return false;
	}

};

#endif

#endif

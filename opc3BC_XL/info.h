class info {
  public:
    char* Text;
    uint16_t TextColor;
    int Prio;
    unsigned long Dur;
    bool firstDrawn = false;
    bool enabled = true;
    int X, Y, W, H;
    const unsigned short* Img;

    info(const char* text, uint16_t color, int prio, int dur) {
      Text = (char*)text;
      TextColor = color;
      Prio = prio;
      Dur = dur;
      enabled = true;
    };

    info(const char* text, uint16_t color, int prio, int dur, const unsigned short *img) {
      Text = (char*)text;
      TextColor = color;
      Prio = prio;
      Dur = dur;
      enabled = true;
      Img = img;
    };

    info() {
      Text = (char*)"";
      TextColor = 0;
      Prio = 0;
      Dur = 0;
    };

    bool operator == (info cInfo) {
      return Text == cInfo.Text;
    };

    bool operator != (info cInfo) {
      return Text != cInfo.Text;
    };

    bool isTouched(int touchX, int touchY) {
      if (enabled && touchX >= X && touchX <= X + W && touchY >= Y && touchY <= Y + H) return true;
      else return false;
    }
};

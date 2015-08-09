#include "WProgram.h"

#include "Encoder.h"
#include "Bounce.h"
#include <i2c_t3.h>
#include <SeeedOLED.h>


#define OLED_BUFFER_SIZE 256
class OLEDBuffer {
public:
    OLEDBuffer(){
        write_index = 0;
        read_index = 0;
        line = 0;
    };
    void init(){
        Wire.begin();
        SeeedOled.init();
        SeeedOled.clearDisplay();
        SeeedOled.setTextXY(2,4);
        SeeedOled.putString("Redshell");
        SeeedOled.setTextXY(3,3);
        SeeedOled.putString("Controller");
        SeeedOled.setTextXY(5,2);
        SeeedOled.putString("(by modlfo)");
    };

    void setTextXY(uint8_t col,uint8_t row){
        putChar(0);
        putChar(col);
        putChar(row);
    };

    void putString(const char* text){
        char* ptr = (char*) text;
        while(*ptr){
            putChar(*ptr);
            ptr++;
        }
    };

    void putChar(char c ){
        buffer[write_index] = c;
        write_index = (write_index+1)%OLED_BUFFER_SIZE;
    };

    void putPercentage(int16_t n){
        int16_t num = n / 0x147;
        char buff[3];
        int8_t i=0;
        if(num==0){
            buff[i] = '0';
            i++;
        }
        while(num>0){
            buff[i] = (num % 10) +'0';
            i++;
            num /= 10;
        }
        i--;
        for(int j=0;j<(2-i);j++){
            putChar(' ');
        }
        do{
            putChar(buff[i]);
            i--;
        }while(i>=0);
    };

    void send(){
        if(write_index!=read_index){
            char c = buffer[read_index];
            if(c==0){ // setTextXY position
                read_index = (read_index +1) % OLED_BUFFER_SIZE;
                uint8_t col = buffer[read_index];
                read_index = (read_index +1) % OLED_BUFFER_SIZE;
                uint8_t row = buffer[read_index];
                read_index = (read_index +1) % OLED_BUFFER_SIZE;
                SeeedOled.setTextXY(col,row);
            }
            else
            {
                if(line>=8){
                    read_index = (read_index +1) % OLED_BUFFER_SIZE;
                    line = 0;
                }
                else{
                    SeeedOled.putCharLine(c,line);
                    line++;
                }
            }
        }
    };

    void clearDisplay(){
        write_index = 0;
        read_index = 0;
        line = 0;
        for(int j=0;j<7;j++)
          {
            setTextXY(j,0);
            {
              for(int i=0;i<16;i++)  //clear all columns
              {
                putChar(' ');
              }
            }
          }
    }
protected:
    uint8_t write_index,read_index;
    uint8_t line;
    char buffer[OLED_BUFFER_SIZE];
};

// Class that handles the screen, buttons and the encoders
class UI {
public:
    UI():
        encoder1(17,16),
        encoder2(15,14),
        encoder3(12,11),
        encoder4(7,6),
        button1(5,10),
        button2(4,10),
        button3(3,10),
        button4(2,10)
        {
            for(int8_t i = 0; i<16;i++){
                controls[i] = 0;
                disp_controls[i] = 0;
            }
            selector = -1;
            encoders[0] = &encoder1;
            encoders[1] = &encoder2;
            encoders[2] = &encoder3;
            encoders[3] = &encoder4;
        };

    void init(){
        // Initializes the pins
        pinMode(5, INPUT_PULLUP);
        pinMode(4, INPUT_PULLUP);
        pinMode(3, INPUT_PULLUP);
        pinMode(2, INPUT_PULLUP);
        // Add string ends to the labels buffer
        for(int8_t i=0; i<16; i++){
            char label[10];
            sprintf(label,"CC %i",30+i);
            putLabel(label,i);
        }
        // Initializes the screen
        oled.init();
        send_counter = 0;
    };

    // Process the ui
    void process(){
        // sends whatever the oled has in buffer
        oled.send();

        // Updates the buttons bounce filters
        button1.update(); button2.update(); button3.update(); button4.update();
        // Button 1 selects page 1
        if(button1.fallingEdge()){
            selector = 0;
            oled.clearDisplay();
            oled.setTextXY(0,6);
            oled.putString("Page 1");
            force = 1;
        }
        // Button 2 selects page 1
        if(button2.fallingEdge()){
            selector = 1;
            oled.clearDisplay();
            oled.setTextXY(0,6);
            oled.putString("Page 2");
            force = 1;
        }
        // Button 3 selects page 3
        if(button3.fallingEdge()){
            selector = 2;
            oled.clearDisplay();
            oled.setTextXY(0,6);
            oled.putString("Page 3");
            force = 1;
        }
        // Button 3 selects page 3
        if(button4.fallingEdge()){
            selector = 3;
            oled.clearDisplay();
            oled.setTextXY(0,6);
            oled.putString("Page 4");
            force = 1;
        }

        if(selector>=0 && selector<4){
            int8_t index = 4 * selector;
            // Process the encoders depending on the active page
            for(int8_t i = 0; i<4; i++){
                controls[index+i] = readEncoderParam(*encoders[i],controls[index+i]);
            }
            update();
            force = 0;
        }
    };

    void setControl(int8_t val,int8_t index) {
        controls[index] = val;
    };
    int16_t getControl(int8_t index) {
        return controls[index];
    };
    void putLabel(char* txt,int8_t index){
        char* label = (char*) &labels[index*9];
        memcpy(label,txt,8);
        label[8] = 0;
    };

protected:

    int16_t readEncoderParam(Encoder& encoder,int16_t param){
        int16_t val = encoder.read();
        int32_t ret_param = param;
        int32_t abs_val = val > 0 ? val : -val;
        if(abs_val>2){
            encoder.write(0);
            ret_param += val*100;
            ret_param = ret_param > 0x7FFF ? 0x7FFF : ret_param;
            ret_param = ret_param < 0 ? 0 : ret_param;
        }
        return ret_param;
    };

    // Updates the parameter values in the screen
    void update(){

        if(selector>=0){
            int8_t index = 4 * selector;
            char* label;
            for(int8_t i = 0; i<4; i++){
                label = (char*)&labels[(index+i)*9];
                if(disp_controls[index+i] != controls[index+i] || force ){
                    updateParam(controls[index+i],label,2+i);
                    disp_controls[index+i] = controls[index+i];
                }
            }
        }

    };

    void updateParam(int16_t val,char* label,uint8_t pos){
        oled.setTextXY(pos,1);
        oled.putString(label);
        oled.setTextXY(pos,10);
        oled.putPercentage(val);
        oled.putString(" % ");

    };

    Encoder encoder1;
    Encoder encoder2;
    Encoder encoder3;
    Encoder encoder4;

    Bounce button1;
    Bounce button2;
    Bounce button3;
    Bounce button4;

    OLEDBuffer oled;

    uint8_t send_counter;

    uint8_t selector,force;
    uint8_t current_encoder;

    int16_t controls[16];
    int16_t disp_controls[16];
    Encoder* encoders[4];
    char labels[9*16];
};

// User interface object
UI ui;
volatile uint8_t done;
uint8_t midi_event;


// Handles the ontrol change
void OnControlChange(byte channel, byte control, byte value){
    if(control>=30 && control<=46){
        int8_t index = control - 30;
        ui.setControl(index,value << 7);
    }
}

// Handles note on events
void OnNoteOn(byte channel, byte note, byte velocity){
    // If the velocity is larger than zero, means that is turning on
    if(velocity){
        // Sets the 7 bit midi value as pitch
    }
    else {
    }

}

extern "C" int main(void)
{

    // Initializes the objects
    pinMode(13, OUTPUT);
    pinMode(23, OUTPUT);
    //analogWriteResolution(12);

    // Defines the handlers of midi events
    usbMIDI.setHandleControlChange(OnControlChange);
    usbMIDI.setHandleNoteOn(OnNoteOn);

    ui.init();
    //delay(3000);

    // Loop
    while (1) {
        // Set the pin to 1 to mark the begining of the render cycle

        // Process the buttons and screen
        ui.process();
        // Reads the midi data
        usbMIDI.read();
        // Set the pin low to mark the end of rendering and processing
    }

}




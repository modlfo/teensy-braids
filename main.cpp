#include "WProgram.h"

#include "Encoder.h"
#include "Bounce.h"
#include <i2c_t3.h>
#include <SeeedOLED.h>
#include "blit.h"
#include "svf.h"
#include "vultin.h"
#include "monoin.h"
#include "adsr.h"

IntervalTimer myTimer;

const uint16_t kAudioBlockSize = 64;

uint8_t sync_buffer[kAudioBlockSize];

int32_t bufferA[kAudioBlockSize];
int32_t bufferB[kAudioBlockSize];
uint8_t buffer_sel;

uint8_t buffer_index;
volatile uint8_t wait;


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
        SeeedOled.putString("Synthesizer");
        SeeedOled.setTextXY(5,1);
        SeeedOled.putString("(Vult Engine)");
    };

    void setTextXY(uint8_t col,uint8_t row){
        putChar(0);
        putChar(col);
        putChar(row);
    };

    void putString(char* text){
        char* ptr=text;
        while(*ptr){
            putChar(*ptr);
            ptr++;
        }
    };

    void putChar(char c ){
        buffer[write_index] = c;
        write_index = (write_index+1)%OLED_BUFFER_SIZE;
    };

    void putPercentage(int32_t n){
        int32_t num = n / 0x147;
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
            shape = 0;
            disp_shape = 0;
            timbre = 0;
            disp_timbre = 0;
            cut = 0x7FFF;
            disp_cut = 0;
            selector = -1;
            sustain = 0x7FFF;
        };

    void init(){
        // Initializes the pins
        pinMode(5, INPUT_PULLUP);
        pinMode(4, INPUT_PULLUP);
        pinMode(3, INPUT_PULLUP);
        pinMode(2, INPUT_PULLUP);
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
        // When the button 1 is pressed select oscillator configuration
        if(button1.fallingEdge()){
            selector = 0;
            oled.clearDisplay();
            oled.setTextXY(0,3);
            oled.putString("Oscillator");
            force = 1;
        }
        // Button 2 selects the envelope
        if(button2.fallingEdge()){
            selector = 1;
            oled.clearDisplay();
            oled.setTextXY(0,4);
            oled.putString("Envelope");
            force = 1;
        }
        // Button 3 is the LFO
        if(button3.fallingEdge()){
            selector = 2;
            oled.clearDisplay();
            oled.setTextXY(0,6);
            oled.putString("LFO");
            force = 1;
        }

        // Process the encoders depending on the active page
        switch(selector){
            case 0:
                switch(current_encoder){
                    case 0: shape  = readEncoderParam(encoder1,shape); break;
                    case 1: timbre = readEncoderParam(encoder2,timbre); break;
                    case 2: cut  = readEncoderParam(encoder3,cut); break;
                    case 3: res  = readEncoderParam(encoder4,res); break;
                    default: break;
                }
                break;
            case 1:
                switch(current_encoder){
                    case 0: attack  = readEncoderParam(encoder1,attack); break;
                    case 1: decay   = readEncoderParam(encoder2,decay); break;
                    case 2: sustain = readEncoderParam(encoder3,sustain); break;
                    case 3: release = readEncoderParam(encoder4,release); break;
                    default: break;
                }
                break;
            default:
                int32_t dummy = 0;
                switch(current_encoder){
                    case 0: dummy = readEncoderParam(encoder1,dummy); break;
                    case 1: dummy = readEncoderParam(encoder2,dummy); break;
                    case 2: dummy = readEncoderParam(encoder3,dummy); break;
                    case 3: dummy = readEncoderParam(encoder4,dummy); break;
                    default: break;
                }
                break;
        }

        current_encoder = (current_encoder+1)%4;
        update();
        force = 0;
    };

    void    setShape(int32_t val) { shape  = val; };
    int32_t getShape(void)        { return shape; };
    void    setTimbre(int32_t val){ timbre = val; };
    int32_t getTimbre(void)       { return timbre;};
    void    setCut(int32_t val)   { cut  = val; };
    int32_t getCut(void)          { return cut; };
    void    setRes(int32_t val)   { res  = val; };
    int32_t getRes(void)          { return res; };

    void    setAttack(int32_t val)  { attack  = val; };
    int32_t getAttack(void)         { return attack; };
    void    setDecay(int32_t val)   { decay  = val; };
    int32_t getDecay(void)          { return decay; };
    void    setSustain(int32_t val) { sustain  = val; };
    int32_t getSustain(void)        { return sustain; };
    void    setRelease(int32_t val) { release  = val; };
    int32_t getRelease(void)        { return release; };


protected:

    int32_t readEncoderParam(Encoder& encoder,int32_t param){
        int32_t val = encoder.read();
        int32_t ret_param = param;
        int32_t abs_val = val > 0 ? val : -val;
        if(abs_val>2){
            encoder.write(0);
            ret_param += val*256;//128
            ret_param = ret_param > 0x7FFF ? 0x7FFF : ret_param;
            ret_param = ret_param < 0 ? 0 : ret_param;
        }
        return ret_param;
    };

    // Updates the parameter values in the screen
    void update(){
        switch(selector){
            case 0:
                if(disp_shape!=shape || force ) { updateParam(shape,  "Shape  : ",2); disp_shape = shape; }
                if(disp_timbre!=timbre || force){ updateParam(timbre, "Timbre : ",3); disp_timbre = timbre; }
                if(disp_cut!=cut || force)  { updateParam(cut,  "Cut  : ",4); disp_cut = cut;}
                if(disp_res!=res || force)  { updateParam(res,  "Res  : ",5); disp_res = res;}
                break;
            case 1:
                if(disp_attack!=attack || force )  { updateParam(attack, "Attack : ",2); disp_attack = attack; }
                if(disp_decay!=decay || force)     { updateParam(decay,  "Decay  : ",3); disp_decay = decay; }
                if(disp_sustain!=sustain || force) { updateParam(sustain,"Sustain: ",4); disp_sustain = sustain;}
                if(disp_release!=release || force) { updateParam(release,"Release: ",5); disp_release = release;}
                break;
            default: break;
        }
    };

    void updateParam(int32_t val,char* label,uint8_t pos){
        oled.setTextXY(pos,1);
        oled.putString(label);
        oled.setTextXY(pos,9);
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

    int32_t shape;
    int32_t disp_shape;
    int32_t timbre;
    int32_t disp_timbre;
    int32_t cut;
    int32_t disp_cut;
    int32_t res;
    int32_t disp_res;

    int32_t attack;
    int32_t disp_attack;
    int32_t decay;
    int32_t disp_decay;
    int32_t sustain;
    int32_t disp_sustain;
    int32_t release;
    int32_t disp_release;

};

// User interface object
UI ui;
volatile uint8_t done;
uint8_t midi_event;

_blit_struct_osc osc;
_state_variable_struct_svf svf;
_monoin_struct_0 monoin;
_adsr_struct_adsr adsr;
// Timer interruption to put the following sample
void putSample(void){
    if(!done){
        midi_event = ~midi_event;
        digitalWriteFast(13, midi_event);
    }

    int32_t val;
    if(buffer_sel)
        val = ((bufferB[buffer_index]+0x7FFF))>>4;
    else
        val = ((bufferA[buffer_index]+0x7FFF))>>4;

    buffer_index = buffer_index+1;
    analogWrite(A14, val);
    if(buffer_index>=kAudioBlockSize) {
        wait = 0;
        buffer_index = 0;
        done = 1;
        buffer_sel =~buffer_sel;
    }

}

int32_t pitch,pre_pitch;
int32_t p1,p1_pre,p2,p2_pre;

// Handles the ontrol change
void OnControlChange(byte channel, byte control, byte value){
    if(control==32){
        ui.setShape(value << 7);
    }
    if(control==33){
        ui.setCut(value << 7);
    }
    if(control==34){
        ui.setTimbre(value << 7);
    }
}

// Handles note on events
void OnNoteOn(byte channel, byte note, byte velocity){
    // If the velocity is larger than zero, means that is turning on
    if(velocity){
        pitch = _monoin__noteOn(&monoin,fix_from_int(note));
    }
    else{
        pitch = _monoin__noteOff(&monoin,fix_from_int(note));
    }

}

// Handles note on events
void OnNoteOff(byte channel, byte note, byte velocity){
    pitch = _monoin__noteOff(&monoin,fix_from_int(note));

}

extern "C" int main(void)
{
    // Initalizes the buffers to zero
    memset(bufferA, 0, kAudioBlockSize);
    memset(bufferB, 0, kAudioBlockSize);

    // Global used to trigger the next buffer to render
    wait = 0;

    // Initializes the objects
    myTimer.begin(putSample,1e6/32000.0);

    pinMode(13, OUTPUT);
    pinMode(23, OUTPUT);
    analogWriteResolution(12);

    // Defines the handlers of midi events
    usbMIDI.setHandleControlChange(OnControlChange);
    usbMIDI.setHandleNoteOn(OnNoteOn);
    usbMIDI.setHandleNoteOff(OnNoteOff);

    pitch = fix_from_int(44);

    ui.init();
    _blit_struct_osc_init(&osc);
    _state_variable_struct_svf_init(&svf);
    _monoin_struct_0_init(&monoin);
    _adsr_struct_adsr_init(&adsr);
    //delay(3000);

    // Loop
    while (1) {
        // Set the pin to 1 to mark the begining of the render cycle
        digitalWriteFast(23,HIGH);
        // Clears the render buffer
        memset(sync_buffer, 0, sizeof(sync_buffer));
        // If the pitch changes update it
        if(pre_pitch!=pitch){

            pre_pitch = pitch;
        }

        int32_t shape = ui.getShape()*2;
        int32_t timbre = ui.getTimbre()*2;
        int32_t cut = ui.getCut()*2;
        int32_t res = ui.getRes()*2;
        int32_t attack = ui.getAttack()*2;
        int32_t decay = ui.getDecay()*2;
        int32_t sustain = ui.getSustain()*2;
        int32_t release = ui.getRelease()*2;

        if(buffer_sel){
            for(int i=0;i<kAudioBlockSize;i++){
                int32_t v = _blit__osc(&osc,pitch,timbre,shape);
                int32_t env = _adsr__adsr(&adsr,_monoin__isGateOn(&monoin),attack,decay,sustain,release);
                v = _state_variable__svf(&svf,v,cut,res,0);
                //int32_t env = _monoin__isGateOn(&monoin);
                bufferA[i] = fix_mul(v,env);
            }
        }
        else{
            for(int i=0;i<kAudioBlockSize;i++){
                int32_t v = _blit__osc(&osc,pitch,timbre,shape);
                int32_t env = _adsr__adsr(&adsr,_monoin__isGateOn(&monoin),attack,decay,sustain,release);
                v = _state_variable__svf(&svf,v,cut,res,0);
                //int32_t env = _monoin__isGateOn(&monoin);
                bufferB[i] = fix_mul(v,env);
            }
        }
        // Process the buttons and screen
        ui.process();
        // Reads the midi data
        usbMIDI.read();
        // Set the pin low to mark the end of rendering and processing
        digitalWriteFast(23,LOW);

        // Waits until the buffer is ready to render again
        wait = 1;
        while(wait);

        done = 0;
    }

}




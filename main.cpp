#include "WProgram.h"

#include "macro_oscillator.h"
#include "envelope.h"
#include "Encoder.h"
#include "Bounce.h"
#include <i2c_t3.h>
#include <SeeedOLED.h>
using namespace braids;

MacroOscillator osc;
Envelope env;
IntervalTimer myTimer;

const uint32_t kSampleRate = 96000;
const uint16_t kAudioBlockSize = 28;

uint8_t sync_buffer[kAudioBlockSize];

int16_t bufferA[kAudioBlockSize];
int16_t bufferB[kAudioBlockSize];
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
		SeeedOled.putString("(Braids Engine)");
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
			color = 0;
			disp_color = 0;
			selector = -1;
		};

	void init(){
		pinMode(5, INPUT_PULLUP);
		pinMode(4, INPUT_PULLUP);
		pinMode(3, INPUT_PULLUP);
		pinMode(2, INPUT_PULLUP);
		oled.init();
		send_counter = 0;
	};

	void process(){
		oled.send();

		button1.update(); button2.update(); button3.update(); button4.update();
		if(button1.fallingEdge()){
			selector = 0;
			oled.clearDisplay();
			oled.setTextXY(0,3);
			oled.putString("Oscillator");
			force = 1;
		}

		if(button2.fallingEdge()){
			selector = 1;
			oled.clearDisplay();
			oled.setTextXY(0,4);
			oled.putString("Envelope");
			force = 1;
		}

		if(button3.fallingEdge()){
			selector = 2;
			oled.clearDisplay();
			oled.setTextXY(0,6);
			oled.putString("LFO");
			force = 1;
		}

		switch(selector){
			case 0:
				switch(current_encoder){
					case 0: shape  = readEncoderParam(encoder1,shape); break;
					case 1: timbre = readEncoderParam(encoder2,timbre); break;
					case 2: color  = readEncoderParam(encoder3,color); break;
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
				int16_t dummy = 0;
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

	void    setShape(int16_t val) { shape  = val; };
	int16_t getShape(void)        { return shape; };
	void    setTimbre(int16_t val){ timbre = val; };
	int16_t getTimbre(void)       { return timbre;};
	void    setColor(int16_t val) { color  = val; };
	int16_t getColor(void)        { return color; };

	void    setAttack(int16_t val)  { attack  = val; };
	int16_t getAttack(void)         { return attack; };
	void    setDecay(int16_t val)   { decay  = val; };
	int16_t getDecay(void)          { return decay; };
	void    setSustain(int16_t val) { sustain  = val; };
	int16_t getSustain(void)        { return sustain; };
	void    setRelease(int16_t val) { release  = val; };
	int16_t getRelease(void)        { return release; };
	

protected:

	int16_t readEncoderParam(Encoder& encoder,int16_t param){
		int16_t val = encoder.read();
		int32_t ret_param = param;
		int32_t abs_val = val > 0 ? val : -val;
		if(abs_val>2){
			encoder.write(0);
			ret_param += val*128;
			ret_param = ret_param > 0x7FFF ? 0x7FFF : ret_param; 
			ret_param = ret_param < 0 ? 0 : ret_param;
		}
		return ret_param;
	};
	
	void update(){
		switch(selector){
			case 0:
				if(disp_shape!=shape || force ) { updateParam(shape,  "Shape  : ",2); disp_shape = shape; }
				if(disp_timbre!=timbre || force){ updateParam(timbre, "Timbre : ",3); disp_timbre = timbre; }
				if(disp_color!=color || force)  { updateParam(color,  "Color  : ",4); disp_color = color;}
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

	void updateParam(int16_t val,char* label,uint8_t pos){
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

	int16_t shape;
	int16_t disp_shape;
	int16_t color;
	int16_t disp_color;
	int16_t timbre;
	int16_t disp_timbre;

	int16_t attack;
	int16_t disp_attack;
	int16_t decay;
	int16_t disp_decay;
	int16_t sustain;
	int16_t disp_sustain;
	int16_t release;
	int16_t disp_release;

};


UI ui;
volatile uint8_t done;
uint8_t midi_event;

void putSample(void){
	if(!done){
		midi_event = ~midi_event;
		digitalWriteFast(13, midi_event);
	}

	uint16_t val;
	if(buffer_sel)
		val = ((uint16_t)(bufferB[buffer_index]+0x7FFF))>>4;
	else	
		val = ((uint16_t)(bufferA[buffer_index]+0x7FFF))>>4;

	buffer_index = buffer_index+1;	
	analogWrite(A14, val);
	if(buffer_index>=kAudioBlockSize) {
		wait = 0;
		buffer_index = 0;
		done = 1;
		buffer_sel =~buffer_sel;
	}
		
}

int16_t pitch,pre_pitch;
int16_t p1,p1_pre,p2,p2_pre;

void OnControlChange(byte channel, byte control, byte value){
	if(control==32){
		ui.setShape(value << 7);
	}
	if(control==33){
		ui.setColor(value << 7);
	}
	if(control==34){
		ui.setTimbre(value << 7);
	}
}

void OnNoteOn(byte channel, byte note, byte velocity){
	if(velocity){
		pitch = note << 7;
		osc.Strike();
		env.Trigger(ENV_SEGMENT_ATTACK);
	}
	else{
		env.Trigger(ENV_SEGMENT_DEAD);
	}
	
}

extern "C" int main(void)
{

	memset(bufferA, 0, kAudioBlockSize);
	memset(bufferB, 0, kAudioBlockSize);
	
	buffer_index = 0;
	wait = 0;

	osc.Init();
	env.Init();
  	osc.set_shape(MACRO_OSC_SHAPE_GRANULAR_CLOUD);
  	osc.set_parameters(0, 0);
  	myTimer.begin(putSample,1e6/96000.0);

	pinMode(13, OUTPUT);
	pinMode(23, OUTPUT);
	analogWriteResolution(12);

	usbMIDI.setHandleControlChange(OnControlChange);
	usbMIDI.setHandleNoteOn(OnNoteOn);

	pitch = 44 << 7;

	ui.init();
	//delay(3000);

	while (1) {
		digitalWriteFast(23,HIGH);
		memset(sync_buffer, 0, sizeof(sync_buffer));
    	if(pre_pitch!=pitch){
    		osc.set_pitch(pitch);
    		pre_pitch = pitch;
    	}
    	osc.set_parameters(ui.getTimbre(),ui.getColor());
    	env.Update(ui.getAttack()>>8, ui.getDecay()>>8, ui.getSustain()>>8, ui.getRelease()>>8);
  		
//    	if(shape!=pre_shape){
    	uint16_t shape = ui.getShape()>>10;
    	if (shape >= MACRO_OSC_SHAPE_DIGITAL_MODULATION) {
    		  shape = MACRO_OSC_SHAPE_DIGITAL_MODULATION;
    		} else if (shape <= 0) {
    		  shape = 0;
    		}

    		MacroOscillatorShape osc_shape = static_cast<MacroOscillatorShape>(shape);//
////    		pre_shape = shape;//
    		osc.set_shape(osc_shape);
    	//}
    	if(buffer_sel){
    		osc.Render(sync_buffer, bufferA, kAudioBlockSize);
    		for(int i=0;i<kAudioBlockSize;i++){
    			uint16_t ad_value = env.Render();
    			int32_t product = static_cast<int32_t>(ad_value) * bufferA[i];
    			bufferA[i] = product>>16; 
    		}
    	}
    	else{	
    		osc.Render(sync_buffer, bufferB, kAudioBlockSize);
    		for(int i=0;i<kAudioBlockSize;i++){
    			uint16_t ad_value = env.Render();
    			int32_t product = static_cast<int32_t>(ad_value) * bufferB[i];
    			bufferB[i] = product >> 16 ;
    		}
    	}
		ui.process();
		usbMIDI.read();
		digitalWriteFast(23,LOW);
		
		wait = 1;
		while(wait) {
		}
		done = 0;
	}

}




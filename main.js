

var midi = require('midi');

output = new midi.output();

var device = 'Teensy';
/* Search the output port */
var ports = output.getPortCount();
for (var i = 0; i < ports; i++) {
name = output.getPortName(i);
if(name.indexOf(device)>-1){
  output.openPort(i);
  console.log('Opening output '+name);
  break;
	}
}

pitch = 32;
p1 = 0;
p2 = 0;
shape = 0;
setInterval(
	function(){
		pitch = pitch + 12;
		if(pitch>64) pitch = 32;
		
		output.sendMessage([0x90,pitch,100]);
		setTimeout(
			function(){
				output.sendMessage([0x90,pitch,0]);
				//output.sendMessage([0xB0,33,p1]);
				//output.sendMessage([0xB0,34,p2]);
			},50);

	},300);

setInterval(
	function(){
		shape = Math.floor((Math.random() * 128));
		p1 = Math.floor((Math.random() * 128));
		p2 = Math.floor((Math.random() * 128));
		//output.sendMessage([0xB0,32,shape]);
		//output.sendMessage([0xB0,33,p1]);
		//output.sendMessage([0xB0,34,p2]);
	},3000);
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

var slider = document.getElementById("setpoint");
var output = document.getElementById("demo");
output.innerHTML = slider.value; // Display the default slider value

// Update the current slider value (each time you drag the slider handle)
slider.oninput = function() {
  output.innerHTML = this.value;
} 

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);
    var valorNivel = myObj.nivel;
    var valorNiveInt = parseInt(valorNivel);
    updateNivel(valorNiveInt);
    document.getElementById('valor-nivel').innerHTML = myObj[keys[0]];
    for (var i = 1; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
    }
}

function updateSliderPWM(element) {
    var sliderValue = document.getElementById(element.id).value;
    console.log(sliderValue);
    websocket.send(sliderValue.toString());
}

function updateNivel(value) {
    var nivelBar = document.getElementById("nivel");
    var containerHeight = 170; // Altura del contenedor en píxeles
    var minValue = 0; // Valor mínimo esperado
    var maxValue = 45; // Valor máximo esperado

    // Calcula la altura de la barra en relación con el valor
    var height = ((value - minValue) / (maxValue - minValue)) * containerHeight;
    nivelBar.style.height = height + 'px';
}

function sendStart() {
	websocket.send("start");
}

function sendStop() {
	websocket.send("stop");
}

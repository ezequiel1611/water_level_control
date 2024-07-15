# Resumen del Proyecto
El informe describe el diseño, análisis y fabricación de un depósito de agua, con una
salida y una entrada de agua, cuyo nivel del líquido es controlado para mantenerlo estable en
el valor que el usuario desee. Empleando un sensor ultrasónico se mide el nivel del agua en
tiempo real y un controlador PI implementado en el microcontrolador ESP8266, de acuerdo
a la medición del nivel, ajusta la potencia administrada a la bomba controlando así el flujo
de entrada para mantener estable el nivel del líquido. Se realizaron pruebas para calibrar los
sensores de caudal empleados, para verificar como afecta la temperatura al sensor ultrasónico,
para verificar la linealidad del control de potencia de la bomba y para determinar el rango de
linealidad del sistema. Los resultados obtenidos demuestran que es correcto asumir un rango
lineal de funcionamiento y que dentro del mismo el controlador PI funciona correctamente
manteniendo el nivel estable con un error del 5%.

# Requisitos
- Visual Studio Code
- PlatformIO
  - Librería BMP085
  - Librería ESPAsyncWebServer
  - Librería Arduino_JSON
  - Librería SimpleKalmanFilter

# Clonar Repositorio
Para poder usar el sistema mediante la interfaz gráfica desarrollada se debe descargar este repositorio
para así compilar y subir tanto el código de control como la página web al micrcontrolador ESP8266.

## Linux
1. **Abrir una Terminal de Comandos:** Ubiquese en la carpeta donde quiera descargar el repositorio y
abra allí una terminal de comandos. También puede usar el acceso rápido `Ctrl+Alt+T` para abrir un terminal
de comandos y moverse a la ubicación deseada con el comando `cd`.
2. **Clonar el Repositorio:** En la terminal de comandos ahora escriba:
  ```
  git clone https://github.com/ezequiel1611/water_level_control
  ```
y presione `Enter` para crear una copia local de este repositorio en su computadora.

## Windows
1. **Abrir el Símbolo del Sistema:** Presione `Win+R`, escriba `cmd` y presione `Enter` para abrir el 
símbolo del sistema. Muevase hacia la carpeta donde quiere descargar el repositorio usando el comando `cd`.
Por ejemplo, si quiere tener el repositorio en una carpeta llamada `Scripts` en el Escritorio, puede escribir:
  ```
  cd %USERPROFILE%\Desktop\Scripts
  ```
2. **Clonar el Repositorio:** En el símbolo del sistema ahora escriba:
  ```
  git clone https://github.com/ezequiel1611/water_level_control
  ```
y presione `Enter` para crear una copia local de este repositorio en su computadora.

# Configurar WebSocket
Una vez ya descargados los archivos, debe abrir el proyecto usando la extensión de PlatformIO en Visual Studio Code.
Allí verá que básicamente el proyecto se divide en dos, por un lado el código de control del ESP8266 (*main.cpp*)
y por otro el desarrollo de la página web que incluye todo lo que está dentro de la carpeta *data*.

En este proyecto el ESP8266 funciona como un Access Point por lo que es el mismo microcontrolador el que genera la
red WiFi y si se desea puede modificar tanto su SSID como su contraseña cambiando los valores de las variables
*ssid* y *password* en el archivo *main.cpp*, donde la configuración del WebSocket comienza en la línea 42.
  ```
  const char* ssid = "ControlDeNivel";
  const char* password = "patronato1914";
  ```
También puede cambiar la dirección IP donde se creará la paǵina web, para lo cual debe modificar la variable:
  ```
  IPAddress ip(192,168,1,200);
  ```
Si se desea también puede modificar tanto la puerta de enlace como la máscara de subred, cambiando las variables:
  ```
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  ```
Si no desea modificar la configuración del WebSocket, esta configuración es la que se emplea por defecto.

# Controlador PI
Si se desea modificar las constantes *Kp* y *Ki* del controlador, puede modificar las constantes definidas en el
archivo *main.cpp* las cuáles por defecto son las obtenidas por el algoritmo de optimización genético usado.
  ```
  #define Kp 35.615
  #define Ki 4.58
  ```
Si no desea modificar las constantes del controlador PI, estos son los valores empleados por defecto.

# Instrucciones de Uso
Ya una vez se configurado tanto el WebSocket como el controlador PI, primero se debe compilar y subir el código
al ESP8266 y luego se podrá conectar a la red WiFi para controlar el sistema mediante la página web.
## Compilar y Subir Archivos
1. **Compilar el Código Principal:** Para compilar el archivo *main.cpp*, seleccionelo y luego presione el atajo
   `Alt+Ctrl+B`.
2. **Compilar Archivos de la Página Web:** Para compilar los archivos *index.html*, *style.css* y *script.js*
pertenecientes a la página web debe presionar el ícono de PlatformIO en la barra lateral izquierda de extensiones
y en la sección de PROJECT TASKS desglose la carpeta *Platform* y haga click en *Build Filesystem Image*.
3. **Subir Archivos al ESP8266:** Para subir el archivo *main.cpp*, seleccionelo y luego presione el atajo
`Alt+Ctrl+U`. Ahora, para subir los archivos de la página web presione el ícono de PlatformIO en la barra lateral
izquierda de extensiones y en la sección PROJECT TASKS desglose la carpeta *Platform* y
haga click en *Upload Filesystem Image*.

Con esto, tanto el programa de control como la página web ya se comenzarán a funcionar apenas se energize el sistema.

## Establecer la Conexión
Primeramente debe conectarse a la red WiFi creada por el ESP8266, la cuál será por defecto:

**SSID:** *ControlDeNivel*

**Contraseña:** *patronato1914*

Ahora abrá cualquier navegador web e ingrese la dirección IP de la página web creada, la cuál será por defecto:
`http://192.168.1.200/`

- Si tiene algún problema a la hora de mantenerse conectado a la red WiFi usando algún dispositivo móvil, vaya
a la sección de ajustes de WiFi y seleccione la opción de *Ajustes de IP Estática* en lugar de *DHCP*.

## Página Web
![previsualización de la página web](https://github.com/ezequiel1611/water_level_control/blob/master/web_design.png)

La página web cuenta con tres controles que el usuario puede emplear, los primero dos son botones de *Start* y *Stop*
que encienden o apagan respectivamente la bomba de agua. Luego se presenta un controlador deslizable, el cuál sirve para que el usuario pueda determinar el nivel del agua deseado que quiere que el sistema mantenga de forma estable. 

Al encender el sistema la bomba estará apagada y el usuario debe presionar el botón de *Start* para poder comenzar
a usar el control de nivel. Un LED rojo indica el estado de la bomba, siempre que el mismo este apagado la bomba también lo estará y viceversa.

Una vez encendida la bomba, el nivel inicial es 0 cm por lo el sistema no actuará en un principio. El usuario debe usar el control deslizable para establecer el nivel deseado, el cual se muestra en la parte inferior del slider, y cuando se suelta el control ahí el sistema comenzará a trabajar para lograr establecer ese nivel del líquido. En cualquier momento el usuario puede cambiar el nivel objetivo deseado. También podrá apagar o encender la bomba en cualquier momento sin importar el nivel objetivo establecido.

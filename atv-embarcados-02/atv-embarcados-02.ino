#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 

// **** CONFIGURAÇÕES DE REDE (MUDE AQUI!) ****
const char* ssid = "Azul123-2G";
const char* password = "minhacasinha";

const char* config_url = "https://api.npoint.io/eabec6f23473f8c2c502";


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


const int LDR_PIN = 19;     // Pino do sensor LDR 
const int ACTUATOR_PIN = 2; // Pino padrão para o atuador


int global_actuator_pin = ACTUATOR_PIN;
int timespan = 0;
int loop_counter = 0; // Variável global para contar as rodadas
int run_again = NULL;

// ------------------------------------------------
// Função para buscar a configuração via GET
// ------------------------------------------------
void get_config() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(config_url);

        Serial.println("Enviando GET para buscar configuração...");
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (error) {
                Serial.print("Erro ao ler JSON: ");
                Serial.println(error.f_str());
            } else {
                // Atualiza o Timespan
                if (doc.containsKey("timespan")) {
                    timespan = doc["timespan"].as<int>();

                    Serial.print("Novo Timespan: ");
                    Serial.println(timespan);
                }
                if (doc.containsKey("run_again")) {
                    run_again = doc["run_again"].as<int>();
                }
                // Atualiza e reconfigura o pino do atuador
                if (doc.containsKey("actuator_pin")) {
                    int new_pin = doc["actuator_pin"].as<int>();
                    if (new_pin != global_actuator_pin) {
                        global_actuator_pin = new_pin;
                        pinMode(global_actuator_pin, OUTPUT);
                        Serial.print("Novo Actuator Pin: ");
                        Serial.println(global_actuator_pin);
                    }
                }
            }
        } else {
            Serial.print("Erro no GET: ");
            Serial.println(http.errorToString(httpCode));
        }
        http.end();
    } else {
        Serial.println("Wi-Fi não conectado. Pulando GET.");
    }
}

// ------------------------------------------------
// Setup (Executa apenas uma vez)
// ------------------------------------------------
void setup() {
    Serial.begin(115200);

    // Configura os pinos
    pinMode(LDR_PIN, INPUT);
    pinMode(ACTUATOR_PIN, OUTPUT); 

    // Inicializa o display OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("Erro ao inicializar OLED");
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Conecta ao WiFi (apenas no Setup)
    Serial.print("Conectando ao WiFi: ");
    Serial.println(ssid);
    display.setCursor(0, 0);
    display.print("Conectando WiFi...");
    display.display();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado ao WiFi!");

    // Busca a configuração inicial após conectar
    get_config();
}

// ------------------------------------------------
// Loop principal (Executa repetidamente)
// ------------------------------------------------
void loop() {
    // 1. BUSCA A CONFIGURAÇÃO (GET NA API) A CADA LOOP
    get_config(); 

    // Se o timespan for maior que 0 e o contador atingir o limite, imprime a mensagem.
    if (timespan > 0 && loop_counter >= timespan) {

        digitalWrite(global_actuator_pin, LOW);
        Serial.println("=====================================");
        Serial.println("LIMITE DE LEITURA ATINGIDO. SISTEMA PARADO.");
        Serial.println("Reinicie o dispositivo para retomar.");
        Serial.println("=====================================");
        // Prepara e mostra a mensagem de parada no OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.setTextSize(2);
        display.println("IT'S WORKING!");
        display.println("");
        display.println("LET'S GO!!");
        display.display();

        while(true) {
            get_config();
            if (run_again == 1) {
                loop_counter = 0;
                break;
            }
            delay(1000); 
        }
        
    } else {
        
        // 2. Lê o valor do LDR (digitalRead mantido conforme sua lógica)
        int ldrValue = digitalRead(LDR_PIN);
        String brightness_status;

        // 3. Lógica de controle: LIGA o atuador se LDR == 1
        if (ldrValue == 1) {
            digitalWrite(global_actuator_pin, HIGH); // Liga atuador
            brightness_status = "Status: ON (Escuro)";
        } else {
            digitalWrite(global_actuator_pin, LOW);  // Desliga atuador
            brightness_status = "Status: OFF (Claro)";
        }

        // 4. Atualiza o display OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        
        // Exibe a contagem de rodadas: (X/Y)
        display.print("(");
        // Exibe o número da rodada atual (começando em 1)
        display.print(loop_counter + 1); 
        display.print("/");
        display.print(timespan); 
        display.print(") LDR: ");
        
        display.println(ldrValue);
        
        display.print("Pin: ");
        display.println(global_actuator_pin);

        display.setCursor(0, 30);
        display.println(brightness_status);

        display.setCursor(0, 50);
        display.print("IP: ");
        display.print(WiFi.localIP());
        display.display();

        // 5. Log no Serial Monitor
        Serial.print("(");
        Serial.print(loop_counter + 1);
        Serial.print("/");
        Serial.print(timespan);
        Serial.print(") LDR: ");
        Serial.print(ldrValue);
        Serial.print(" | Pino: ");
        Serial.print(global_actuator_pin);
        Serial.print(" | ");
        Serial.println(brightness_status);
        
        loop_counter++;

        // 6. Espera 5 segundos antes do próximo ciclo (e do próximo GET)
        delay(2000);
    }
}
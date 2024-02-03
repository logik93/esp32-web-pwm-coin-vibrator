#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

const char *ssid = "GET YER OWN WIFI";
const char *password = "1234";

const int pwmPin = 13; // Pin GPIO, który używamy do generowania PWM
const int pwmFrequency = 5000; // Częstotliwość PWM w Hz
const int pwmResolution = 8; // Rozdzielczość PWM (od 1 do 16, im większa, tym dokładniejsza)

int pwmValue = 0; // Początkowa wartość PWM

// Parametry fali LFO
float lfoFrequency = 256; // Częstotliwość fali LFO w Hz
float lfoAmplitude = 255; // Amplituda fali LFO (od 0 do 255) można więcej nawet

// Zmienna do przechowywania czasu ostatniego zmierzonego punktu fali LFO
unsigned long lastLFOTime = 0;

// Utwórz obiekt serwera WWW
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // Inicjalizuj Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Inicjalizuj PWM
  ledcSetup(0, pwmFrequency, pwmResolution);
  ledcAttachPin(pwmPin, 0);

  // Definiuj ścieżkę obsługującą żądania PWM
  server.on("/setPWM", HTTP_POST, [](AsyncWebServerRequest *request){
    String valueStr = request->arg("value");
    pwmValue = valueStr.toInt();
    ledcWrite(0, pwmValue);
    request->send(200, "text/plain", "OK");
  });

  // Definiuj ścieżkę obsługującą żądania LFO
  server.on("/setLFO", HTTP_POST, [](AsyncWebServerRequest *request){
    String freqStr = request->arg("freq");
    String ampStr = request->arg("amp");
    lfoFrequency = freqStr.toFloat();
    lfoAmplitude = ampStr.toFloat();
    request->send(200, "text/plain", "OK");
  });

  // Definiuj strony HTML i skrypty
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><body>";
    html += "<h1>ESP32 PWM Control</h1>";
    html += "<p>Current PWM Value: " + String(pwmValue) + "</p>";
    html += "<form action='/setPWM' method='post'>";
    html += "<label for='value'>Set PWM Value (0-255):</label>";
    html += "<input type='text' name='value'>";
    html += "<input type='submit' value='Set PWM'>";
    html += "</form>";

    html += "<h2>LFO Settings</h2>";
    html += "<form action='/setLFO' method='post'>";
    html += "<label for='freq'>LFO Frequency:</label>";
    html += "<input type='text' name='freq' value='" + String(lfoFrequency) + "'>";
    html += "<label for='amp'>LFO Amplitude:</label>";
    html += "<input type='text' name='amp' value='" + String(lfoAmplitude) + "'>";
    html += "<input type='submit' value='Set LFO'>";
    html += "</form>";

    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Rozpocznij serwer
  server.begin();
}

void loop() {
  // Oblicz zmianę czasu od ostatniego pomiaru LFO
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - lastLFOTime;

  // Sprawdź, czy minęła wystarczająca ilość czasu dla nowego pomiaru LFO
  if (elapsedTime >= (1000 / lfoFrequency)) {
    // Oblicz nową wartość fali LFO
    float lfoValue = lfoAmplitude * sin(2 * PI * lfoFrequency * currentTime / 1000.0);

    // Dodaj wartość LFO do aktualnej wartości PWM
    int newPwmValue = pwmValue + lfoValue;

    // Ogranicz wartość PWM do zakresu 0-255
    newPwmValue = constrain(newPwmValue, 0, 255);

    // Ustaw nową wartość PWM
    ledcWrite(0, newPwmValue);

    // Zaktualizuj czas ostatniego pomiaru LFO
    lastLFOTime = currentTime;

    // Wyświetl aktualną wartość PWM w konsoli
    Serial.println(String(newPwmValue));
  }

  // Dodatkowy kod pętli głównej (jeśli potrzebujesz)
}

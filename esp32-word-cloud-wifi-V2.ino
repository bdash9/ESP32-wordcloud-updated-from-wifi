/* Script that lets the ESP32 recieve words via web connection and create a wordcloud on the TFT screen -Ben Dash

From a command line:
curl -X POST -d "words to display " http://SERVER_IP/addwords
curl -X POST -d "words=Led Zeppelin" http://172.16.0.27/addwords

(Note the trailing space above after "words to diplay ")

Or from a browser:
Multiple words but the "+" is displayed:
http://172.16.0.27/addwords?words=Led+Zeppelin
Single words:
http://172.16.0.27/addwords?words=Hi+

*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeMono9pt7b.h> // Include a font library
//#include <SPI.h>
#include <WiFi.h>    

/* Available fonbts for the Adafruit ESP32-S2 TFT Feather
FreeMono12pt7b.h    FreeSansBoldOblique12pt7b.h
FreeMono18pt7b.h    FreeSansBoldOblique18pt7b.h
FreeMono24pt7b.h    FreeSansBoldOblique24pt7b.h
FreeMono9pt7b.h     FreeSansBoldOblique9pt7b.h
FreeMonoBold12pt7b.h    FreeSansOblique12pt7b.h
FreeMonoBold18pt7b.h    FreeSansOblique18pt7b.h
FreeMonoBold24pt7b.h    FreeSansOblique24pt7b.h
FreeMonoBold9pt7b.h   FreeSansOblique9pt7b.h
FreeMonoBoldOblique12pt7b.h FreeSerif12pt7b.h
FreeMonoBoldOblique18pt7b.h FreeSerif18pt7b.h
FreeMonoBoldOblique24pt7b.h FreeSerif24pt7b.h
FreeMonoBoldOblique9pt7b.h  FreeSerif9pt7b.h
FreeMonoOblique12pt7b.h   FreeSerifBold12pt7b.h
FreeMonoOblique18pt7b.h   FreeSerifBold18pt7b.h
FreeMonoOblique24pt7b.h   FreeSerifBold24pt7b.h
FreeMonoOblique9pt7b.h    FreeSerifBold9pt7b.h
FreeSans12pt7b.h    FreeSerifBoldItalic12pt7b.h
FreeSans18pt7b.h    FreeSerifBoldItalic18pt7b.h
FreeSans24pt7b.h    FreeSerifBoldItalic24pt7b.h
FreeSans9pt7b.h     FreeSerifBoldItalic9pt7b.h
FreeSansBold12pt7b.h    FreeSerifItalic12pt7b.h
FreeSansBold18pt7b.h    FreeSerifItalic18pt7b.h
FreeSansBold24pt7b.h    FreeSerifItalic24pt7b.h
FreeSansBold9pt7b.h   FreeSerifItalic9pt7b.h 
*/

const char* ssid = "INDIGO";
const char* password = "redsun1234";

WiFiServer server(80); // create a server on port 80
WiFiClient client; // create a client object

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

int margin = 10; // Set the margin between words

#define MAX_WORDS 10 // maximum number of words in the word cloud

String words[MAX_WORDS]; // array to store the words
int wordCount = 0; // number of words in the array

bool wordsAdded = false;

void setup() {
  Serial.begin(115200);

  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);

  WiFi.begin(ssid, password); // connect to WiFi
  tft.println("Connecting to WiFi..");

  // wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }
  tft.println();
  tft.println("Connected to WiFi!");

  server.begin(); // Start the server
  tft.println("Server started");

  // Print the IP address
  tft.print("Use this URL:       ");
  tft.print("http://");
  tft.print(WiFi.localIP());
  tft.println("/");
  delay(4000);
}

// This allows word updates from a broswer only (no curl)
void loop() {
  if (client = server.available()) { // check for incoming client connections
    // read the entire request
    String request = client.readString();
    client.flush(); // clear the buffer

    // extract the request method and path
    int firstSpaceIndex = request.indexOf(" ");
    int secondSpaceIndex = request.indexOf(" ", firstSpaceIndex + 1);
    String method = request.substring(0, firstSpaceIndex);
    String path = request.substring(firstSpaceIndex + 1, secondSpaceIndex);

    // check if the request is a GET to the /addwords path
    if (method == "GET" && path.startsWith("/addwords")) {
      // extract the words from the query parameter
      int queryStartIndex = path.indexOf("?");
      String query = path.substring(queryStartIndex + 1);
      int wordsIndex = query.indexOf("words=");
      String words = query.substring(wordsIndex + 6);

      // add the words to the array
      addWords(words);

      // set the flag to indicate that words have been added
      wordsAdded = true;
    }
  }

  if (wordsAdded) { // only draw the word cloud if words have been added
    drawWordCloud();
    delay(2500); //delay 2.5 seconds
  }
}


/* Just curl -Comment out above void loop and comment this in for curl updates only
void loop() {
  if (client = server.available()) { // check for incoming client connections
    // read the entire request
    String request = client.readString();
    client.flush(); // clear the buffer

    // extract the body of the request
    int bodyIndex = request.indexOf("\r\n\r\n");
    String body = request.substring(bodyIndex + 4);

    // add the words to the array
    addWords(body);

    // set the flag to indicate that words have been added
    wordsAdded = true;
  }

  if (wordsAdded) { // only draw the word cloud if words have been added
    drawWordCloud();
    delay(2500); //delay 2.5 seconds
  }
}
*/

void addWords(String text) {
  // split the text into individual words and add them to the array
  int startIndex = 0;
  for (int i = 0; i < text.length(); i++) {
    if (text[i] == ' ' || text[i] == '\n' || i == text.length() - 1) {
      String word = text.substring(startIndex, i);
      if (wordCount < MAX_WORDS) {
        words[wordCount] = word;
        wordCount++;
      }
      startIndex = i + 1;
    }
  }
}

void drawWordCloud() {
  // choose a random color for the background
  int r = random(0, 255);
  int g = random(0, 255);
  int b = random(0, 255);
  tft.fillScreen(tft.color565(r, g, b)); // set the background color

  int x, y; // Variables to store the x and y positions of each word

  // draw the words on the screen
  for (int i = 0; i < wordCount; i++) {
    // set the font size and color
    tft.setTextSize(2);
    tft.setFont(&FreeMono9pt7b);
    tft.setTextColor(ST77XX_WHITE);

    // convert the word to a character array
    char word[words[i].length() + 1];
    words[i].toCharArray(word, sizeof(word));

    x = random(margin, tft.width() - margin); // Generate a random x position within the margins
    y = random(margin, tft.height() - margin); // Generate a random y position within the margins

    tft.setCursor(x, y); // Set the cursor to the random position
    tft.print(words[i]); // Print the word at the cursor position
  }
}

